#pragma once

#include "ResistanceSensor.h"
#include "engine.h"
#include "engine_configuration_generated_structures.h"

class ThermistorSensor final : public ResistanceSensor
{
public:
    ThermistorSensor() = default;
    ThermistorSensor(SensorType type, adc_channel_e analogChannel, float supplyVoltage, thermistor_conf_s* thermistorConfig);

protected:
    SensorResult ConvertVoltage(float volts) override;

private:
    ThermistorMath m_thermistorMath;
};
