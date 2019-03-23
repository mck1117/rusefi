#include "AnalogSensor.h"

#include "global.h"
#include "engine.h"
#include "analog_input.h"

EXTERN_ENGINE;

AnalogSensor::AnalogSensor(const char* name, adc_channel_e analogChannel)
    : Sensor(name)
    , m_analogChannel(analogChannel)
{
}

void AnalogSensor::PostVoltage(float volts)
{
    SensorResult result = ConvertVoltage(volts);

    if(result.Valid)
    {
        Set(result.Value);
    }
    else
    {
        Invalidate();
    }
}

void AnalogSensor::OnGetValue()
{
    // This implementation is temporary until the value is pushed
    // upon ADC conversion complete
    float volts = getVoltage(GetName(), m_analogChannel);

    PostVoltage(volts);
}
