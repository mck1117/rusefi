/**
 * @file	test_fuel_map.cpp
 *
 * @date Nov 6, 2013
 * @author Andrey Belomutskiy, (c) 2012-2019
 */

#include "global.h"
#include "engine_configuration.h"
#include "fuel_math.h"
#include "trigger_structure.h"
#include "allsensors.h"
#include "engine_math.h"
#include "trigger_decoder.h"
#include "engine_test_helper.h"
#include "efiGpio.h"
#include "advance_map.h"

extern float testMafValue;

TEST(misc, testMafFuelMath) {
	printf("====================================================================================== testMafFuelMath\r\n");
	WITH_ENGINE_TEST_HELPER(FORD_ASPIRE_1996);

	engineConfiguration->fuelAlgorithm = LM_REAL_MAF;
	engineConfiguration->injector.flow = 200;

	setAfrMap(config->afrTable, 13);

	float fuelMs = getRealMafFuel(300, 6000 PASS_ENGINE_PARAMETER_SUFFIX);
	assertEqualsM("fuelMs", 26.7099, fuelMs);
}

TEST(misc, testFuelMap) {
	printf("====================================================================================== testFuelMap\r\n");

	printf("Setting up FORD_ASPIRE_1996\r\n");
	WITH_ENGINE_TEST_HELPER(FORD_ASPIRE_1996);

	printf("Filling fuel map\r\n");
	for (int k = 0; k < FUEL_LOAD_COUNT; k++) {
		for (int r = 0; r < FUEL_RPM_COUNT; r++) {
			eth.engine.config->fuelTable[k][r] = k * 200 + r;
		}
	}
	for (int i = 0; i < FUEL_LOAD_COUNT; i++)
		eth.engine.config->fuelLoadBins[i] = i;
	for (int i = 0; i < FUEL_RPM_COUNT; i++)
		eth.engine.config->fuelRpmBins[i] = i;

	ASSERT_EQ( 1005,  getBaseTableFuel(5, 5)) << "base fuel table";

	printf("*************************************************** initThermistors\r\n");


	printf("*** getInjectorLag\r\n");
//	engine->engineState.vb
	assertEqualsM("lag", 1.04, getInjectorLag(12 PASS_ENGINE_PARAMETER_SUFFIX));

	for (int i = 0; i < VBAT_INJECTOR_CURVE_SIZE; i++) {
		eth.engine.engineConfigurationPtr->injector.battLagCorrBins[i] = i;
		eth.engine.engineConfigurationPtr->injector.battLagCorr[i] = 0.5 + 2 * i;
	}

	eth.engine.updateSlowSensors(PASS_ENGINE_PARAMETER_SIGNATURE);

	// because all the correction tables are zero
	printf("*************************************************** getRunningFuel 1\r\n");
	eth.engine.periodicFastCallback(PASS_ENGINE_PARAMETER_SIGNATURE);
	float baseFuel = getBaseTableFuel(5, getEngineLoadT(PASS_ENGINE_PARAMETER_SIGNATURE));
	assertEqualsM("base fuel", 5.3679, getRunningFuel(baseFuel PASS_ENGINE_PARAMETER_SUFFIX));

	printf("*************************************************** setting IAT table\r\n");
	for (int i = 0; i < IAT_CURVE_SIZE; i++) {
		eth.engine.config->iatFuelCorrBins[i] = i;
		eth.engine.config->iatFuelCorr[i] = 2 * i;
	}
	eth.engine.config->iatFuelCorr[0] = 2;

	printf("*************************************************** setting CLT table\r\n");
	for (int i = 0; i < CLT_CURVE_SIZE; i++) {
		eth.engine.config->cltFuelCorrBins[i] = i;
		eth.engine.config->cltFuelCorr[i] = 100;
	}

	setFlatInjectorLag(0 PASS_CONFIG_PARAMETER_SUFFIX);

	float iat = getIntakeAirTemperature(PASS_ENGINE_PARAMETER_SIGNATURE);
	ASSERT_FALSE(cisnan(iat));
	float iatCorrection = getIatFuelCorrection(-KELV PASS_ENGINE_PARAMETER_SUFFIX);
	ASSERT_EQ( 2,  iatCorrection) << "IAT";
	engine->sensors.clt = getCoolantTemperature(PASS_ENGINE_PARAMETER_SIGNATURE);
	float cltCorrection = getCltFuelCorrection(PASS_ENGINE_PARAMETER_SIGNATURE);
	ASSERT_EQ( 1,  cltCorrection) << "CLT";
	float injectorLag = getInjectorLag(getVBatt(PASS_ENGINE_PARAMETER_SIGNATURE) PASS_ENGINE_PARAMETER_SUFFIX);
	ASSERT_EQ( 0,  injectorLag) << "injectorLag";

	testMafValue = 5;

	// 1005 * 2 for IAT correction
	printf("*************************************************** getRunningFuel 2\r\n");
	eth.engine.periodicFastCallback(PASS_ENGINE_PARAMETER_SIGNATURE);
	baseFuel = getBaseTableFuel(5, getEngineLoadT(PASS_ENGINE_PARAMETER_SIGNATURE));
	ASSERT_EQ( 30150,  getRunningFuel(baseFuel PASS_ENGINE_PARAMETER_SUFFIX)) << "v1";

	testMafValue = 0;

	engineConfiguration->cranking.baseFuel = 4;

	printf("*************************************************** getStartingFuel\r\n");
	// NAN in case we have issues with the CLT sensor
	ASSERT_EQ( 4,  getCrankingFuel3(NAN, 0 PASS_ENGINE_PARAMETER_SUFFIX)) << "getStartingFuel nan";
	assertEqualsM("getStartingFuel#1", 17.8, getCrankingFuel3(0, 4 PASS_ENGINE_PARAMETER_SUFFIX));
	assertEqualsM("getStartingFuel#2", 8.4480, getCrankingFuel3(8, 15 PASS_ENGINE_PARAMETER_SUFFIX));
	assertEqualsM("getStartingFuel#3", 6.48, getCrankingFuel3(70, 0 PASS_ENGINE_PARAMETER_SUFFIX));
	assertEqualsM("getStartingFuel#4", 2.6069, getCrankingFuel3(70, 50 PASS_ENGINE_PARAMETER_SUFFIX));
}


