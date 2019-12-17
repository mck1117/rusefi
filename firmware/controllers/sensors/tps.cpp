/**
 * @author Andrey Belomutskiy, (c) 2012-2019
 */
#include "engine.h"
#include "tps.h"
#include "interpolation.h"
#include "adc_inputs.h"
#if EFI_PROD_CODE
#include "settings.h"
#endif /* EFI_PROD_CODE */

EXTERN_ENGINE;

/**
 * set mock_pedal_position X
 * See also directPwmValue
 */
percent_t mockPedalPosition = MOCK_UNDEFINED;

#if !EFI_PROD_CODE
/**
 * this allows unit tests to simulate TPS position
 */
void setMockTpsAdc(percent_t tpsPosition DECLARE_ENGINE_PARAMETER_SUFFIX) {
	engine->mockTpsAdcValue = TPS_TS_CONVERSION * tpsPosition;
}

void setMockTpsValue(percent_t tpsPosition DECLARE_ENGINE_PARAMETER_SUFFIX) {
	engine->mockTpsValue = tpsPosition;
}
#endif /* EFI_PROD_CODE */

// see also setMockThrottlePedalSensorVoltage
void setMockThrottlePedalPosition(percent_t value DECLARE_ENGINE_PARAMETER_SUFFIX) {
	mockPedalPosition = value;
}

/**
 * We are using one instance for read and another for modification, this is how we get lock-free thread-safety
 *
 */
static tps_roc_s states[2];

// todo if TPS_FAST_ADC
//int tpsFastAdc = 0;

static volatile int tpsRocIndex = 0;

/**
 * this method is lock-free thread-safe if invoked only from one thread
 */
void saveTpsState(efitimeus_t now, float curValue) {
	int tpsNextIndex = (tpsRocIndex + 1) % 2;
	tps_roc_s *cur = &states[tpsRocIndex];
	tps_roc_s *next = &states[tpsNextIndex];

	next->prevTime = cur->curTime;
	next->prevValue = cur->curValue;
	next->curTime = now;
	next->curValue = curValue;

	//int diffSysticks = overflowDiff(now, cur->curTime);
	float diffSeconds = 0;// TODO: do we need this? diffSysticks * 1.0 / CH_FREQUENCY;
	next->rateOfChange = (curValue - cur->curValue) / diffSeconds;

	// here we update volatile index
	tpsRocIndex = tpsNextIndex;
}

/**
 * this read-only method is lock-free thread-safe
 */
float getTpsRateOfChange(void) {
	return states[tpsRocIndex].rateOfChange;
}

/*
 * Return current TPS position based on configured ADC levels, and adc
 *
 * */
percent_t getTpsValue(int index, int adc DECLARE_ENGINE_PARAMETER_SUFFIX) {

	DISPLAY_STATE(Engine)
	DISPLAY_TAG(TPS_SECTION);
	DISPLAY_SENSOR(TPS)
	DISPLAY_TEXT(EOL)


	DISPLAY_TEXT(Analog_MCU_reads);
	engine->engineState.currentTpsAdc = adc;
#if !EFI_UNIT_TEST
	engine->engineState.DISPLAY_FIELD(tpsVoltageMCU) = adcToVolts(adc);
#endif
	DISPLAY_TEXT(Volts);
	DISPLAY_TEXT(from_pin) DISPLAY(DISPLAY_CONFIG(tps1_1AdcChannel))
	DISPLAY_TEXT(EOL);

	DISPLAY_TEXT(Analog_ECU_reads);
	engine->engineState.DISPLAY_FIELD(tpsVoltageBoard) =
	DISPLAY_TEXT(Rdivider) engine->engineState.tpsVoltageMCU * CONFIG(DISPLAY_CONFIG(analogInputDividerCoefficient));
	DISPLAY_TEXT(EOL);


	if (engineConfiguration->tpsMin == engineConfiguration->tpsMax) {
		warning(CUSTOM_INVALID_TPS_SETTING, "Invalid TPS configuration: same value %d", engineConfiguration->tpsMin);
		return NAN;
	}

	DISPLAY_TEXT(Current_ADC)
	DISPLAY(DISPLAY_FIELD(currentTpsAdc))
	DISPLAY_TEXT(interpolate_between)

	DISPLAY(DISPLAY_CONFIG(tpsMax))
	DISPLAY_TEXT(and)
	DISPLAY(DISPLAY_CONFIG(tpsMin))

	int tpsMax = index == 0 ? CONFIG(tpsMax) : CONFIG(tps2Max);
	int tpsMin = index == 0 ? CONFIG(tpsMin) : CONFIG(tps2Min);

	float result = interpolateMsg("TPS", TPS_TS_CONVERSION * tpsMax, 100,
			TPS_TS_CONVERSION * tpsMin, 0, adc);
	if (result < engineConfiguration->tpsErrorDetectionTooLow) {
#if EFI_PROD_CODE
		// too much noise with simulator
		warning(OBD_Throttle_Position_Sensor_Circuit_Malfunction, "TPS too low: %.2f", result);
#endif /* EFI_PROD_CODE */
	}
	if (result > engineConfiguration->tpsErrorDetectionTooHigh) {
#if EFI_PROD_CODE
		// too much noise with simulator
		warning(OBD_Throttle_Position_Sensor_Range_Performance_Problem, "TPS too high: %.2f", result);
#endif /* EFI_PROD_CODE */
	}

	// this would put the value into the 0-100 range
	return maxF(0, minF(100, result));
}

