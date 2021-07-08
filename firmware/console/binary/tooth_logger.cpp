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
	bool trigger : 1;
	bool sync : 1;
	bool coil : 1;
	bool injector : 1;
} composite_logger_s;

static_assert(sizeof(composite_logger_s) == COMPOSITE_PACKET_SIZE, "composite packet size");

/**
 * Engine idles around 20Hz and revs up to 140Hz, at 60/2 and 8 cylinders we have about 20Khz events
 * If we can read buffer at 50Hz we want buffer to be about 400 elements.
 */
#define BUFFER_SIZE (COMPOSITE_PACKET_COUNT / 2)
static composite_logger_s buffers[2][BUFFER_SIZE] CCM_OPTIONAL;
static composite_logger_s* frontBuffer = buffers[0];
static composite_logger_s* backBuffer = buffers[1];
static size_t nextIdx = 0;
static bool ToothLoggerEnabled = false;
static uint32_t lastEdgeTimestamp = 0;

static bool currentTrigger1 = false;
static bool currentTrigger2 = false;
static bool currentTdc = false;
// any coil, all coils thrown together
static bool currentCoilState = false;
// same about injectors
static bool currentInjectorState = false;

int getCompositeRecordCount() {
	return nextIdx;
}


#if EFI_UNIT_TEST
#include "logicdata.h"
int copyCompositeEvents(CompositeEvent *events) {
	for (size_t i = 0; i < NextIdx; i++) {
		CompositeEvent *event = &events[i];
		event->timestamp = SWAP_UINT32(buffer[i].timestamp);
		event->primaryTrigger = buffer[i].priLevel;
		event->secondaryTrigger = buffer[i].secLevel;
		event->isTDC = buffer[i].trigger;
		event->sync = buffer[i].sync;
		event->coil = buffer[i].coil;
		event->injector = buffer[i].injector;
	}
	return NextIdx;
}

#endif // EFI_UNIT_TEST

static efitick_t lastReadyTime = 0;

static void queueDataForRead(efitick_t timestamp) {
	// Signal that there are now events in the buffer available to read
	tsOutputChannels.toothLogReady = true;
	lastReadyTime = timestamp;

	// Swap to the back buffer
	auto temp = frontBuffer;
	frontBuffer = backBuffer;
	backBuffer = temp;
	backSize = nextIdx;

	// Reset front buffer
	nextIdx = 0;
}

static void SetNextCompositeEntry(efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	uint32_t nowUs = NT2US(timestamp);

	if (nextIdx >= BUFFER_SIZE) {
		// Buffer full, nothing to do.
		return;
	}

	// Claim an entry, and write to it under lock
	{
		chibios_rt::CriticalSectionLocker csl;

		// TS uses big endian, grumble
		auto& entry = frontBuffer[nextIdx++];
		entry.timestamp = SWAP_UINT32(nowUs);
		entry.priLevel = currentTrigger1;
		entry.secLevel = currentTrigger2;
		entry.trigger = currentTdc;
		entry.sync = engine->triggerCentral.triggerState.getShaftSynchronized();
		entry.coil = currentCoilState;
		entry.injector = currentInjectorState;

		if (nextIdx >= BUFFER_SIZE) {
			queueDataForRead(timestamp);
		}
	}

	// If it's been a long time since the last flush, force a flush so the user sees *something*
	if (timestamp - lastReadyTime > MS2NT(5000)) {
		queueDataForRead(timestamp);
	}
}

void LogTriggerTooth(trigger_event_e tooth, efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	// bail if we aren't enabled
	if (!ToothLoggerEnabled) {
		return;
	}

	// Don't log at significant engine speed
	if (engine->rpmCalculator.getRpm() > 4000) {
		return;
	}

	ScopePerf perf(PE::LogTriggerTooth);

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
		currentTrigger1 = false;
		break;
	case SHAFT_PRIMARY_RISING:
		currentTrigger1 = true;
		break;
	case SHAFT_SECONDARY_FALLING:
		currentTrigger2 = false;
		break;
	case SHAFT_SECONDARY_RISING:
		currentTrigger2 = true;
		break;
// major hack to get most value of limited logic data write
#if EFI_UNIT_TEST
	case SHAFT_3RD_FALLING:
		currentCoilState = false;
		break;
	case SHAFT_3RD_RISING:
		currentCoilState = true;
		break;
#endif
	default:
		break;
	}

	SetNextCompositeEntry(timestamp PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerTopDeadCenter(efitick_t timestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {
	// bail if we aren't enabled
	if (!ToothLoggerEnabled) {
		return;
	}
	currentTdc = true;
	SetNextCompositeEntry(timestamp PASS_ENGINE_PARAMETER_SUFFIX);
	currentTdc = false;
	SetNextCompositeEntry(timestamp + 10 PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerCoilState(efitick_t timestamp, bool state DECLARE_ENGINE_PARAMETER_SUFFIX) {
	if (!ToothLoggerEnabled) {
		return;
	}
	currentCoilState = state;
	UNUSED(timestamp);
	//SetNextCompositeEntry(timestamp, trigger1, trigger2, trigger PASS_ENGINE_PARAMETER_SUFFIX);
}

void LogTriggerInjectorState(efitick_t timestamp, bool state DECLARE_ENGINE_PARAMETER_SUFFIX) {
	if (!ToothLoggerEnabled) {
		return;
	}
	currentInjectorState = state;
	UNUSED(timestamp);
	//SetNextCompositeEntry(timestamp, trigger1, trigger2, trigger PASS_ENGINE_PARAMETER_SUFFIX);
}

void EnableToothLogger() {
	// Reset the last edge to now - this prevents the first edge logged from being bogus
	lastEdgeTimestamp = getTimeNowUs();

	// Reset write index
	nextIdx = 0;

	// Enable logging of edges as they come
	ToothLoggerEnabled = true;

#if EFI_TUNER_STUDIO
	// Tell TS that we're ready for it to read out the log
	// nb: this is a lie, as we may not have written anything
	// yet.  However, we can let it continuously read out the buffer
	// as we update it, which looks pretty nice.
	tsOutputChannels.toothLogReady = false;
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
	// swap buffers under lock...
	chibios_rt::CriticalSectionLocker csl;

	size_t writtenCount;

	if (backSize > 0) {
		writtenCount = backSize;
		backSize = 0;
	} else {
		// back buffer is empty, swap buffers and return the front buffer
		auto temp = frontBuffer;
		frontBuffer = backBuffer;
		backBuffer = temp;

		auto writtenCount = nextIdx;
		nextIdx = 0;

		tsOutputChannels.toothLogReady = false;
	}

	return { reinterpret_cast<const uint8_t*>(backBuffer), writtenCount };
}

#endif /* EFI_TOOTH_LOGGER */
