#pragma once

#include "engine_configuration_generated_structures.h"
#include "expected.h"

expected<float> readGppwmChannel(gppwm_channel_e channel);

class GppwmCond final {
public:
	void init(gppwm_condition* cfg) { m_cfg = cfg; }

	bool shouldDisable() const;

private:
	const gppwm_condition* m_cfg;
};
