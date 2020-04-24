/**
 * @file	map_averaging.cpp
 *
 * In order to have best MAP estimate possible, we real MAP value at a relatively high frequency
 * and average the value within a specified angle position window for each cylinder
 *
 * @date Dec 11, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 *
 * This file is part of rusEfi - see http://rusefi.com
 *
 * rusEfi is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * rusEfi is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "global.h"
#include "os_access.h"

#include "map.h"

#if EFI_MAP_AVERAGING

#include "map_averaging.h"
#include "trigger_central.h"
#include "adc_inputs.h"
#include "allsensors.h"
#include "engine_configuration.h"
#include "interpolation.h"
#include "engine.h"
#include "engine_math.h"
#include "perf_trace.h"
#include "thread_controller.h"

#if EFI_SENSOR_CHART
#include "sensor_chart.h"
#endif /* EFI_SENSOR_CHART */

#define FAST_MAP_CHART_SKIP_FACTOR 16

static Logging *logger;
/**
 * this instance does not have a real physical pin - it's only used for engine sniffer
 */
static NamedOutputPin mapAveragingPin("map");

/**
 * Running counter of measurements per revolution
 */
static volatile int measurementsPerRevolutionCounter = 0;
/**
 * Number of measurements in previous shaft revolution
 */
static volatile int measurementsPerRevolution = 0;

// allow a bit more smoothing
#define MAX_MAP_BUFFER_LENGTH (INJECTION_PIN_COUNT * 2)
// in MAP units, not voltage!
int mapMinBufferLength = 0;
// we need this 'NO_VALUE_YET' to properly handle transition from engine not running to engine already running
// but prior to first processed result
#define NO_VALUE_YET -100
// this is 'minimal averaged' MAP within avegaging window
static float currentPressure = NO_VALUE_YET;

EXTERN_ENGINE;

static scheduling_s startTimer[INJECTION_PIN_COUNT][2];


SEMAPHORE_DECL(map_avg_start, 0);

static void startAveraging(void *arg) {
	(void) arg;

	ScopePerf perf(PE::Temporary1);

	syssts_t sts = chSysGetStatusAndLockX();
	chSemSignalI(&map_avg_start);

	/*if (!port_is_isr_context()) {
		chSchRescheduleS();
	}*/

	chSysRestoreStatusX(sts);
}


static ADCConversionGroup adcConvGroup = { FALSE, 1, nullptr, nullptr,
	0,
	ADC_CR2_SWSTART,
	0, // sample times for channels 10...18
	ADC_SMPR2_SMP_AN9(ADC_SAMPLE_84),

	0,	// htr
	0,	// ltr

	0,	// sqr1
	0,	// sqr2
	ADC_SQR3_SQ1_N(ADC_CHANNEL_IN9)	// sqr3 - vbatt is on pf3 = adc9
};

struct MapAverager : public ThreadController<512> {
	adcsample_t m_samples[256];

	MapAverager() : ThreadController("map avg", NORMALPRIO + 20){}

	void ThreadTask() override {
		while (true) {
			// Timeout in 10ms = 100hz
			msg_t msg = chSemWaitTimeout(&map_avg_start, TIME_MS2I(1000));

			ScopePerf perf(PE::Temporary2);

			mapAveragingPin.setValue(1);

			float sampleDurationUs;

			//if (msg == MSG_TIMEOUT) {
				// Sample for 1ms in case of timeout - low engine speed
			//	sampleDurationUs = 1e3f;
			//} else {
				sampleDurationUs = engine->rpmCalculator.oneDegreeUs * ENGINE(engineState.mapAveragingDuration);
			//}

			(void)sampleDurationUs;

			constexpr float sampleRate = 48681.54;
			float sampleDurationSec = sampleDurationUs / 1e6f;

			int sampleCount = sampleDurationSec * sampleRate;

			if (sampleCount != 1) {
				// make sure it's an even number
				sampleCount &= 0xFFFFFFFE;
			}

			int samplesRemaining = sampleCount;

			uint32_t sum = 0;

			while (samplesRemaining > 0) {
				size_t batchSize = minI(samplesRemaining, efi::size(m_samples));
				sum += sampleBatch(batchSize);
				samplesRemaining -= batchSize;
			}

			float average = (float)sum / sampleCount;

			mapAveragingPin.setValue(0);
		}
	}

	uint32_t sampleBatch(size_t sampleCount) {
		ScopePerf perf(PE::Temporary3);

		{
			ScopePerf perf(PE::Temporary3);
			adcConvert(&ADCD2, &adcConvGroup, m_samples, sampleCount);
		}

		uint32_t sum = 0;

		for (size_t i = 0; i < sampleCount; i++)
		{
			sum += m_samples[i];
		}

		return sum;
	}
};

#if EFI_TUNER_STUDIO
void postMapState(TunerStudioOutputChannels *tsOutputChannels) {
}
#endif /* EFI_TUNER_STUDIO */

