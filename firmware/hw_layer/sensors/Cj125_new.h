
#pragma once

#include "Cj125Base.h"

#include "PeriodicController.h"

class Cj125_new : public Cj125Base, public PeriodicController<UTILITY_THREAD_STACK_SIZE>
{
private:
    const cj125_hardware_config& m_hw;

	OutputPin m_heaterPin;
	SimplePwm m_heaterPwm;

public:
	Cj125_new(const cj125_config& config, Cj125Spi& spi)
		: Cj125Base(config.controller, spi)
		, PeriodicController("cj125", LOWPRIO, 50)
        , m_hw(config.hw)
	{
	}

	// PeriodicController implementation
	bool OnStarted() override;

	void PeriodicTask(efitime_t nowNt) override
	{
		Update(nowNt);
	}

	float GetPeriod() const override
	{
		return m_periodSeconds;
	}

	// analog inputs
	float GetUr() const override;
	float GetUa() const override;

    // heater control
    void SetHeaterEffectiveVoltage(float volts) override;
};
