/**
 *  @file dc_motor.h
 * 
 *  @date Dec 22, 2018
 *  @author Matthew Kennedy, (c) 2018
 */

#pragma once

// hack hack hack
struct Actuator
{
	virtual bool set(float power) = 0;
};

/**
 * @brief Brushed or brushless DC motor interface
 * 
 * Represents a DC motor (brushed or brushless) that provides simple
 * torque/power/current/duty cycle control, but not accurate absolute position control.
 */
class DcMotor : public Actuator
{
public:
    /**
     * @brief Sets the motor duty cycle.
     * @param duty +1.0f represents full power forward, and -1.0f represents full power backward.
     * @return True if any fault was detected driving the motor, and false if successful.
     */
    //virtual bool set(float duty) = 0;

    /**
     * @brief Get the current motor duty cycle.
     * @return The current duty cycle setting. +1.0f represents full power forward, and -1.0f represents full power backward.
     */
    virtual float get() const = 0;

    virtual bool isOpenDirection() const = 0;
};

class SimplePwm;

/**
 * @brief Represents a DC motor controller (H bridge) with one pin for enable (PWM),
 * and two pins for direction control.
 * 
 * The enable pin is used for PWM and disable, and the two direction pins are used
 * to set the polarity of each half of the H bridge.  setting {dir1,dir2} = 10 should,
 * turn the motor one direction (positive duty), and = 01 should turn the other way (negative
 * duty).
 */
class TwoPinDcMotor : public DcMotor
{
public:
    enum class ControlType
    {
    	/**
    	 * For example TLE7209 - two control wires:
    	 * PWM on both wires - one to open, another to close
    	 */
        PwmDirectionPins,
		/**
		 * For example VNH2SP30 - three control wires:
		 * PWM on 'enable' PIN, two binary pins for direction
		 *
		 * TLE9201 with two wire control also uses this mode
		 * PWM on one pin, open/close using one binary direction pin, second direction pin unused
		 */
        PwmEnablePin,
    };

private:
    SimplePwm* const m_enable;
    SimplePwm* const m_dir1;
    SimplePwm* const m_dir2;
    float m_value = 0;

    ControlType m_type;
public:
    /**
     * @param enable SimplePwm driver for enable pin, for PWM speed control.
     * @param dir1 Enable 1 or direction 1 pin.  Gets set high to rotate forward.
     * @param dir2 Enable 2 or direction 2 pin.  Gets set high to rotate backward.
     */
    TwoPinDcMotor(SimplePwm* enable, SimplePwm* dir1, SimplePwm* dir2);

    virtual bool set(float duty) override;
    float get() const override;
    bool isOpenDirection() const override;

    void setType(ControlType type) { m_type = type; }
};
