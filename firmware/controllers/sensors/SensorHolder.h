#pragma once

#include "Sensor.h"

template<typename TSensorValue>
struct SensorHolderResult
{
    bool Valid;
    TSensorValue Value;

    SensorHolderResult() = default;

    // Allow implicit conversion from untyped Sensor::Result
    SensorHolderResult(Sensor::Result untyped)
        : Valid(untyped.Valid)
        , Value(TSensorValue(untyped.Value))
    {
    }
};

template<typename TSensorValue, uint32_t TDefault>
class SensorHolder
{
private:
    Sensor* m_sensor;

public:
    SensorHolder() : m_sensor(nullptr) { }

    void SetSensor(Sensor* sensor)
    {
        m_sensor = sensor;
    }

    inline SensorHolderResult<TSensorValue> Get()
    {
        Sensor* sensor = m_sensor;

        if(sensor)
        {
            return sensor->Get();
        }
        else
        {
            SensorHolderResult<TSensorValue> result;
            result.Valid = false;
            result.Value = TSensorValue(0.0f);
            return result;
        }
    }

    inline TSensorValue GetOrDefault()
    {
        auto x = Get();

        if(x.Valid)
        {
            return x.Value;
        }
        else
        {
            return TSensorValue(TDefault);
        }
    }
};
