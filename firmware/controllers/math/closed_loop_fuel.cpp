#include "closed_loop_fuel.h"
#include "closed_loop_fuel_cell.h"

#include "engine.h"

#include "thermistors.h"
#include "engine_math.h"

EXTERN_ENGINE;

ClosedLoopFuelCellImpl cells[STFT_CELL_COUNT];

static size_t computeBin(int rpm, float load, stft_s& cfg) {
	// Low RPM -> idle
	if (rpm < cfg.maxIdleRegionRpm * RPM_1_BYTE_PACKING_MULT)
	{
		return 0;
	}

	// Low load -> overrun
	if (load < cfg.maxOverrunLoad)
	{
		return 1;
	}

	// High load -> power
	if (load > cfg.minPowerLoad)
	{
		return 2;
	}

	// Default -> normal "in the middle" cell
	return 3;
}

static bool shouldCorrect(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	const auto& cfg = CONFIG(stft);

	// User disable bit
	if (!CONFIG(fuelClosedLoopCorrectionEnabled)) {
		return false;
	}

	// Don't correct if not running
	if (!ENGINE(rpmCalculator).isRunning(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		return false;
	}

	// Startup delay - allow O2 sensor to warm up, etc
	if (cfg.startupDelay > ENGINE(engineState.running.timeSinceCrankingInSecs)) {
		return false;
	}

	// Check that the engine is hot enough
	if (getCoolantTemperature() < cfg.minClt) {
		return false;
	}

	// If all was well, then we're enabled!
	return true;
}

static bool shouldUpdateCorrection(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	const auto& cfg = CONFIG(stft);

	// Pause (but don't reset) correction if the AFR is off scale.
	// It's probably a transient and poorly tuned transient correction
	float afr = ENGINE(sensors.currentAfr);
	if (afr < (cfg.minAfr * 0.1f) || afr > (cfg.maxAfr * 0.1f)) {
		return false;
	}

	return true;
}

float fuelClosedLoopCorrection(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	if (!shouldCorrect(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		return 1.0f;
	}

	size_t binIdx = computeBin(GET_RPM(), getEngineLoadT(PASS_ENGINE_PARAMETER_SIGNATURE), CONFIG(stft));

#if EFI_TUNER_STUDIO
	if (engineConfiguration->debugMode == DBG_FUEL_PID_CORRECTION) {
		tsOutputChannels.debugIntField1 = binIdx;
	}
#endif // EFI_TUNER_STUDIO

	auto& cell = cells[binIdx];

	// todo: push configuration at startup
	cell.configure(&CONFIG(stft.cellCfgs[binIdx]));

	if (shouldUpdateCorrection(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		cell.update(CONFIG(stft.deadband) * 0.001f, CONFIG(stftIgnoreErrorMagnitude) PASS_ENGINE_PARAMETER_SUFFIX);
	}

	return cell.getAdjustment();
}
