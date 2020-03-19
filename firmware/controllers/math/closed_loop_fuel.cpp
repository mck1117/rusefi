
#include "closed_loop_fuel.h"

#include "global_shared.h"
#include "engine.h"

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


// 			getCoolantTemperature() < CONFIG(fuelClosedLoopCltThreshold) ||
// 			getTPS(PASS_ENGINE_PARAMETER_SIGNATURE) > CONFIG(fuelClosedLoopTpsThreshold) ||
// 			ENGINE(sensors.currentAfr) < CONFIG(fuelClosedLoopAfrLowThreshold) ||
// 			ENGINE(sensors.currentAfr) > engineConfiguration->fuelClosedLoopAfrHighThreshold) {
// 		engine->engineState.running.pidCorrection = 0;
// 		fuelPid.reset();
// 		return;
// 	}

static bool shouldCorrect(float clt) {
	if (!CONFIG(fuelClosedLoopCorrectionEnabled)) {
		return false;
	}

	if (clt < CONFIG(fuelClosedLoopCltThreshold)) {
		return false;
	}

	return true;
}

float fuelClosedLoopCorrection(int rpm, float load, float clt) {
	if (!shouldCorrect(clt)) {
		return 1.0f;
	}

	auto binIdx = computeBin(rpm, load, CONFIG(stft));

	auto& cell = cells[binIdx];

	// todo: push configuration at startup
	cell.configure(&CONFIG(stft.cellCfgs[binIdx]));

	cell.update(0.02f, 0.005f);

	if (engineConfiguration->debugMode == DBG_FUEL_PID_CORRECTION) { 
		tsOutputChannels.debugIntField1 = binIdx;
	}

	return cell.getAdjustment();
}
