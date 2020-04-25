
#include "gppwm_channel.h"

#include "engine.h"
#include "pwm_generator_logic.h"
#include "table_helper.h"

EXTERN_ENGINE;

bool GppwmChannel::shouldDisable() const {
	// If any condition reports we should disable, propogate
	for (size_t i = 0; i < efi::size(m_conditions); i++)
	{
		if (m_conditions[i].shouldDisable()) {
			return true;
		}
	}

	return false;
}

void GppwmChannel::setOutput(float result) {
	if (!m_usePwm) {
		// Flip hard at 50 - convert to on/off
		result = (result > 50) ? 100 : 0;
	}

	m_pwm->setSimplePwmDutyCycle(clampF(0, result / 100.0f, 100));
}

float GppwmChannel::getOutput() const {
	if (shouldDisable()) {
		return m_config->dutyWhenDisabled;
	} else {
		// Look up with the table
		expected<float> loadAxisValue = readGppwmChannel(m_config->loadAxis);

		// If that didn't work, fall back on disabled value
		if (!loadAxisValue) {
			return m_config->dutyWhenDisabled;
		}

		return m_table->getValue(GET_RPM(), loadAxisValue.Value);
	}
}

void GppwmChannel::update() {
	// Without a config, nothing to do.
	if (!m_config) {
		return;
	}

	float output = getOutput();
	setOutput(output);
}
