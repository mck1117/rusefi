#pragma once

#include "AnalogSensor.h"
#include "interpolation.h"

class LinearAnalogSensor final : public AnalogSensor
{
public:
    LinearAnalogSensor(SensorType type, adc_channel_e analogChannel, float v1, float out1, float v2, float out2);
protected:
    SensorResult ConvertVoltage(float volts) override;

private:
    FastInterpolation m_interpolator;
};
