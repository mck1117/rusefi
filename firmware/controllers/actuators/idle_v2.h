#pragma once

#include "engine.h"
#include "expected.h"
#include "pid.h"

class IdleControllerBase {
public:
	DECLARE_ENGINE_PTR;

	void init(pid_s* pid);
	void update();

	enum class Phase : uint8_t {
		Cranking,
		Idling,
		Other
	};

	float getPosition();

	// VIRTUAL FUNCTIONS BELOW

	// TARGET DETERMINATION
	virtual int getTargetRpm(float clt) const = 0;

	// PHASE DETERMINATION: what is the driver trying to do right now?
	virtual Phase determinePhase(int rpm, int targetRpm, SensorResult tps) const = 0;

	// OPEN LOOP CORRECTIONS
	virtual float getOpenLoop(Phase phase, float clt, SensorResult tps) const = 0;
	virtual float getCrankingOpenLoop(float clt) const = 0;
	virtual float getRunningOpenLoop(float clt, SensorResult tps) const = 0;

	// CLOSED LOOP CORRECTIONS
	virtual float getClosedLoop(Phase phase, int rpm, int targetRpm) = 0;
};

class IdleControllerV2 : public IdleControllerBase {
public:
	// TARGET DETERMINATION
	int getTargetRpm(float clt) const override;

	// PHASE DETERMINATION: what is the driver trying to do right now?
	Phase determinePhase(int rpm, int targetRpm, SensorResult tps) const override;

	// OPEN LOOP CORRECTIONS
	float getOpenLoop(Phase phase, float clt, SensorResult tps) const override;
	float getCrankingOpenLoop(float clt) const override;
	float getRunningOpenLoop(float clt, SensorResult tps) const override;

	// CLOSED LOOP CORRECTIONS
	float getClosedLoop(Phase phase, int rpm, int targetRpm) override;

private:
	Pid m_pid;
};
