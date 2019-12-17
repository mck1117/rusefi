/**
 * @file    converter_sensor.h
 *
 * @date September 12, 2019
 * @author Matthew Kennedy, (c) 2019
 */

#pragma once

#include "converters/sensor_converter_func.h"
#include "stored_value_sensor.h"

#include <type_traits>

/**
 * @brief Class for sensors that convert from some raw floating point
 * value (ex: voltage, frequency, pulse width) to a sensor reading.
 *
 * To use this class, implement the conversion operation for your sensor
 * as a class that inherits from SensorConverter, and implement convert
 * to convert a raw reading from the sensor to a usable value (and valid bit).
 *
 * Register an instance of the new class with an interface
 * that provides and posts raw values so the sensor can update.
 */
class FunctionalSensor final : public StoredValueSensor {
public:
	explicit FunctionalSensor(SensorType type)
		: StoredValueSensor(type) { }

	void postRawValue(float inputValue);

	void setFunction(SensorConverter& func) {
		m_function = &func;
	}

private:
	// Conversion function for this sensor
	SensorConverter* m_function = nullptr;
};
