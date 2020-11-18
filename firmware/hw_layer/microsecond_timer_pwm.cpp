#include "global.h"
#include "os_access.h"
#include "microsecond_timer.h"
#include "os_util.h"

bool hwStarted = false;

void setHardwareSchedulerTimer(efitick_t nowNt, efitick_t setTimeNt) {
	efiAssertVoid(OBD_PCM_Processor_Fault, hwStarted, "HW.started");

	if (hasFirmwareError()) {
		return;
	}

	auto timeInFuture = setTimeNt - nowNt;

	// Tried to schedule in the past - this can happen after config reset, etc
	if (timeInFuture < US2NT(3)) {
		warning(CUSTOM_OBD_LOCAL_FREEZE, "local freeze cnt=%d", timerFreezeCounter);
		setTimeNt = nowNt + US2NT(3);
	}

	// Tried to schedule very far out
	if (timeInFuture >= TOO_FAR_INTO_FUTURE_NT) {
		// we are trying to set callback for too far into the future. This does not look right at all
		firmwareError(CUSTOM_ERR_TIMER_OVERFLOW, "setHardwareSchedulerTimer() too far: %d", NT2US(timeInFuture));
		return;
	}

	pwm_lld_enable_channel(&SCHEDULER_PWM_DEVICE, 0, setTimeNt);
	pwmEnableChannelNotificationI(&SCHEDULER_PWM_DEVICE, 0);
}

void globalTimerCallback();

static void hwTimerCallback(PWMDriver*) {
	pwmDisableChannelNotificationI(&SCHEDULER_PWM_DEVICE, 0);
	globalTimerCallback();
}

static constexpr PWMConfig timerConfig = {
	12'000'000,		// 12mhz was chosen because it's the GCD of (168, 180, 216), the three speeds of STM32 we support
	(uint32_t)-1,	// timer period = 2^32 counts
	nullptr,		// No update callback
	{
		{PWM_OUTPUT_DISABLED, hwTimerCallback},	// Channel 0 = timer callback, others unused
		{PWM_OUTPUT_DISABLED, nullptr},
		{PWM_OUTPUT_DISABLED, nullptr},
		{PWM_OUTPUT_DISABLED, nullptr}
	},
	0,	// CR1
	0	// CR2
};

void initMicrosecondTimer() {
	pwmStart(&SCHEDULER_PWM_DEVICE, &timerConfig);

	// ChibiOS doesn't let you configure timers in output compare mode, only PWM mode.
	// We want to be able to set the compare register without waiting for an update event
	// (which would take 358 seconds at 12mhz timer speed), so we have to use normal upcounting
	// output compare mode instead.
	SCHEDULER_TIMER_DEVICE->CCMR1 = 0x00006810;

	hwStarted = true;
}
