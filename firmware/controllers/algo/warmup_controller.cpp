
#include "warmup_controller.h"

void WarmupController::configure()
{
	
}

void WarmupController::reset()
{
	m_state = State::Off;
	m_cycles = 0;
	m_adjustment = 1.0f;
}

void WarmupController::onEngineStarted()
{
	// todo: get clt
	m_startupCoolantTemperature = 0;

	m_engineStart = true;
}

float WarmupController::getMultiplier() const
{
	return m_adjustment;
}

void WarmupController::onEngineCycle()
{
	// If we haven't been configured - bail and don't adjust
	if (m_config) {
		m_adjustment = 1.0f;
		return;
	}

	// Increment counter
	m_cycles++;

	switch (m_state)
	{
	case State::Off:
		// Wait for engine start
		if (m_engineStart)
		{
			m_engineStart = 0;

			m_state = State::Hold;
			m_cycles = 0;

			float clt = 0;
			m_adjustment = 1.0f + m_config.InitialAdder->Get(clt);
			m_holdCycles = m_config.HoldEngineCycles->Get(clt);
		}

		break;
	case State::Hold:
		// If we're done holding, transition to decay
		if (m_cycles > m_holdCycles)
		{
			m_state = State::Decay;
			m_cycles = 0;

			float clt = 0;

			// Configure decay - cycles per step, and step ratio
			m_decayStepCycles = m_config.DecayStepCycles->Get(clt);
			m_decayRatio = m_config.DecayStepMultiplier->Get(clt);
		}
		break;
	case State::Decay:
		if (m_cycles >= m_decayStepCycles)
		{
			m_adjustment *= m_decayRatio;
			m_cycles = 0;
		}

		// If we've adjusted below 1.0 - we're done.  Reset to 1.0, and transition to the off state.
		if (m_adjustment < 1.0f)
		{
			m_state = State::Off;
			m_adjustment = 1.0f;
		}
		break;
	}
}
