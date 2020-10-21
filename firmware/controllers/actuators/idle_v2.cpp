#include "idle_v2.h"
#include "idle_thread.h"

EXTERN_ENGINE;

void IdleControllerV2::init(pid_s* pid) {
	m_pid.initPidClass(pid);
}

float IdleControllerBase::getPosition() {
	// On failed sensor, use 0 deg C - should give a safe highish idle
	float clt = Sensor::get(SensorType::Clt).value_or(0);
	auto tps = Sensor::get(SensorType::DriverThrottleIntent);
	auto rpm = engine->rpmCalculator.getRpm();

	// Compute the target we're shooting for
	auto targetRpm = getTargetRpm(clt);

	// Determine what phase we're in: cranking, idling, running
	auto phase = determinePhase(rpm, targetRpm, tps);

	// Compute open loop value
	float result = getOpenLoop(phase, clt, tps);

	// Don't do PID in case of failed TPS - we don't know when the driver is trying to leave idle land
	if (tps.Valid && engineConfiguration->idleMode == IM_AUTO) {
		result += getClosedLoop(phase, rpm, targetRpm);
	}

	return clampF(0, result, 100);
}

int IdleControllerV2::getTargetRpm(float clt) const {
	// TODO: bump target rpm based on AC and/or fan(s)?

	return interpolate2d("cltRpm", clt, CONFIG(cltIdleRpmBins), CONFIG(cltIdleRpm));
}

IdleControllerV2::Phase IdleControllerV2::determinePhase(int rpm, int targetRpm, SensorResult tps) const {
	if (!engine->rpmCalculator.isRunning()) {
		return Phase::Cranking;
	}

	if (!tps) {
		// TODO: is this the right thing to do?
		return Phase::Other;
	}

	// if throttle pressed, we're out of the idle corner
	if (tps.Value > CONFIG(idlePidDeactivationTpsThreshold)) {
		return Phase::Other;
	}

	// If rpm too high, we're out of the idle corner
	int maximumIdleRpm = targetRpm + CONFIG(idlePidRpmUpperLimit);
	if (rpm > maximumIdleRpm) {
		return Phase::Other;
	}

	// No other conditions met, we are idling!
	return Phase::Idling;
}

float IdleControllerV2::getCrankingOpenLoop(float clt) const {
	return 100.0f * interpolate2d("cltCrankingT", clt, config->cltCrankingCorrBins, config->cltCrankingCorr);
}

float IdleControllerV2::getRunningOpenLoop(float clt, SensorResult tps) const {
	float running = 100.0f * interpolate2d("cltT", clt, config->cltIdleCorrBins, config->cltIdleCorr);

	// Now we bump it by the AC/fan amount if necessary
	running += engine->acSwitchState ? CONFIG(acIdleExtraOffset) : 0;
	// TODO: fan idle bump
	// running += enginePins.fanRelay.getLogicValue() ? CONFIG(acIdleExtraOffset) : 0;

	// Now bump it by the specified amount when the throttle is opened (if configured)
	// nb: invalid tps will make no change, no explicit check required
	running += interpolateClamped(
		0, 0,
		CONFIG(idlePidDeactivationTpsThreshold), CONFIG(iacByTpsTaper),
		tps.value_or(0));

	return running;
}

float IdleControllerV2::getOpenLoop(Phase phase, float clt, SensorResult tps) const {
	float running = getRunningOpenLoop(clt, tps);

	// Cranking value is either its own table, or the running value if not overriden
	float cranking = CONFIG(overrideCrankingIacSetting) ? getCrankingOpenLoop(clt) : running;

	// if we're cranking, nothing more to do.
	if (phase == Phase::Cranking) {
		return cranking;
	}

	// Interpolate between cranking and running over a short time
	auto revsSinceStart = engine->rpmCalculator.getRevolutionCounterSinceStart();
	return interpolateClamped(0, cranking, CONFIG(afterCrankingIACtaperDuration), running, revsSinceStart);
}

float IdleControllerV2::getClosedLoop(Phase phase, int rpm, int targetRpm) {
	// Reset PID when cranking - don't try to do closed loop when not running
	if (phase == Phase::Cranking) {
		m_pid.reset();
	}

	// Only update PID controller during idle conditions
	if (phase == Phase::Idling) {
		m_pid.getOutput(targetRpm, rpm);
	}

	// But always return the position, even not when in idle conditions
	return m_pid.output;
}

IdleControllerV2 idler;

void startNewIdleControl() {
	idler.init(&CONFIG(idleRpmPid));
}

float getNewIdleControllerPosition() {
	return idler.getPosition();
}
