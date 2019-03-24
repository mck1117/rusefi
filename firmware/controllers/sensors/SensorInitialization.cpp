#include "Sensor.h"
#include "LinearAnalogSensor.h"

#include "global.h"
#include "engine.h"

EXTERN_ENGINE;

static LinearAnalogSensor vbatt;

void initializeSensors()
{
    // Vbatt
    vbatt = LinearAnalogSensor(SensorType::BatteryVoltage, engineConfiguration->vbattAdcChannel, 0, 0, 1.0f, engineConfiguration->vbattDividerCoeff);
    vbatt.Register();
}
