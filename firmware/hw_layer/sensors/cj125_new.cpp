#include "cj125_new.h"

#include "engine.h"
#include "adc_inputs.h"
#include "adc_math.h"
#include "voltage.h"

EXTERN_ENGINE
;

// Heater control parameters

// Heater params for Idle(cold), Preheating and Control stages
// See http://www.waltech.com/wideband-files/boschsensordatasheet.htm
#define CJ125_MAXIMUM_HEATER_VOLTAGE	12.0f	// Do not allow more than 12v effective heater voltage
#define CJ125_HEATER_IDLE_VOLTAGE       0.0f    // Disable heat during idle state
#define CJ125_HEATER_PREHEAT_VOLTAGE    2.0f    // 2v preheat until condensation period is over



#define CJ125_UR_PREHEAT_THR			2.0f	// Ur > 2.0 Volts is too cold to control with PID
#define CJ125_UR_OVERHEAT_THR			0.5f	// Ur < 0.5 Volts is overheat
#define CJ125_UR_UNDERHEAT_THR          2.2f    // Ur > 2.2v after closed loop = something went wrong

#define CJ125_PUMP_SHUNT_RESISTOR		61.9f
#define CJ125_STOICH_RATIO				14.7f
#define CJ125_PUMP_CURRENT_FACTOR		1000.0f

// Some experimental magic values for heater PID regulator
#define CJ125_PID_LSU42_P				(80.0f / 16.0f * 5.0f / 1024.0f)
#define CJ125_PID_LSU42_I				(25.0f / 16.0f * 5.0f / 1024.0f)

#define CJ125_PID_LSU49_P               (8.0f)
#define CJ125_PID_LSU49_I               (0.003f)

void Cj125_new::PeriodicTask(efitime_t nowNt)
{
    // Handle heater state machine
    {
        float vUr = getVoltageDivided("cj125ur", m_config.adcUr);
        m_diagChannels.vUr = vUr;

        // Figure out which state we should be in
        State nextState = TransitionFunction(m_state, vUr, m_lastError);

        // Handle state change if necessary
        if(m_state != nextState)
        {
            m_lastStateChangeTime = nowNt;

            OnStateChanged(nextState);
            m_state = nextState;
        }

        m_diagChannels.state = m_state;
        m_diagChannels.lastError = m_lastError;

        OutputFunction(m_state, vUr);
    }

    // Handle lambda conversion
    {
        float vUa = getVoltageDivided("cj125ua", m_config.adcUa);
        m_diagChannels.vUa = vUa;

        //float pumpCurrent = (vUa - 1.5f) * amplCoeff * (CJ125_PUMP_CURRENT_FACTOR / CJ125_PUMP_SHUNT_RESISTOR);
        //m_convertedLambda =  = interpolate2d("cj125Lsu", pumpCurrent, (float *)cjLSUBins[sensorType], (float *)cjLSULambda[sensorType], cjLSUTableSize[sensorType]);
        m_convertedLambda = 0.89f;
    }
}

/* static */ Cj125_new::State Cj125_new::TransitionFunction(Cj125_new::State currentState, float vUr, Cj125_new::ErrorType& outError)
{
	bool isStopped = engine->rpmCalculator.isStopped(PASS_ENGINE_PARAMETER_SIGNATURE);

	switch (currentState) {
        case State::Disabled:
            return State::Disabled;
		case State::Idle:
			// If the engine's turning, time to heat the sensor.
			if(!isStopped) {
				return State::Preheat;
			}

			return State::Idle;
        case State::Preheat:
            // If the engine stopped, turn off the sensor!
			if(isStopped) {
				return State::Idle;
            }

            // TODO check for timeout

            return State::Preheat;
		case State::Warmup:
			// If the engine stopped, turn off the sensor!
			if(isStopped) {
				return State::Idle;
			}

			// If the sensor is sufficiently warm for PID, switch to that
			if(vUr < CJ125_UR_PREHEAT_THR) {
				return State::Running;
			}

            return State::Warmup;
		case State::Running:
			// If the engine stopped, turn off the sensor!
			if(isStopped) {
				return State::Idle;
			}

			// If the sensor is very hot, turn it off because something's wrong
			if(vUr < CJ125_UR_OVERHEAT_THR)	{
                outError = ErrorType::Overheat;
				return State::Error;
			}

			// If the sensor is very cold, something went wrong.
			if(vUr > CJ125_UR_UNDERHEAT_THR) {
				outError = ErrorType::Underheat;
				return State::Error;
			}

			return State::Running;
		case State::Error:
            // Never leave error state.
			return State::Error;
	}

    // should never get here!
    return State::Disabled;
}

