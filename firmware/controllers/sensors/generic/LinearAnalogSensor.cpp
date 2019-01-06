#include "LinearAnalogSensor.h"

LinearAnalogSensor::LinearAnalogSensor(float v1, float v2, float out1, float out2, float min, float max)
    : m_interpolator(v1, out1, v2, out2)
    , m_min(min)
    , m_max(max)
{
}

Sensor::Result LinearAnalogSensor::ConvertSensor(float volts)
{
    float value = m_interpolator.getValue(volts);

    // Invalid if the value is outside the min/max
    if(value < m_min || value > m_max)
    {
        return { false, value };
    }
    else
    {
        return { true, value };
    }
}
