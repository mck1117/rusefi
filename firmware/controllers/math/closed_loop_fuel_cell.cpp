
#include "closed_loop_fuel_cell.h"

#include "global.h"
#include "engine.h"

EXTERN_ENGINE;

constexpr float updateFreq = 1000.0f / FAST_CALLBACK_PERIOD_MS;

void ClosedLoopFuelCellBase::update()
{
	float lambdaDeadband = getLambdaDeadband();

	// Compute how far off target we are
	float lambdaError = getLambdaError();

	// If we're within the deadband, make no adjustment.
	if (absF(lambdaError) < lambdaDeadband) {
		return;
	}

	// Convert per-second to per-cycle (200hz)
	float adjustAmount = getAdjustmentRate() / updateFreq;

	// Nudge the adjustment up or down by the appropriate amount
	float adjust = m_adjustment + adjustAmount * ((lambdaError < 0) ? 1 : -1);

	// Clamp to bounds
	float minAdjust = getMinAdjustment();
	float maxAdjust = getMaxAdjustment();

	if (adjust > maxAdjust) {
		adjust = maxAdjust;
	} else if (adjust < minAdjust) {
		adjust = minAdjust;
	}

	m_adjustment = adjust;
}

float ClosedLoopFuelCellBase::getAdjustment() const {
	return 1.0f + m_adjustment;
}

float ClosedLoopFuelCellImpl::getLambdaError() const {
	return (ENGINE(engineState.targetAFR) - ENGINE(sensors.currentAfr)) / 14.7f;
}

float ClosedLoopFuelCellImpl::getLambdaDeadband() const {
	if (!m_config) {
		// If no config, return huge deadband, so no adjustment happens.
		return 1.0f;
	}

	return m_config->lambdaDeadband;
}

float ClosedLoopFuelCellImpl::getMaxAdjustment() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0;
	}

	// TODO: make this a function of [rpm, load]
	return 0.1f;
}

float ClosedLoopFuelCellImpl::getMinAdjustment() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0;
	}

	// TODO: make this a function of [rpm, load]
	return -0.1f;
}

float ClosedLoopFuelCellImpl::getAdjustmentRate() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0;
	}

	// TODO: make this a function of [rpm, load]
	return 0.01f;
}
