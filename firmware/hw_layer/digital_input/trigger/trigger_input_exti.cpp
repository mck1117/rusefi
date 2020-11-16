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

struct exti_snapshot
{
	uint64_t timestamp;
	bool valid;
	bool state;
};

struct cb_arg
{
	uint64_t timestamp;
	bool state;
	ioline_t line;
};

exti_snapshot snapshots[16];

#define EXTI_HANDLER(vect, exti_ch) \
CH_FAST_IRQ_HANDLER(vect)										\
{																\
	uint64_t timestamp = getTimeNowNt();						\
	uint32_t pr = EXTI->PR;										\
	pr &= EXTI->IMR & (1U << (exti_ch));						\
	EXTI->PR = pr;												\
	snapshots[exti_ch].timestamp = timestamp;					\
	snapshots[exti_ch].valid = 1;								\
	snapshots[exti_ch].state = palReadLine((ioline_t)_pal_events[exti_ch].arg) == PAL_HIGH;	\
	TIM14->EGR |= STM32_TIM_EGR_CC1G;							\
}

EXTI_HANDLER(Vector58, 0);
EXTI_HANDLER(Vector5C, 1);
EXTI_HANDLER(Vector60, 2);
EXTI_HANDLER(Vector64, 3);
EXTI_HANDLER(Vector68, 4);

CH_IRQ_HANDLER(STM32_TIM14_HANDLER) {
	OSAL_IRQ_PROLOGUE();

	// Dispatch all interrupts as necessary
	for (size_t i = 0; i < 16; i++)
	{
		if (snapshots[i].valid) {
			cb_arg arg
			{
				snapshots[i].timestamp,
				snapshots[i].state,
				(ioline_t)_pal_events[i].arg
			};

			_pal_events[i].cb(&arg);

			snapshots[i].valid = false;
		}
	}

	OSAL_IRQ_EPILOGUE();
}

static void shaft_callback(void *arg) {
	auto arg2 = reinterpret_cast<cb_arg*>(arg);

	// do the time sensitive things as early as possible!
	efitick_t stamp = arg2->timestamp;

	if (!engine->hwTriggerInputEnabled) {
		return;
	}

	ioline_t pal_line = arg2->line;
	bool rise = arg2->state;

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

	rccResetTIM14();
	rccEnableTIM14(true);
	nvicEnableVector(STM32_TIM14_NUMBER, ICU_PRIORITY);
	TIM14->DIER |= STM32_TIM_DIER_CC1IE;

	/* TODO:
	 * * do not set to both edges if we need only one
	 * * simplify callback in case of one edge */
	ioline_t pal_line = PAL_LINE(getHwPort("trg", brainPin), getHwPin("trg", brainPin));
	efiExtiEnablePin(msg, brainPin, PAL_EVENT_MODE_BOTH_EDGES, isTriggerShaft ? shaft_callback : cam_callback, (void *)pal_line);

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
