
#include "global.h"
#include "engine.h"

EXTERN_ENGINE;



efitick_t minNextEdgeTime = 0;
efitick_t maxNextEdgeTime = 0;


static void validateTrigger(trigger_event_e signal, uint32_t index, efitick_t edgeTimestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	int rpm = engine->rpmCalculator.getRpm();

	// Only check when RPM is fast enough (ie, not cranking)
	if (rpm > 500) {
		if (edgeTimestamp < minNextEdgeTime) {
			palTogglePad(GPIOD, 2);
		}

		if (edgeTimestamp > maxNextEdgeTime) {
			palTogglePad(GPIOD, 2);
		}
	}
	

	float* buffer = engine->triggerCentral.triggerShape.wave.switchTimes;
	int size = engine->triggerCentral.triggerShape.getSize();
	// Engine cycle visits the pattern twice on 4 stroke crank sensors
	int index2 = index % size;

	float currentSwitchTime = buffer[index2];

	int nextIndex = index2 + 2;

	// wrap correctly
	float nextSwitchTime = nextIndex >= size
							? buffer[0] + 1
							: buffer[nextIndex];


	float delta = nextSwitchTime - currentSwitchTime;
	float cycleTime = engine->rpmCalculator.oneDegreeUs * 720;

	float timeToNextToothUs = delta * cycleTime;

	float minAllowedUs = timeToNextToothUs * 0.9f;
	float maxAllowedUs = timeToNextToothUs * 1.1f;

	minNextEdgeTime = edgeTimestamp + US2NT(minAllowedUs);
	maxNextEdgeTime = edgeTimestamp + US2NT(maxAllowedUs);
}



void initTriggerValidator() {
		palSetPadMode(GPIOD, 2, PAL_MODE_OUTPUT_PUSHPULL);

	addTriggerEventListener(validateTrigger, "main loop", engine);
}
