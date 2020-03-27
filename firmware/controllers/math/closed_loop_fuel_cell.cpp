
#include "closed_loop_fuel_cell.h"

#include "global.h"
#include "engine.h"
#include "engine_configuration_generated_structures.h"

EXTERN_ENGINE;

constexpr float updateFreq = 1000.0f / FAST_CALLBACK_PERIOD_MS;

void ClosedLoopFuelCellBase::update(float lambdaDeadband)
{
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

#define MAX_ADJ (0.25f)

float ClosedLoopFuelCellImpl::getMaxAdjustment() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0;
	}

	float raw = m_config->maxAdd * 0.01f;
	// Don't allow maximum less than 0, or more than maximum adjustment
	return minF(MAX_ADJ, maxF(raw, 0));
}

float ClosedLoopFuelCellImpl::getMinAdjustment() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0;
	}

	float raw = m_config->maxRemove * 0.01f;
	// Don't allow minimum more than 0, or more than maximum adjustment
	return maxF(-MAX_ADJ, minF(raw, 0));
}

float ClosedLoopFuelCellImpl::getAdjustmentRate() const {
	if (!m_config) {
		// If no config, disallow adjustment.
		return 0.0f;
	}

	// 0.1% per LSB
	float raw = m_config->adjRate * 0.001f;
	// Don't allow maximum less than 0, or more than maximum rate
	return minF(0.2f, maxF(raw, 0));
}
