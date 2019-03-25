#include "Sensor.h"
#include "LinearAnalogSensor.h"

#include "global.h"
#include "engine.h"

EXTERN_ENGINE;

static LinearAnalogSensor vbatt;
static LinearAnalogSensor oilpressure;

void initializeSensors()
{
    // Vbatt
    vbatt = LinearAnalogSensor(SensorType::BatteryVoltage, engineConfiguration->vbattAdcChannel, 0, 0, 1.0f, engineConfiguration->vbattDividerCoeff);
    vbatt.Register();

    // oil pressure
    {
        oil_pressure_config_s* conf = &CONFIG(oilPressure);
        oilpressure = LinearAnalogSensor(SensorType::OilPressure, conf->hwChannel, conf->v1, conf->value1, conf->v2, conf->value2);
    }
}
