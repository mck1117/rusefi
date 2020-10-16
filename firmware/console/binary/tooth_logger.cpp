/*
 * @file tooth_logger.cpp
 *
 * @date Jul 7, 2019
 * @author Matthew Kennedy
 */

#include "tooth_logger.h"

#include "global.h"
#include "perf_trace.h"

#if EFI_TOOTH_LOGGER

EXTERN_ENGINE;

#include <cstddef>
#include "efitime.h"
#include "efilib.h"
#include "tunerstudio_outputs.h"

typedef struct __attribute__ ((packed)) {
    uint16_t timestamp;
} tooth_logger_s;

typedef struct __attribute__ ((packed)) {
	// the whole order of all packet bytes is reversed, not just the 'endian-swap' integers
	uint32_t timestamp;
	// unfortunately all these fields are required by TS...
	bool priLevel : 1;
	bool secLevel : 1;
	bool isTDC : 1;
	bool sync : 1;
	bool coil : 1;
	bool injector : 1;
} composite_logger_s;

/**
 * Engine idles around 20Hz and revs up to 140Hz, at 60/2 and 8 cylinders we have about 20Khz events
 * If we can read buffer at 50Hz we want buffer to be about 400 elements.
 */
static composite_logger_s buffer[COMPOSITE_PACKET_COUNT] CCM_OPTIONAL;
static size_t NextIdx = 0;
static volatile bool ToothLoggerEnabled = false;

static uint32_t lastEdgeTimestamp = 0;

static bool trigger1 = false;
static bool trigger2 = false;
// any coil, all coils thrown together
static bool coil = false;
// same about injectors
static bool injector = false;

int getCompositeRecordCount() {
	return NextIdx;
}


#if EFI_UNIT_TEST
#include "logicdata.h"
int copyCompositeEvents(CompositeEvent *events) {
	for (int i = 0;i < NextIdx;i++) {
		CompositeEvent *event = &events[i];
		event->timestamp = SWAP_UINT32(buffer[i].timestamp);
		event->primaryTrigger = buffer[i].priLevel;
		event->secondaryTrigger = buffer[i].secLevel;
		event->isTDC = buffer[i].isTDC;
		event->sync = buffer[i].sync;
		event->coil = buffer[i].coil;
		event->injector = buffer[i].injector;
	}
	return NextIdx;
}

#endif // EFI_UNIT_TEST

static void SetNextCompositeEntry(efitick_t timestamp, bool trigger1, bool trigger2,
		bool isTDC DECLARE_ENGINE_PARAMETER_SUFFIX) {
	uint32_t nowUs = NT2US(timestamp);
	// TS uses big endian, grumble
	buffer[NextIdx].timestamp = SWAP_UINT32(nowUs);
	buffer[NextIdx].priLevel = trigger1;
	buffer[NextIdx].secLevel = trigger2;
	buffer[NextIdx].isTDC = isTDC;
	buffer[NextIdx].sync = engine->triggerCentral.triggerState.shaft_is_synchronized;
	buffer[NextIdx].coil = coil;
	buffer[NextIdx].injector = injector;

	NextIdx++;

	static_assert(sizeof(composite_logger_s) == COMPOSITE_PACKET_SIZE, "composite packet size");

	// If we hit the end, loop
	if (NextIdx >= sizeof(buffer) / sizeof(buffer[0])) {
		NextIdx = 0;
	}
}

void LogTriggerTooth(trigger_event_e tooth, efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	ScopePerf perf(PE::LogTriggerTooth);
	// bail if we aren't enabled
	if (!ToothLoggerEnabled) {
		return;
	}

	// Don't log at significant engine speed
	if (engine->rpmCalculator.getRpm() > 4000) {
		return;
	}

/*
		// We currently only support the primary trigger falling edge
    	// (this is the edge that VR sensors are accurate on)
    	// Since VR sensors are the most useful case here, this is okay for now.
    	if (tooth != SHAFT_PRIMARY_FALLING) {
    		return;
    	}

    	uint32_t nowUs = NT2US(timestamp);
    	// 10us per LSB - this gives plenty of accuracy, yet fits 655.35 ms in to a uint16
    	uint16_t delta = static_cast<uint16_t>((nowUs - lastEdgeTimestamp) / 10);
    	lastEdgeTimestamp = nowUs;

    	SetNextEntry(delta);
*/

	switch (tooth) {
	case SHAFT_PRIMARY_FALLING:
		trigger1 = false;
		break;
	case SHAFT_PRIMARY_RISING:
		trigger1 = true;
		break;
	case SHAFT_SECONDARY_FALLING:
		trigger2 = false;
		break;
	case SHAFT_SECONDARY_RISING:
		trigger2 = true;
		break;
	default:
		break;
	}

	SetNextCompositeEntry(timestamp, trigger1, trigger2, false PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerTopDeadCenter(efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	// bail if we aren't enabled
	if (!ToothLoggerEnabled) {
		return;
	}
	SetNextCompositeEntry(timestamp, trigger1, trigger2, true PASS_ENGINE_PARAMETER_SUFFIX);
	SetNextCompositeEntry(timestamp + 10, trigger1, trigger2, false PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerCoilState(efitick_t timestamp, bool state DECLARE_ENGINE_PARAMETER_SUFFIX) {
	if (!ToothLoggerEnabled) {
		return;
	}
	coil = state;
	SetNextCompositeEntry(timestamp, trigger1, trigger2, false PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerInjectorState(efitick_t timestamp, bool state DECLARE_ENGINE_PARAMETER_SUFFIX) {
	if (!ToothLoggerEnabled) {
		return;
	}
	injector = state;
	SetNextCompositeEntry(timestamp, trigger1, trigger2, false PASS_ENGINE_PARAMETER_SUFFIX);
}

void EnableToothLogger() {
	// Clear the buffer
	memset(buffer, 0, sizeof(buffer));

	// Reset the last edge to now - this prevents the first edge logged from being bogus
	lastEdgeTimestamp = getTimeNowUs();

	// Reset write index
	NextIdx = 0;

	// Enable logging of edges as they come
	ToothLoggerEnabled = true;

#if EFI_TUNER_STUDIO
	// Tell TS that we're ready for it to read out the log
	// nb: this is a lie, as we may not have written anything
	// yet.  However, we can let it continuously read out the buffer
	// as we update it, which looks pretty nice.
	tsOutputChannels.toothLogReady = true;
#endif // EFI_TUNER_STUDIO
}

void EnableToothLoggerIfNotEnabled() {
	if (!ToothLoggerEnabled) {
		EnableToothLogger();
	}
}

void DisableToothLogger() {
	ToothLoggerEnabled = false;
#if EFI_TUNER_STUDIO
	tsOutputChannels.toothLogReady = false;
#endif // EFI_TUNER_STUDIO
}

ToothLoggerBuffer GetToothLoggerBuffer() {
	return { reinterpret_cast<uint8_t*>(buffer), sizeof(buffer) };
}

#endif /* EFI_TOOTH_LOGGER */
