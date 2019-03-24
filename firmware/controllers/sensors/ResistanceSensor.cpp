#include "ResistanceSensor.h"

ResistanceSensor::ResistanceSensor(SensorType type, adc_channel_e analogChannel, float supplyVoltage, float pullUpResistance)
    : AnalogSensor(type, analogChannel)
    , m_supplyVoltage(supplyVoltage)
    , m_pullUpResistor(pullUpResistance)
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

    // Negative resistance doesn't make sense...
    // Only return Valid=true if resistance is non-negative
    return { resistance > 0, resistance };
}
