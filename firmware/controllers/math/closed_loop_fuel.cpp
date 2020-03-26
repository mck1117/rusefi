
#include "closed_loop_fuel.h"

#include "global_shared.h"
#include "engine.h"

#include "thermistors.h"
#include "engine_math.h"

#include "closed_loop_fuel_cell.h"

EXTERN_ENGINE;

ClosedLoopFuelCellImpl cells[STFT_CELL_COUNT];


static size_t computeBin(int rpm, float load, stft_s& cfg) {
	// Low RPM -> idle
	if (rpm < cfg.maxIdleRegionRpm)
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
	
	if (!CONFIG(fuelClosedLoopCorrectionEnabled)) {
		return false;
	}

	// Check that the engine is hot enough
	if (getCoolantTemperature() < cfg.minClt) {
		return false;
	}

	// Check that AFR is reasonable
	float afr = ENGINE(sensors.currentAfr);
	if (afr < (cfg.minAfr * 0.01f) || afr > (cfg.maxAfr * 0.01f)) {
		return false;
	}

	// If all was well, then we're enabled!
	return true;
}

float fuelClosedLoopCorrection(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	if (!shouldCorrect(PASS_ENGINE_PARAMETER_SIGNATURE)) {
		return 1.0f;
	}

	size_t binIdx = computeBin(GET_RPM(), getEngineLoadT(PASS_ENGINE_PARAMETER_SIGNATURE), CONFIG(stft));

	if (engineConfiguration->debugMode == DBG_FUEL_PID_CORRECTION) {
		tsOutputChannels.debugIntField1 = binIdx;
	}

	auto& cell = cells[binIdx];

	// todo: push configuration at startup
	cell.configure(&CONFIG(stft.cellCfgs[binIdx]));

	cell.update(0.02f, 0.005f);

	return cell.getAdjustment();
}
