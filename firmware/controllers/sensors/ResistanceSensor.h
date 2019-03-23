#pragma once

#include "AnalogSensor.h"

class ResistanceSensor : public AnalogSensor
{
public:
    ResistanceSensor(const char* name, adc_channel_e analogChannel, float supplyVoltage, float pullUpResistance);

protected:
    SensorResult ConvertVoltage(float volts) override;

private:
    const float m_supplyVoltage;
    const float m_pullUpResistor;
};
