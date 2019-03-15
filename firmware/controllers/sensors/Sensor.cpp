#include "Sensor.h"

#include <cstring>

Sensor* Sensor::s_llFirst = nullptr;

Sensor::Sensor(const char* name)
    : m_name(name)
    , m_llNext(nullptr)
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
    UpdateAtGetTime();

    return { m_valid, m_value };
}

void Sensor::Register()
{
    // Append to the front of the linked list
    m_llNext = s_llFirst;
    s_llFirst = this;
}

const char* Sensor::GetName()
{
    return m_name;
}

/* static */ Sensor* Sensor::FindSensorByName(const char* name)
{
    Sensor* s = Sensor::s_llFirst;

    while(s != nullptr)
    {
        // If the string matches, we've found it.
        if(strcmp(s->m_name, name) == 0)
        {
            return s;
        }

        s = s->m_llNext;
    }

    // Didn't find it, return null.
    return nullptr;
}
