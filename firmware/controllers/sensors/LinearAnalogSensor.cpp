#include "LinearAnalogSensor.h"

LinearAnalogSensor::LinearAnalogSensor(const char* name, adc_channel_e analogChannel, float v1, float out1, float v2, float out2)
    : AnalogSensor(name, analogChannel)
    , m_interpolator(v1, out1, v2, out2)
{
}

SensorResult LinearAnalogSensor::ConvertVoltage(float volts)
{
    return { true, m_interpolator.getValue(volts) };
}
