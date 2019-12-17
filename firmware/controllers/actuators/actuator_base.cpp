#include "actuator_base.h"

void ActuatorBase::init(Actuator* actuator, pid_s* pidParameters, const Limits& targetLimits, const Limits& outputLimits) {
	m_actuator = actuator;

	m_targetLimits = targetLimits;
	m_outputLimits = outputLimits;

	m_pid.initPidClass(pidParameters);
}

void ActuatorBase::setEnabled(bool enable) {
	m_enabled = enable;
}

void ActuatorBase::run() {
	// Update the controller
	update();

	if (m_actuator) {
		// Drive the actuator
		m_actuator->set(getOutput());
	}
}

void ActuatorBase::update() {
	// Allow manual override
	if (m_overrideEnable) {
		m_output = m_overrideValue;
		return;
	}

	// Allow disable
	if (!m_enabled) {
		m_output = 0;
		return;
	}

	// Reset ourselves if necessary
	if (m_shouldReset) {
		m_pid.reset();
		m_shouldReset = false;
	}

	m_target = m_targetLimits.clamp(getTargetImpl());

	// ***************************************
	//              Open Loop
	// ***************************************
	m_openLoop = computeOpenLoop(m_target);

	// ***************************************
	//         Closed Loop (optional)
	// ***************************************
	if (isClosedLoopEnabled()) {
		float pos = getActualPosition();
		m_error = m_target - pos;
		m_closedLoop = m_pid.getOutput(m_target, pos);
	} else {
		m_closedLoop = 0;
	}

	// Clamp the output to the allowed limits
	m_output = m_outputLimits.clamp(m_openLoop + m_closedLoop);
}
