
#include "engine_test_helper.h"
#include "gppwm_channel.h"
#include "gppwm.h"

#include "mocks.h"

using ::testing::InSequence;

TEST(GpPwm, OutputWithPwm) {
	GppwmChannel ch;

	gppwm_channel cfg;

	MockPwm pwm;

	// Shouldn't throw with no config
	EXPECT_NO_THROW(ch.setOutput(10));

	{
		InSequence i;
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.25f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.75f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(1.0f));
	}

	ch.init(true, &pwm, nullptr, &cfg);

	// Set the output - should set directly to PWM
	ch.setOutput(25.0f);
	ch.setOutput(75.0f);

	// Test clamping behavior - should clamp to [0, 100]
	ch.setOutput(-10.0f);
	ch.setOutput(110.0f);
}

TEST(GpPwm, OutputOnOff) {
	GppwmChannel ch;

	gppwm_channel cfg;
	cfg.onAboveDuty = 50;
	cfg.offBelowDuty = 40;

	MockPwm pwm;

	{
		InSequence i;
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(1.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(1.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(1.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.0f));
		EXPECT_CALL(pwm, setSimplePwmDutyCycle(0.0f));
	}

	ch.init(false, &pwm, nullptr, &cfg);

	// Test rising edge - these should output 0, 1, 1
	ch.setOutput(49.0f);
	ch.setOutput(51.0f);
	ch.setOutput(49.0f);

	// Test falling edge - these should output 1, 0, 0
	ch.setOutput(41.0f);
	ch.setOutput(39.0f);
	ch.setOutput(41.0f);
}
