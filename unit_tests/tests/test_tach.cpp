#include "engine_test_helper.h"

TEST(tachometer, testThreePulsePerRev) {
    // This engine has a tach pin set - we need that
    WITH_ENGINE_TEST_HELPER(BMW_E34);

    // We don't actually care about ign/inj at all, just tach
    engineConfiguration->isInjectionEnabled = false;
    engineConfiguration->isIgnitionEnabled = false;

    // Configure tach pulse count
    // 5 PPR, 25% duty
    engineConfiguration->tachPulsePerRev = 5;
    engineConfiguration->tachPulseDuractionMs = 0.25f;
    engineConfiguration->tachPulseDurationAsDutyCycle = true;
    engineConfiguration->tachPulseTriggerIndex = 0;

    // Set predictable trigger settings
    engineConfiguration->trigger.customTotalToothCount = 8;
    engineConfiguration->trigger.customSkippedToothCount = 0;
    engineConfiguration->useOnlyRisingEdgeForTrigger = false;
    engineConfiguration->ambiguousOperationMode = FOUR_STROKE_CRANK_SENSOR; //FOUR_STROKE_CAM_SENSOR
	eth.applyTriggerWaveform();

    // get the engine running - 6 revolutions
    eth.fireTriggerEvents(48);

    // ensure engine speed and position
	ASSERT_EQ(1500,  GET_RPM()) << "RPM";
	EXPECT_EQ(15,  engine->triggerCentral.triggerState.getCurrentIndex()) << "index #1";
    ASSERT_EQ(engine->triggerCentral.triggerState.shaft_is_synchronized, true);

    // Clean the slate
	eth.clearQueue();

	eth.moveTimeForwardMs(5 /*ms*/);

    // This will schedule tach events!
    eth.firePrimaryTriggerRise();
    ASSERT_EQ(1500, eth.engine.rpmCalculator.rpmValue) << "RPM";

    // Now we verify that the correct stuff got scheduled
    float enginePeriod = 40000; // us
    float tachPeriod = enginePeriod / 5;
    float tachHighTime = tachPeriod * 0.25f;

    // Build the times we expect the edges to have occured
    float events[20];
    for (int i = 0; i < 10; i++) {
        auto periodStart = i * tachPeriod;
        events[2 * i] = periodStart;
        events[2 * i + 1] = periodStart + tachHighTime;
    }

    // Check that exactly 20 events got scheduled
    ASSERT_EQ(engine->executor.size(), 20);

    auto start = eth.getTimeNowUs();

    // Check that everything is in the right spot
    for (int i = 0; i < 20; i++)
    {
        auto s = engine->executor.getForUnitTest(i);
        ASSERT_NE(s, nullptr);

        EXPECT_NEAR(start + events[i], s->momentX, 1);
    }
}
