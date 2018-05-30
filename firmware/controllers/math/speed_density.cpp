/**
 * @file	speed_density.cpp
 *
 * See http://rusefi.com/wiki/index.php?title=Manual:Software:Fuel_Control#Speed_Density for details
 *
 * @date May 29, 2014
 * @author Andrey Belomutskiy, (c) 2012-2018
 */

#include "main.h"
#include "speed_density.h"
#include "interpolation.h"
#include "rpm_calculator.h"
#include "engine_math.h"
#include "engine_state.h"

EXTERN_ENGINE;

fuel_Map3D_t veMap("VE");
fuel_Map3D_t ve2Map("VE2");
afr_Map3D_t afrMap("AFR", 1.0 / AFR_STORAGE_MULT);
baroCorr_Map3D_t baroCorrMap("baro");

static float tCharge(float massFlowRateGPerS)
{
	if(massFlowRateGPerS < 1)
	{
		return 0.1317;
	}

	return 1.3867f / massFlowRateGPerS + 0.1317f;
}

static float tChargeMathK(float airTempC, float coolantTempC, float charge_xfer_eff)
{
	return coolantTempC * charge_xfer_eff + airTempC * (1 - charge_xfer_eff) + 273.15;
}

/**
 * is J/g*K
 */
#define GAS_R 0.28705

float getCycleAirMass(engine_configuration_s *engineConfiguration, float VE, float MAP, float tempK) {
	// todo: pre-calculate cylinder displacement to save one division
	float cylinderDisplacement = engineConfiguration->specs.displacement;
	return (cylinderDisplacement * VE * MAP) / (GAS_R * tempK);
}

float getCylinderAirMass(engine_configuration_s *engineConfiguration, float VE, float MAP, float tempK) {
	return getCycleAirMass(engineConfiguration, VE, MAP, tempK) / engineConfiguration->specs.cylindersCount;
}

/**
 * @return per cylinder injection time, in seconds
 */
float sdMath(engine_configuration_s *engineConfiguration, float airMass, float AFR) {

	/**
	 * todo: pre-calculate gramm/second injector flow to save one multiplication
	 * open question if that's needed since that's just a multiplication
	 */
	float injectorFlowRate = cc_minute_to_gramm_second(engineConfiguration->injector.flow);
	/**
	 * injection_pulse_duration = fuel_mass / injector_flow
	 * fuel_mass = air_mass / target_afr
	 *
	 * injection_pulse_duration = (air_mass / target_afr) / injector_flow
	 */
	return airMass / (AFR * injectorFlowRate);
}

EXTERN_ENGINE;


float sdChargeTemp = 0;

/**
 * @return per cylinder injection time, in Milliseconds
 */
floatms_t getSpeedDensityFuel(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	float map = getMap();
	efiAssert(!cisnan(map), "NaN map", 0);

	float adjustedMap = map + engine->engineLoadAccelEnrichment.getEngineLoadEnrichment(PASS_ENGINE_PARAMETER_SIGNATURE);
	efiAssert(!cisnan(adjustedMap), "NaN adjustedMap", 0);
	
	int rpm = ENGINE(rpmCalculator).getRpm(PASS_ENGINE_PARAMETER_SIGNATURE);
	float coolantC = ENGINE(sensors.clt);
	float intakeC = ENGINE(sensors.iat);

	if(cisnan(coolantC) || cisnan(intakeC))
	{
		warning(CUSTOM_ERR_TCHARGE_NOT_READY2, "tChargeK not ready"); // this would happen before we have CLT reading for example
		return 0;
	}

	// Initial estimate at 40 deg C, since that's a reasonable starting point
	//float airMass = getCycleAirMass(engineConfiguration, ENGINE(engineState.currentVE), adjustedMap, 273 + 40);
	float airMass = 0;

	float chargeTemp = 0;

	// Iterate to converge to a tCharge solution
	for(int i = 0; i < 3; i++)
	{
		float massFlow = airMass * rpm / 2 / 60;
		float xfer_eff = tCharge(massFlow);
		chargeTemp = tChargeMathK(intakeC, coolantC, xfer_eff);
		airMass = getCycleAirMass(engineConfiguration, ENGINE(engineState.currentVE), adjustedMap, chargeTemp);
	}

	sdChargeTemp = chargeTemp;

	// Convert to per-cylinder air mass
	airMass = airMass / engineConfiguration->specs.cylindersCount;

	efiAssert(!cisnan(airMass), "NaN airMass", 0);
#if EFI_PRINTF_FUEL_DETAILS || defined(__DOXYGEN__)
	printf("map=%.2f adjustedMap=%.2f airMass=%.2f\t\n",
			map, adjustedMap, engine->engineState.airMass);
#endif /*EFI_PRINTF_FUEL_DETAILS */


	engine->engineState.airMass = airMass;
	return sdMath(engineConfiguration, airMass, ENGINE(engineState.targetAFR)) * 1000;
}

static const baro_corr_table_t default_baro_corr = {
		{1.141, 1.086, 1.039, 1},
		{1.141, 1.086, 1.039, 1},
		{1.141, 1.086, 1.039, 1},
		{1.141, 1.086, 1.039, 1}
};

void setDefaultVETable(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	setRpmTableBin(config->veRpmBins, FUEL_RPM_COUNT);
	veMap.setAll(80);

//	setRpmTableBin(engineConfiguration->ve2RpmBins, FUEL_RPM_COUNT);
//	setLinearCurve(engineConfiguration->ve2LoadBins, FUEL_LOAD_COUNT, 10, 300, 1);
//	ve2Map.setAll(0.81);

	setRpmTableBin(config->afrRpmBins, FUEL_RPM_COUNT);
	afrMap.setAll(14.7);

	setRpmTableBin(engineConfiguration->baroCorrRpmBins, BARO_CORR_SIZE);
	setLinearCurve(engineConfiguration->baroCorrPressureBins, BARO_CORR_SIZE, 75, 105, 1);
	memcpy(engineConfiguration->baroCorrTable, default_baro_corr, sizeof(default_baro_corr));
}

void initSpeedDensity(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	veMap.init(config->veTable, config->veLoadBins, config->veRpmBins);
//	ve2Map.init(engineConfiguration->ve2Table, engineConfiguration->ve2LoadBins, engineConfiguration->ve2RpmBins);
	afrMap.init(config->afrTable, config->afrLoadBins, config->afrRpmBins);
	baroCorrMap.init(engineConfiguration->baroCorrTable, engineConfiguration->baroCorrPressureBins, engineConfiguration->baroCorrRpmBins);
}