static void confgiureFordAspireTriggerShape(TriggerShape * s) {
	s->initialize(FOUR_STROKE_CAM_SENSOR, true);

	s->addEvent720(53.747, T_SECONDARY, TV_RISE);
	s->addEvent720(121.90, T_SECONDARY, TV_FALL);
	s->addEvent720(232.76, T_SECONDARY, TV_RISE);
	s->addEvent720(300.54, T_SECONDARY, TV_FALL);
	s->addEvent720(360, T_PRIMARY, TV_RISE);

	s->addEvent720(409.8412, T_SECONDARY, TV_RISE);
	s->addEvent720(478.6505, T_SECONDARY, TV_FALL);
	s->addEvent720(588.045, T_SECONDARY, TV_RISE);
	s->addEvent720(657.03, T_SECONDARY, TV_FALL);
	s->addEvent720(720, T_PRIMARY, TV_FALL);

	ASSERT_FLOAT_EQ(53.747 / 720, s->wave.getSwitchTime(0));
	ASSERT_EQ( 1,  s->wave.getChannelState(1, 0)) << "@0";
	ASSERT_EQ( 1,  s->wave.getChannelState(1, 0)) << "@0";

	ASSERT_EQ( 0,  s->wave.getChannelState(0, 1)) << "@1";
	ASSERT_EQ( 0,  s->wave.getChannelState(1, 1)) << "@1";

	ASSERT_EQ( 0,  s->wave.getChannelState(0, 2)) << "@2";
	ASSERT_EQ( 1,  s->wave.getChannelState(1, 2)) << "@2";

	ASSERT_EQ( 0,  s->wave.getChannelState(0, 3)) << "@3";
	ASSERT_EQ( 0,  s->wave.getChannelState(1, 3)) << "@3";

	ASSERT_EQ( 1,  s->wave.getChannelState(0, 4)) << "@4";
	ASSERT_EQ( 1,  s->wave.getChannelState(1, 5)) << "@5";
	ASSERT_EQ( 0,  s->wave.getChannelState(1, 8)) << "@8";
	ASSERT_FLOAT_EQ(121.90 / 720, s->wave.getSwitchTime(1));
	ASSERT_FLOAT_EQ(657.03 / 720, s->wave.getSwitchTime(8));

	ASSERT_EQ( 0,  s->wave.findAngleMatch(53.747 / 720.0, s->getSize())) << "expecting 0";
	assertEqualsM("expecting not found", -1, s->wave.findAngleMatch(53 / 720.0, s->getSize()));
	ASSERT_EQ(7, s->wave.findAngleMatch(588.045 / 720.0, s->getSize()));

	ASSERT_EQ( 0,  s->wave.findInsertionAngle(23.747 / 720.0, s->getSize())) << "expecting 0";
	ASSERT_EQ( 1,  s->wave.findInsertionAngle(63.747 / 720.0, s->getSize())) << "expecting 1";
}

