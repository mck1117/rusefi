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

void AnalogSensor::OnGetValue()
{
    float volts = getVoltage(GetName(), m_analogChannel);

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
