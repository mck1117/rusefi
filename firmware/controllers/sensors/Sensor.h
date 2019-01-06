#pragma once

class Sensor
{
public:
    /**
     * @brief Stores a result from a sensor interface.
     */
    struct Result
    {
        bool Valid;
        float Value;
    };

    /**
     * @brief Retrieve the sensor's last reading.
     */
    inline Result Get() const
    {
        return m_result;
    }

private:
    Result m_result;

protected:
    // @brief protected constructor so this class is abstract.
    Sensor() :
        m_result({ false, 0.0f })
    {
    }

    /**
     * @brief Set the sensor's value.  Call this whenever a new value is available for the sensor.
     */
    inline void Set(Result result)
    {
        m_result = result;
    }
};

/**
 * Represents a sensor that converts an analog voltage to a sensor reading.
 */
class AnalogVoltageSensor : public Sensor
{
protected:
    AnalogVoltageSensor() {};

    /**
     * @brief Convert an analog voltage to a sensor reading.
     */
    virtual Sensor::Result ConvertSensor(float volts) = 0;

public:
    /**
     * @brief Provide an analog voltage to convert.
     */
    void SetVoltage(float volts)
    {
        Sensor::Result result = ConvertSensor(volts);
        Set(result);
    }
};
