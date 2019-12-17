#pragma once

#include "pid.h"
#include "dc_motor.h"

struct Limits
{
	float Min;
	float Max;

	float clamp(float input)
	{
		if (input < Min) {
			return Min;
		} else if (input > Max) {
			return Max;
		} else {
			return input;
		}
	}

	Limits() = default;
	Limits(float min, float max) : Min(min), Max(max) { }
};

class ActuatorBase
{
public:
	void reset() {
		m_shouldReset = true;
		m_overrideEnable = false;
		m_overrideValue = 0;
	}

	void init(Actuator* actuator, pid_s* pidParameters, const Limits& targetLimits, const Limits& outputLimits);
	void setEnabled(bool enable);

	// Update the controller's internal state, and push it to the actuator.
	void run();

	float getOutput() const {
		return m_output;
	}

	float getOpenLoop() const {
		return m_openLoop;
	}

	float getClosedLoop() const {
		return m_closedLoop;
	}

	float getError() const {
		return m_error;
	}

	float getTarget() const {
		return m_target;
	}

	void setOverride(float value) {
		m_overrideValue = value;
		m_overrideEnable = true;
	}

	void disableOverride() {
		m_overrideEnable = false;
	}

	// Inspect the internal state of the PID controller
	const pid_state_s* getPidState() const {
		return &m_pid;
	}

protected:
	/**************************************************************************
	 * Override these functions to implement the details of your controller:
	 *************************************************************************/

	// Comput the open loop output as a function of the target position
	virtual float computeOpenLoop(float targetPosition) const = 0;

	// What is the target position for the actuator?
	virtual float getTargetImpl() const = 0;
	// Where is the actuator in reality?
	virtual float getActualPosition() const = 0;

	// Should the actuator be controlled in closed loop mode?
	virtual bool isClosedLoopEnabled() const = 0;

	// If your controller wants to update debug info, etc...
	virtual void onStateUpdated() const {}


	// Inspect the PID controller's state, if you want
	const Pid& pid() const { return m_pid; }

private:
	// global enable & reset
	bool m_enabled = true;
	bool m_shouldReset = true;

	// override settings
	bool m_overrideEnable = false;
	bool m_overrideValue = 0.0f;

	// current state
	float m_openLoop = 0;
	float m_closedLoop = 0;
	float m_output = 0;
	float m_error = 0;
	float m_target = 0;

	// Actuator - this is where we output the result
	Actuator* m_actuator = nullptr;

	// PID controller for closed loop operation
	Pid m_pid;

	// input/output limitations
	Limits m_targetLimits;
	Limits m_outputLimits;

	void update();
};
