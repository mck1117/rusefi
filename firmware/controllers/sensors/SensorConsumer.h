#pragma once

#include "Sensor.h"

template <typename TValue = float>
class SensorConsumer final
{
private:
    Sensor* m_sensor;
    const char* m_name;

public:
    SensorConsumer(const char* name)
        : m_sensor(nullptr)
        , m_name(name)
    {
    }

    SensorResult Get()
    {
        if(!m_sensor)
        {
            m_sensor = Sensor::FindSensorByName(m_name);
        }

        Sensor* sensor = m_sensor;

        if(sensor)
        {
            return sensor->Get();
        }
        else
        {
            return { false, 0.0f };
        }
    }

    TValue GetOrDefault(TValue defaultValue)
    {
        SensorResult result = Get();

        if(result.Valid)
        {
            return TValue(result.Value);
        }
        else
        {
            return defaultValue;
        }
    }
};
