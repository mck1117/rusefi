#include "gppwm_condition.h"
#include "sensor.h"

expected<float> readGppwmChannel(gppwm_channel_e channel) {
	switch (channel) {
	case GPPWM_Tps:
		return Sensor::get(SensorType::DriverThrottleIntent).value_or(0);
	case GPPWM_Map:
		// TODO
		return 0;
	case GPPWM_Clt:
		return Sensor::get(SensorType::Clt).value_or(0);
	case GPPWM_Iat:
		return Sensor::get(SensorType::Iat).value_or(0);
	default:
		return unexpected;
	}
}

bool GppwmCond::shouldDisable() const {
	if (!m_cfg) {
		// No config yet, indicate disable
		return true;
	}

	if (m_cfg->channel == GPPWM_None) {
		return false;
	}

	expected<float> val = readGppwmChannel(m_cfg->channel);

	if (!val) {
		// Something went wrong reading the sensor - indicate we should disable
		return true;
	}

	auto compare = m_cfg->compareValue / 100;

	switch (m_cfg->mode) {
	case GPPWM_GreaterThan:
		return val.Value > compare;
	case GPPWM_LessThan:
		return val.Value < compare;
	default:
		return false;
	}
}