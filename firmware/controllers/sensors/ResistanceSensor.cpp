#include "ResistanceSensor.h"

ResistanceSensor::ResistanceSensor(const char* name, adc_channel_e analogChannel, float supplyVoltage, float pullUpResistance)
    : AnalogSensor(name, analogChannel)
    , m_supplyVoltage(supplyVoltage)
    , m_pullUpResistor(pullUpResistor)
{
}

SensorResult ResistanceSensor::ConvertVoltage(float volts)
{
    float num = m_pullUpResistor * volts;
    float denom = m_supplyVoltage - volts;

    // If we're pegged at the supply voltage, there's nothing connected.
    if(denom < 0.25f)
    {
        return { false, 1e9 };
    }

    float resistance = num / denom;

    return { true, resistance };
}
