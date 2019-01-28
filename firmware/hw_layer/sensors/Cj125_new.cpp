
#include "Cj125_new.h"

#include "engine.h"
#include "adc_inputs.h"
#include "adc_math.h"
#include "pwm_generator.h"
#include "voltage.h"

EXTERN_ENGINE;


bool Cj125_new::OnStarted()
{
    if(!Init())
    {
        return false;
    }

	// bail out if analog inputs aren't set
	if(m_hw.adcUa == EFI_ADC_NONE || m_hw.adcUr == EFI_ADC_NONE)
	{
		return false;
	}

	// bail out if the heater pin isn't set
	if(m_hw.heaterPin == GPIO_UNASSIGNED)
	{
		return false;
	}

    // Init heater pin
    startSimplePwmExt(
        &m_heaterPwm,
        "cj125 heater",
        &engine->executor,
        m_hw.heaterPin,
        &m_heaterPin,
        200,	// hz
        0.0f,	// duty
        applyPinState);

    return true;
}

float Cj125_new::GetUa() const
{
	return getVoltageDivided("cj125ua", m_hw.adcUa) / 2;
}

float Cj125_new::GetUr() const
{
	return getVoltageDivided("cj125ur", m_hw.adcUr) / 2;
}

void Cj125_new::SetHeaterEffectiveVoltage(float volts)
{
	if(volts > CJ125_MAXIMUM_HEATER_VOLTAGE)
	{
		volts = CJ125_MAXIMUM_HEATER_VOLTAGE;
	}

	if(volts < 0)
	{
		volts = 0;
	}

	// Because this is a resistive heater, duty = (V_eff / V_batt) ^ 2
	float powerRatio = volts / getVBatt();
	float dutyCycle = powerRatio * powerRatio;

	m_heaterPwm.setSimplePwmDutyCycle(dutyCycle);
	m_diagChannels.heaterDuty = dutyCycle;
}
