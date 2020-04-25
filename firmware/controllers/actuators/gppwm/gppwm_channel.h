#pragma once

#include "gppwm_condition.h"
#include "gppwm.h"

class OutputPin;
class SimplePwm;
class ValueProvider3D;

class GppwmChannel {
public:
	void init(bool usePwm, SimplePwm* pwm, const ValueProvider3D* table, const gppwm_channel* config);

	void update();


private:
	bool shouldDisable() const;
	float getOutput() const;
	void setOutput(float result);

	const gppwm_channel* m_config;

	bool m_usePwm = false;
	SimplePwm* m_pwm;
	const ValueProvider3D* m_table;
	GppwmCond m_conditions[4];
};
