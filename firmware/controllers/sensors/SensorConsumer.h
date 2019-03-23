#pragma once

#include "Sensor.h"

template <typename TValue = float>
class SensorConsumer final
{
private:
    Sensor* m_sensor;
    SensorType m_type;

public:
    SensorConsumer(SensorType type)
        : m_sensor(nullptr)
        , m_type(type)
    {
    }

    SensorResult Get()
    {
        if(!m_sensor)
        {
            m_sensor = Sensor::FindSensorByType(m_type);
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
