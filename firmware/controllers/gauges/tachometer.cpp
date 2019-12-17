/*
 * @file tachometer.cpp
 * @brief This is about driving external analog tachometers
 *
 * This implementation produces one pulse per engine cycle
 *
 * todo: these is a bit of duplication with dizzySparkOutputPin
 *
 * @date Aug 18, 2015
 * @author Andrey Belomutskiy, (c) 2012-2018
 */

#include "tachometer.h"
#include "trigger_central.h"

#if !EFI_UNIT_TEST

EXTERN_ENGINE;

static scheduling_s tachTurnSignalOff;

static void turnTachPinLow(void) {
	enginePins.tachOut.setLow();
}

static void tachSignalCallback(trigger_event_e ckpSignalType,
		uint32_t index DECLARE_ENGINE_PARAMETER_SUFFIX) {
	UNUSED(ckpSignalType);
	if (index != (uint32_t)engineConfiguration->tachPulseTriggerIndex) {
		return;
	}
	enginePins.tachOut.setHigh();
	float durationMs;
	if (engineConfiguration->tachPulseDurationAsDutyCycle) {
		// todo: implement tachPulseDurationAsDutyCycle
		durationMs = engineConfiguration->tachPulseDuractionMs;
	} else {
		durationMs = engineConfiguration->tachPulseDuractionMs;
	}
	engine->executor.scheduleForLater(&tachTurnSignalOff, (int)MS2US(durationMs), (schfunc_t) &turnTachPinLow, NULL);
}

void initTachometer(void) {
	if (CONFIG(tachOutputPin) == GPIO_UNASSIGNED) {
		return;
	}

	enginePins.tachOut.initPin("analog tach output", CONFIG(tachOutputPin), &CONFIG(tachOutputPinMode));

#if EFI_SHAFT_POSITION_INPUT
	addTriggerEventListener(tachSignalCallback, "tach", engine);
#endif /* EFI_SHAFT_POSITION_INPUT */
}

#endif /* EFI_UNIT_TEST */
