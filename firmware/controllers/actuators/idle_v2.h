#pragma once

#include "engine.h"
#include "expected.h"
#include "pid.h"

class IdleControllerV2 {
public:
	DECLARE_ENGINE_PTR;

	enum class Phase : uint8_t {
		Cranking,
		Idling,
		Other
	};

	float getPosition();

	void init(pid_s* pid);

	// TARGET DETERMINATION
	int getTargetRpm(float clt) const;

	// PHASE DETERMINATION: what is the driver trying to do right now?
	Phase determinePhase(int rpm, int targetRpm, SensorResult tps) const;

	// OPEN LOOP CORRECTIONS
	float getOpenLoop(Phase phase, float clt, SensorResult tps) const;
	float getCrankingOpenLoop(float clt) const;
	float getRunningOpenLoop(float clt, SensorResult tps) const;

	// CLOSED LOOP CORRECTIONS
	float getClosedLoop(Phase phase, int rpm, int targetRpm);

private:
	Pid m_pid;
};

void startNewIdleControl();
float getNewIdleControllerPosition();