/*
 * Return voltage on TPS AND channel
 * */
float getTPSVoltage(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	return getVoltageDivided("tps", engineConfiguration->tps1_1AdcChannel PASS_ENGINE_PARAMETER_SUFFIX);
}

/*
 * Return TPS ADC readings.
 * We need ADC value because TunerStudio has a nice TPS configuration wizard, and this wizard
 * wants a TPS value.
 * @param index [0, ETB_COUNT)
 */
int getTPS12bitAdc(int index DECLARE_ENGINE_PARAMETER_SUFFIX) {
#if !EFI_PROD_CODE
	if (engine->mockTpsAdcValue != MOCK_UNDEFINED) {
		return engine->mockTpsAdcValue;
	}
#endif
	if (engineConfiguration->tps1_1AdcChannel == EFI_ADC_NONE)
		return -1;
#if EFI_PROD_CODE

	if (index == 0) {
		return getAdcValue("tps10", engineConfiguration->tps1_1AdcChannel);
	} else {
		return getAdcValue("tps20", engineConfiguration->tps2_1AdcChannel);
	}
	//	return tpsFastAdc / 4;
#else
	return 0;
#endif /* EFI_PROD_CODE */
}

void grabTPSIsClosed() {
#if EFI_PROD_CODE
	printTPSInfo();
	engineConfiguration->tpsMin = getTPS10bitAdc();
	printTPSInfo();
#endif /* EFI_PROD_CODE */
}

void grabTPSIsWideOpen() {
#if EFI_PROD_CODE
	printTPSInfo();
	engineConfiguration->tpsMax = getTPS10bitAdc();
	printTPSInfo();
#endif /* EFI_PROD_CODE */
}

void grabPedalIsUp() {
#if EFI_PROD_CODE
	float voltage = getVoltageDivided("pPS", engineConfiguration->throttlePedalPositionAdcChannel PASS_ENGINE_PARAMETER_SUFFIX);
	engineConfiguration->throttlePedalUpVoltage = voltage;
	printTPSInfo();
#endif /* EFI_PROD_CODE */
}

void grabPedalIsWideOpen() {
#if EFI_PROD_CODE
	float voltage = getVoltageDivided("pPS", engineConfiguration->throttlePedalPositionAdcChannel PASS_ENGINE_PARAMETER_SUFFIX);
	engineConfiguration->throttlePedalWOTVoltage = voltage;
	printTPSInfo();
#endif /* EFI_PROD_CODE */
}

/**
 * @brief Position on physical primary TPS
 */
static percent_t getPrimaryRawTPS(int index DECLARE_ENGINE_PARAMETER_SUFFIX) {
	int adcValue = getTPS12bitAdc(index PASS_ENGINE_PARAMETER_SUFFIX);
	percent_t tpsValue = getTpsValue(index, adcValue PASS_ENGINE_PARAMETER_SUFFIX);
	return tpsValue;
}

#define NO_TPS_MAGIC_VALUE 66.611

bool hasPedalPositionSensor(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	return engineConfiguration->throttlePedalPositionAdcChannel != EFI_ADC_NONE;
}

bool hasTps2(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	return engineConfiguration->tps2_1AdcChannel != EFI_ADC_NONE;
}

percent_t getPedalPosition(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	if (mockPedalPosition != MOCK_UNDEFINED) {
		return mockPedalPosition;
	}
	DISPLAY_TAG(PEDAL_SECTION);
	DISPLAY_TEXT(Analog_MCU_reads);

	float voltage = getVoltageDivided("pPS", CONFIG(DISPLAY_CONFIG(throttlePedalPositionAdcChannel)) PASS_ENGINE_PARAMETER_SUFFIX);
	percent_t result = interpolateMsg("pedal", engineConfiguration->throttlePedalUpVoltage, 0, engineConfiguration->throttlePedalWOTVoltage, 100, voltage);

	// this would put the value into the 0-100 range
	return maxF(0, minF(100, result));
}

bool hasTpsSensor(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	return engineConfiguration->tps1_1AdcChannel != EFI_ADC_NONE;
}

percent_t getTPSWithIndex(int index DECLARE_ENGINE_PARAMETER_SUFFIX) {
#if !EFI_PROD_CODE
	if (!cisnan(engine->mockTpsValue)) {
		return engine->mockTpsValue;
	}
#endif /* EFI_PROD_CODE */
	if (!hasTpsSensor(PASS_ENGINE_PARAMETER_SIGNATURE))
		return NO_TPS_MAGIC_VALUE;
	// todo: if (config->isDualTps)
	// todo: blah blah
	// todo: if two TPS do not match - show OBD code via malfunction_central.c

	return getPrimaryRawTPS(index PASS_ENGINE_PARAMETER_SUFFIX);
}

percent_t getTPS(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	return getTPSWithIndex(0 PASS_ENGINE_PARAMETER_SUFFIX);
}

void setBosch0280750009(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	// see http://rusefi.com/wiki/index.php?title=Vehicle:VW_Passat_2002_1.8
	engineConfiguration->tpsMin = 159;
	engineConfiguration->tpsMax = 957;

	// todo: add 2nd TPS sensor calibration
}

int convertVoltageTo10bitADC(float voltage) {
	// divided by 2 because of voltage divider, then converted into 10bit ADC value (TunerStudio format)
	return (int) (voltage / 2 * 1024 / 3.3);
}