TEST(misc, testAngleResolver) {
	printf("*************************************************** testAngleResolver\r\n");

	WITH_ENGINE_TEST_HELPER(FORD_ASPIRE_1996);

	engineConfiguration->globalTriggerAngleOffset = 175;

	TriggerShape * ts = &engine->triggerCentral.triggerShape;
	engine->initializeTriggerShape(NULL PASS_ENGINE_PARAMETER_SUFFIX);

	assertEqualsM("index 2", 52.76, ts->eventAngles[3]); // this angle is relation to synch point
	assertEqualsM("time 2", 0.3233, ts->wave.getSwitchTime(2));
	assertEqualsM("index 5", 412.76, ts->eventAngles[6]);
	assertEqualsM("time 5", 0.5733, ts->wave.getSwitchTime(5));

	ASSERT_EQ(4, ts->getTriggerShapeSynchPointIndex());

	ASSERT_EQ( 10,  ts->getSize()) << "shape size";

	event_trigger_position_s injectionStart;

	printf("*************************************************** testAngleResolver 0\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, -122, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ( 2,  injectionStart.eventIndex) << "eventIndex@0";
	ASSERT_NEAR(0.24, injectionStart.angleOffset, EPS5D);

	printf("*************************************************** testAngleResolver 0.1\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, -80, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ( 2,  injectionStart.eventIndex) << "eventIndex@0";
	ASSERT_FLOAT_EQ(42.24, injectionStart.angleOffset);

	printf("*************************************************** testAngleResolver 0.2\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, -54, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ( 2,  injectionStart.eventIndex) << "eventIndex@0";
	ASSERT_FLOAT_EQ(68.2400, injectionStart.angleOffset);

	printf("*************************************************** testAngleResolver 0.3\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, -53, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ(2, injectionStart.eventIndex);
	ASSERT_FLOAT_EQ(69.24, injectionStart.angleOffset);

	printf("*************************************************** testAngleResolver 1\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, 0, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ(2, injectionStart.eventIndex);
	ASSERT_FLOAT_EQ(122.24, injectionStart.angleOffset);

	printf("*************************************************** testAngleResolver 2\r\n");
	TRIGGER_SHAPE(findTriggerPosition(&injectionStart, 56, engineConfiguration->globalTriggerAngleOffset));
	ASSERT_EQ(2, injectionStart.eventIndex);
	ASSERT_FLOAT_EQ(178.24, injectionStart.angleOffset);

	TriggerShape t;
	confgiureFordAspireTriggerShape(&t);
}

TEST(misc, testPinHelper) {
	printf("*************************************************** testPinHelper\r\n");
	ASSERT_EQ(0, getElectricalValue(0, OM_DEFAULT));
	ASSERT_EQ(1, getElectricalValue(1, OM_DEFAULT));

	ASSERT_EQ(0, getElectricalValue(1, OM_INVERTED));
	ASSERT_EQ(1, getElectricalValue(0, OM_INVERTED));
}
