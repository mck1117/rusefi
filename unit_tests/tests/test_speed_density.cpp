/**
 * @file test_speed_density.cpp
 *
 * @date Jun 26, 2014
 * @author Andrey Belomutskiy, (c) 2012-2018
 */

#include "global.h"
#include "engine_test_helper.h"
#include "speed_density.h"

TEST(big, testSpeedDensity) {
	printf("*************************************************** testSpeedDensity\r\n");
	EngineTestHelper eth(FORD_INLINE_6_1995);
	EXPAND_EngineTestHelper;

	engineConfiguration->trigger.customTotalToothCount = 8;
	eth.applyTriggerShape();

	eth.fireTriggerEvents(36);
	ASSERT_EQ( 1500,  eth.engine.rpmCalculator.getRpm(PASS_ENGINE_PARAMETER_SIGNATURE)) << "RPM";

	// 427 cubic inches, that's a LOT of engine
	engineConfiguration->specs.displacement = 6.99728;
	engineConfiguration->specs.cylindersCount = 8;

	engineConfiguration->injector.flow = gramm_second_to_cc_minute(5.303);

	float airMass = getCylinderAirMass(engineConfiguration, 0.92, 98, 293.16);

	ASSERT_FLOAT_EQ(0.9371106624, airMass);

	// 0.01414 sec or 14.14 ms
	ASSERT_FLOAT_EQ(0.014137065038, sdMath(engineConfiguration, airMass, 12.5));
}
