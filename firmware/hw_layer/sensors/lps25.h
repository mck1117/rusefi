#pragma once

#include "i2c_bb.h"

#include "expected.h"

class Lps25 {
public:
	// Returns true if the sensor was initialized successfully.
	bool init(brain_pin_e scl, brain_pin_e sda);

	expected<float> readPressureKpa();

private:
	BitbangI2c m_i2c;
	bool m_hasInit = false;
};
