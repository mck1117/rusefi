
#include "closed_loop_fuel.h"

#include "global_shared.h"
#include "engine.h"

#include "closed_loop_fuel_cell.h"

EXTERN_ENGINE;

ClosedLoopFuelCellImpl cells[16];

static const int rpmSplitPoints[] =
{
	1100, 1800, 2500
};

static const float airmassSplitPoints[] =
{
	0.13f,
	0.19f,
	0.25f,
}

static const closed_loop_fuel_cfg cfg =
{
	0.005f
};

template <class TValue, size_t N>
static size_t findInArray(TValue search, TValue (&arr)[N])
{
	size_t idx = 0;

	while (true) {
		if (idx == N) {
			break;
		}

		if (arr[idx] < search) {
			break;
		}

		idx++;
	}

	return idx;
}

static size_t computeBin(int rpm, float airmass) {
	auto col = findInArray(rpm, rpmSplitPoints);
	auto row = findInArray(airmass, airmassSplitPoints);

	return 4 * col + row;
}


// 			getCoolantTemperature() < CONFIG(fuelClosedLoopCltThreshold) ||
// 			getTPS(PASS_ENGINE_PARAMETER_SIGNATURE) > CONFIG(fuelClosedLoopTpsThreshold) ||
// 			ENGINE(sensors.currentAfr) < CONFIG(fuelClosedLoopAfrLowThreshold) ||
// 			ENGINE(sensors.currentAfr) > engineConfiguration->fuelClosedLoopAfrHighThreshold) {
// 		engine->engineState.running.pidCorrection = 0;
// 		fuelPid.reset();
// 		return;
// 	}

static bool shouldCorrect(int rpm) {
	if (!CONFIG(fuelClosedLoopCorrectionEnabled)) {
		return false;
	}

	if (rpm < CONFIG(fuelClosedLoopRpmThreshold)) {
		return false;
	}

	if (getCoolantTemperature() < CONFIG(fuelClosedLOopCltThreshold)) {
		return false;
	}

	return true;
}

float fuelClosedLoopCorrection(int rpm, float airmass) {
	if (!shouldCorrect(rpm, airmass)) {
		return 1.0f;
	}

	auto& cell = cells[computeBin(rpm, airmass)];

	cell.configure(&cfg);

	cell.update();
	return cell.getAdjustment();
}
