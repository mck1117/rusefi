#include "Sensor.h"

#include <cstring>

Sensor* s_SensorList[static_cast<size_t>(SensorType::DoNotUseLastSensor)];

Sensor::Sensor(SensorType type)
    : m_type(type)
{
}

void Sensor::Set(float value)
{
    m_value = value;
    // Set valid AFTER value
    // This way the value is always valid if the flag is set
    m_valid = true;
}

void Sensor::Invalidate()
{
    m_valid = false;
}

SensorResult Sensor::Get()
{
    OnGetValue();

    return { m_valid, m_value };
}

void Sensor::Register()
{
    // Silently fail if the sensor is disabled
    if(m_type == SensorType::Disabled)
    {
        return;
    }

    size_t index = static_cast<size_t>(m_type);

    // Check if the sensor type is off the end of the list, OR if it's already registered
    if(m_type >= SensorType::DoNotUseLastSensor || s_SensorList[index] != nullptr)
    {
        // TODO throw error because we have a duplicate sensor here
    }

    s_SensorList[index] = this;
}

SensorType Sensor::GetType()
{
    return m_type;
}

/* static */ Sensor* Sensor::FindSensorByType(SensorType type)
{
    if(type >= SensorType::DoNotUseLastSensor)
    {
        return nullptr;
    }

    return s_SensorList[static_cast<size_t>(type)];
}
