
#include "ThermistorSensor.h"

ThermistorSensor::ThermistorSensor(SensorType type, adc_channel_e analogChannel, float supplyVoltage, thermistor_conf_s* thermistorConfig)
        : ResistanceSensor(type, analogChannel, 5.0f, thermistorConfig->bias_resistor)
{
    m_thermistorMath.setConfig(thermistorConfig);
}

SensorResult ThermistorSensor::ConvertVoltage(float volts)
{
    SensorResult resistance = ResistanceSensor::ConvertVoltage(volts);

    // If we couldn't compute a valid resistance, we can't
    // compute a valid temperature.
    if(!resistance.Valid)
    {
        return {false, 0.0f};
    }

    float kelvin = m_thermistorMath.getKelvinTemperatureByResistance(resistance.Value);

    return { true, kelvin - 273.15f };
}
