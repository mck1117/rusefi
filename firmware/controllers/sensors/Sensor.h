#pragma once

#include "SensorType.h"

struct SensorResult
{
    bool Valid;
    float Value;
};

class Sensor
{
public:
    // Get the sensor's current value.
    SensorResult Get();

    // Register the sensor for lookup by name.
    void Register();

    SensorType GetType();

    // Find a sensor by name.  Returns the sensor with the requested type, or nullptr if no such sensor is found.
    static Sensor* FindSensorByType(SensorType type);

protected:
    // Default constructor defaults to SensorType::Disabled (safe for static initializers!)
    Sensor() = default;
    explicit Sensor(SensorType type);

    // Set the value for the sensor.  Also sets the valid flag.
    void Set(float value);

    // Clear the valid flag, indicating that the sensor reading is invalid.
    void Invalidate();

    /**
     * This is a hack! It only exists because the current model converts an analog sensor when
     * its consumer needs a reading. Ideally, the sensor is converted and updated when the ADC
     * conversion completes.  Once that is supported, this function can be removed.
     */
    virtual void OnGetValue() {};

private:
    SensorType m_type;

    // Volatile ensures these members are read in-order
    volatile bool m_valid;
    volatile float m_value;
};
