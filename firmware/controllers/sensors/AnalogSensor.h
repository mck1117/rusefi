#pragma once

#include "Sensor.h"

#include "rusefi_enums.h"

class AnalogSensor : public Sensor
{
private:
    adc_channel_e m_analogChannel;

protected:
    AnalogSensor(const char* name, adc_channel_e analogChannel);

    void UpdateAtGetTime() override;

    // Convert an analog voltage to a sensor reading.
    virtual SensorResult ConvertVoltage(float volts) = 0;
};