void refreshMapAveragingPreCalc(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	int rpm = GET_RPM_VALUE;
	if (isValidRpm(rpm)) {
		MAP_sensor_config_s * c = &engineConfiguration->map;
		angle_t start = interpolate2d("mapa", rpm, c->samplingAngleBins, c->samplingAngle);
		efiAssertVoid(CUSTOM_ERR_MAP_START_ASSERT, !cisnan(start), "start");

		angle_t offsetAngle = TRIGGER_WAVEFORM(eventAngles[CONFIG(mapAveragingSchedulingAtIndex)]);
		efiAssertVoid(CUSTOM_ERR_MAP_AVG_OFFSET, !cisnan(offsetAngle), "offsetAngle");

		for (int i = 0; i < engineConfiguration->specs.cylindersCount; i++) {
			angle_t cylinderOffset = getEngineCycle(engine->getOperationMode(PASS_ENGINE_PARAMETER_SIGNATURE)) * i / engineConfiguration->specs.cylindersCount;
			efiAssertVoid(CUSTOM_ERR_MAP_CYL_OFFSET, !cisnan(cylinderOffset), "cylinderOffset");
			// part of this formula related to specific cylinder offset is never changing - we can
			// move the loop into start-up calculation and not have this loop as part of periodic calculation
			// todo: change the logic as described above in order to reduce periodic CPU usage?
			float cylinderStart = start + cylinderOffset - offsetAngle + tdcPosition();
			fixAngle(cylinderStart, "cylinderStart", CUSTOM_ERR_6562);
			engine->engineState.mapAveragingStart[i] = cylinderStart;
		}
		engine->engineState.mapAveragingDuration = interpolate2d("samp", rpm, c->samplingWindowBins, c->samplingWindow);
	} else {
		for (int i = 0; i < engineConfiguration->specs.cylindersCount; i++) {
			engine->engineState.mapAveragingStart[i] = NAN;
		}
		engine->engineState.mapAveragingDuration = NAN;
	}

}

/**
 * Shaft Position callback used to schedule start and end of MAP averaging
 */
static void mapAveragingTriggerCallback(trigger_event_e ckpEventType,
		uint32_t index, efitick_t edgeTimestamp DECLARE_ENGINE_PARAMETER_SUFFIX) {

	ScopePerf perf(PE::MapAveragingTriggerCallback);
	
#if EFI_ENGINE_CONTROL
	// this callback is invoked on interrupt thread
	UNUSED(ckpEventType);
	if (index != (uint32_t)CONFIG(mapAveragingSchedulingAtIndex))
		return;

	int rpm = GET_RPM_VALUE;
	if (!isValidRpm(rpm)) {
		return;
	}

	measurementsPerRevolution = measurementsPerRevolutionCounter;
	measurementsPerRevolutionCounter = 0;

	int samplingCount = CONFIG(measureMapOnlyInOneCylinder) ? 1 : engineConfiguration->specs.cylindersCount;

	for (int i = 0; i < samplingCount; i++) {
		angle_t samplingStart = ENGINE(engineState.mapAveragingStart[i]);

		angle_t samplingDuration = ENGINE(engineState.mapAveragingDuration);
		assertAngleRange(samplingDuration, "samplingDuration", CUSTOM_ERR_6563);
		if (samplingDuration <= 0) {
			warning(CUSTOM_MAP_ANGLE_PARAM, "map sampling angle should be positive");
			return;
		}

		angle_t samplingEnd = samplingStart + samplingDuration;

		if (cisnan(samplingEnd)) {
			// todo: when would this happen?
			warning(CUSTOM_ERR_6549, "no map angles");
			return;
		}


		fixAngle(samplingEnd, "samplingEnd", CUSTOM_ERR_6563);
		// only if value is already prepared
		int structIndex = getRevolutionCounter() % 2;
		// at the moment we schedule based on time prediction based on current RPM and angle
		// we are loosing precision in case of changing RPM - the further away is the event the worse is precision
		// todo: schedule this based on closest trigger event, same as ignition works
		scheduleByAngle(&startTimer[i][structIndex], edgeTimestamp, samplingStart,
				startAveraging PASS_ENGINE_PARAMETER_SUFFIX);
	}
#endif
}

static void showMapStats(void) {
	scheduleMsg(logger, "per revolution %d", measurementsPerRevolution);
}

#if EFI_PROD_CODE

/**
 * Because of MAP window averaging, MAP is only available while engine is spinning
 * @return Manifold Absolute Pressure, in kPa
 */
float getMap(void) {
	if (engineConfiguration->hasFrequencyReportingMapSensor) {
		return getRawMap();
	}

#if EFI_ANALOG_SENSORS
	if (!isValidRpm(GET_RPM_VALUE) || currentPressure == NO_VALUE_YET)
		return validateMap(getRawMap()); // maybe return NaN in case of stopped engine?
	return validateMap(currentPressure);
#else
	return 100;
#endif
}
#endif /* EFI_PROD_CODE */

MapAverager avgThread;

void initMapAveraging(Logging *sharedLogger DECLARE_ENGINE_PARAMETER_SUFFIX) {
	logger = sharedLogger;

#if EFI_SHAFT_POSITION_INPUT
	addTriggerEventListener(&mapAveragingTriggerCallback, "MAP averaging", engine);
#endif /* EFI_SHAFT_POSITION_INPUT */

#if !EFI_UNIT_TEST
	addConsoleAction("faststat", showMapStats);
#endif /* EFI_UNIT_TEST */

	avgThread.Start();
}

#else

#if EFI_PROD_CODE

float getMap(void) {
#if EFI_ANALOG_SENSORS
	return getRawMap();
#else
	return NAN;
#endif /* EFI_ANALOG_SENSORS */
}
#endif /* EFI_PROD_CODE */

#endif /* EFI_MAP_AVERAGING */
