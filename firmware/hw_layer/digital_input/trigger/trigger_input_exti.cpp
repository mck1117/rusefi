/**
 * @file	trigger_input_exti.cpp
 * @brief	Position sensor hardware layer - PAL version
 *
 * todo: VVT implementation is a nasty copy-paste :(
 *
 * see digital_input_icu.cp
 *
 * @date Dec 30, 2012
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "global.h"

#if EFI_SHAFT_POSITION_INPUT && (HAL_TRIGGER_USE_PAL == TRUE)

#include "trigger_input.h"
#include "digital_input_exti.h"

#if (PAL_USE_CALLBACKS == FALSE)
	#error "PAL_USE_CALLBACKS should be enabled to use HAL_TRIGGER_USE_PAL"
#endif

static Logging *logger;

EXTERN_ENGINE;
static ioline_t primary_line;

uint64_t extiShaftTime;
uint32_t pr;

CH_FAST_IRQ_HANDLER(Vector9C) {
palSetPad(GPIOA, 3);
	extiShaftTime = getTimeNowNt();
palClearPad(GPIOA, 3);

	pr = EXTI->PR;
	pr &= EXTI->IMR & ((1U << 5) | (1U << 6) | (1U << 7) | (1U << 8) |
						(1U << 9));
	EXTI->PR = pr;

	// Fire the timer interrupt now that we've grabbed a snapshot of the time
	TIM14->EGR |= STM32_TIM_EGR_CC1G;
}

#define exti_serve_irq(pr, channel) {                                       \
                                                                            \
  if ((pr) & (1U << (channel))) {                                           \
    _pal_isr_code(channel);                                                 \
  }                                                                         \
}

OSAL_IRQ_HANDLER(STM32_TIM14_HANDLER) {
	OSAL_IRQ_PROLOGUE();

	// clear interrupt flag
	TIM14->SR = 0;

	// serve as if it was an EXTI interrupt
	exti_serve_irq(pr, 5);
	exti_serve_irq(pr, 6);
	exti_serve_irq(pr, 7);
	exti_serve_irq(pr, 8);
	exti_serve_irq(pr, 9);

	OSAL_IRQ_EPILOGUE();
}

static void shaft_callback(void *arg) {
palSetPad(GPIOA, 3);
	// do the time sensitive things as early as possible!
	efitick_t stamp = extiShaftTime;
palClearPad(GPIOA, 3);
palSetPadMode(GPIOA, 3, PAL_MODE_OUTPUT_PUSHPULL);

	if (!engine->hwTriggerInputEnabled) {
		return;
	}
	ioline_t pal_line = (ioline_t)arg;
	bool rise = (palReadLine(pal_line) == PAL_HIGH);

	// todo: support for 3rd trigger input channel
	// todo: start using real event time from HW event, not just software timer?
	if (hasFirmwareErrorFlag)
		return;

	bool isPrimary = pal_line == primary_line;
	if (!isPrimary && !TRIGGER_WAVEFORM(needSecondTriggerInput)) {
		return;
	}

	trigger_event_e signal;
	// todo: add support for 3rd channel
	if (rise) {
		signal = isPrimary ?
					(engineConfiguration->invertPrimaryTriggerSignal ? SHAFT_PRIMARY_FALLING : SHAFT_PRIMARY_RISING) :
					(engineConfiguration->invertSecondaryTriggerSignal ? SHAFT_SECONDARY_FALLING : SHAFT_SECONDARY_RISING);
	} else {
		signal = isPrimary ?
					(engineConfiguration->invertPrimaryTriggerSignal ? SHAFT_PRIMARY_RISING : SHAFT_PRIMARY_FALLING) :
					(engineConfiguration->invertSecondaryTriggerSignal ? SHAFT_SECONDARY_RISING : SHAFT_SECONDARY_FALLING);
	}

	hwHandleShaftSignal(signal, stamp);
}

static void cam_callback(void *arg) {
	efitick_t stamp = getTimeNowNt();
	if (!engine->hwTriggerInputEnabled) {
		return;
	}

	ioline_t pal_line = (ioline_t)arg;

	bool rise = (palReadLine(pal_line) == PAL_HIGH);

	if (rise ^ engineConfiguration->invertCamVVTSignal) {
		hwHandleVvtCamSignal(TV_RISE, stamp);
	} else {
		hwHandleVvtCamSignal(TV_FALL, stamp);
	}
}

/*==========================================================================*/
/* Exported functions.														*/
/*==========================================================================*/

int extiTriggerTurnOnInputPin(const char *msg, int index, bool isTriggerShaft) {
	brain_pin_e brainPin = isTriggerShaft ? CONFIG(triggerInputPins)[index] : engineConfiguration->camInputs[index];

	scheduleMsg(logger, "extiTriggerTurnOnInputPin %s %s", msg, hwPortname(brainPin));

	/* TODO:
	 * * do not set to both edges if we need only one
	 * * simplify callback in case of one edge */
	ioline_t pal_line = PAL_LINE(getHwPort("trg", brainPin), getHwPin("trg", brainPin));
	efiExtiEnablePin(msg, brainPin, PAL_EVENT_MODE_BOTH_EDGES, isTriggerShaft ? shaft_callback : cam_callback, (void *)pal_line);

	// Borrow the i2c driver as an interrupt generation source
	rccResetTIM14();
	rccEnableTIM14(true);
	nvicEnableVector(STM32_TIM14_NUMBER, ICU_PRIORITY);
	TIM14->DIER |= STM32_TIM_DIER_CC1IE;

	return 0;
}

void extiTriggerTurnOffInputPin(brain_pin_e brainPin) {
	efiExtiDisablePin(brainPin);
}

void extiTriggerSetPrimaryChannel(brain_pin_e brainPin) {
	primary_line = PAL_LINE(getHwPort("trg", brainPin), getHwPin("trg", brainPin));
}

void extiTriggerTurnOnInputPins(Logging *sharedLogger) {
	logger = sharedLogger;
}

#endif /* (EFI_SHAFT_POSITION_INPUT && (HAL_TRIGGER_USE_PAL == TRUE)) */