void Cj125_new::OnStateChanged(State nextState)
{
    // TODO: handle state changes here

    switch(nextState)
    {
        case State::Warmup:
            m_warmupRampVoltage = m_config.warmupInitialVoltage;
            break;
        default:
            // Other states don't need anything special
            break;
    }
}

void Cj125_new::OutputFunction(Cj125_new::State state, float vUr)
{
    switch (state)
    {
        case State::Idle:
            SetHeaterEffectiveVoltage(CJ125_HEATER_IDLE_VOLTAGE);
            break;
        case State::Preheat:
            SetHeaterEffectiveVoltage(CJ125_HEATER_PREHEAT_VOLTAGE);
            break;
        case State::Warmup:
            // Increase voltage at the configured ramp rate
            m_warmupRampVoltage += m_config.warmupRampRate * m_periodSeconds;

            SetHeaterEffectiveVoltage(m_warmupRampVoltage);
            break;
        case State::Running:
            // Run in closed loop
            SetHeaterEffectiveVoltage(m_heaterPid.getValue(vUr, m_config.vUrTarget, m_periodSeconds));
            break;
        case State::Error:
            // Disable if we had an error
            SetHeaterEffectiveVoltage(0.0f);
        case State::Disabled:
            break;
    }
}

float Cj125_new::GetLambda() const
{
    if(m_state == State::Running)
    {
        return m_convertedLambda;
    }
    else
    {
        return 0.0f;
    }
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

bool Cj125_new::OnStarted()
{
	// bail out if not enabled
	if(!m_config.enable)
	{
		return false;
	}

	// bail out if analog inputs aren't set
	if(m_config.adcUa == EFI_ADC_NONE || m_config.adcUr == EFI_ADC_NONE)
	{
		return false;
	}

	// bail out if the heater pin isn't set
	if(m_config.heaterPin == GPIO_UNASSIGNED)
	{
		return false;
	}

	// Init heater pin
	startSimplePwmExt(
		m_heaterPwm,
		"cj125 heater",
		&engine->executor,
		m_config.heaterPin,
		&m_heaterPin,
		200,	// hz
		0.0f,	// duty
		applyPinState);

	// Init PID
	m_heaterPidConfig.pFactor = 0;
	m_heaterPidConfig.iFactor = 0;
	m_heaterPidConfig.dFactor = 0;
	m_heaterPidConfig.minValue = 0;
	m_heaterPidConfig.maxValue = CJ125_MAXIMUM_HEATER_VOLTAGE;
	m_heaterPidConfig.offset = 0;
	m_heaterPidConfig.period = m_periodSeconds;
	m_heaterPid.reset();

	// Try to init SPI
	if(m_spi.Init())
	{
		// Cool, we have SPI, use it.
		bool identResult = m_spi.Identify();

		// If ident was successful, then we'll use SPI later.
	}

	return true;
}

cj125_config defaultConfig
{
	false,	// disable
	true,	// lsu 4.9
	EFI_ADC_NONE,	// don't set pins
	EFI_ADC_NONE,
	GPIO_UNASSIGNED,
	30.0f,	// 30 second heater timeout
	7.0f,	// 7v initial
	0.4f,	// 0.4v/s
	1.0f,	// 1.0v target Ur
	// SPI config
	{
		SPI_NONE,	// SPI device
		GPIO_UNASSIGNED,	// CS pin
		OM_DEFAULT	// CS pin mode
	}
};
