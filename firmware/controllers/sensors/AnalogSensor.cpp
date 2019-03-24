#include "AnalogSensor.h"

#include "global.h"
#include "engine.h"
#include "analog_input.h"

EXTERN_ENGINE;

AnalogSensor::AnalogSensor()
    : m_analogChannel(EFI_ADC_NONE)
{
}

AnalogSensor::AnalogSensor(SensorType type, adc_channel_e analogChannel)
    : Sensor(type)
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
	// from the ADC upon conversion complete
    float volts = getVoltage("analogSensor", m_analogChannel);

    PostVoltage(volts);
}
