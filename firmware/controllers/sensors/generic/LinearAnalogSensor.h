#pragma once

#include "Sensor.h"
#include "interpolation.h"

class LinearAnalogSensor final : public AnalogVoltageSensor
{
private:
    const FastInterpolation m_interpolator;

    const float m_min, m_max;

    Sensor::Result ConvertSensor(float volts) override;

public:
    LinearAnalogSensor(float v1, float v2, float out1, float out2, float min, float max);
};

class ScaledAnalogSensor final : public AnalogVoltageSensor
{
private:
    float m_scale;

    Sensor::Result ConvertSensor(float volts) override
    {
        return { true, volts * m_scale };
    }

public:
    ScaledAnalogSensor(float scale) : m_scale(scale) {}
};
