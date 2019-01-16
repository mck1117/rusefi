#include "cj125_new.h"

#include "engine.h"
#include "adc_inputs.h"
#include "adc_math.h"
#include "voltage.h"
#include "pwm_generator.h"

extern TunerStudioOutputChannels tsOutputChannels;

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



// Pump current, mA
static const float cjLSUBins[] = { 
	// LSU 4.9
	-2.0f, -1.602f, -1.243f, -0.927f, -0.8f, -0.652f, -0.405f, -0.183f, -0.106f, -0.04f, 0, 0.015f, 0.097f, 0.193f, 0.250f, 0.329f, 0.671f, 0.938f, 1.150f, 1.385f, 1.700f, 2.000f, 2.150f, 2.250f
};
// Lambda value
static const float cjLSULambda[] = {
	// LSU 4.9
	0.65f, 0.7f, 0.75f, 0.8f, 0.822f, 0.85f, 0.9f, 0.95f, 0.97f, 0.99f, 1.003f, 1.01f, 1.05f, 1.1f, 1.132f, 1.179f, 1.429f, 1.701f, 1.990f, 2.434f, 3.413f, 5.391f, 7.506f, 10.119f
};





void Cj125_new::PeriodicTask(efitime_t nowNt)
{
    // Handle heater state machine
    {
        float vUr = GetUr();
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
        float vUa = GetUa();
        m_diagChannels.vUa = vUa;

		m_convertedLambda = ConvertLambda(vUa);
    }

	m_diagChannels.diag = m_spi.Diagnostic();

	tsOutputChannels.debugFloatField1 = m_diagChannels.heaterDuty;
	tsOutputChannels.debugFloatField2 = 0;
	tsOutputChannels.debugFloatField3 = 0;
	tsOutputChannels.debugFloatField4 = m_diagChannels.vUr;
	tsOutputChannels.debugFloatField5 = m_diagChannels.vUa;
	tsOutputChannels.debugFloatField6 = m_convertedLambda;
	tsOutputChannels.debugFloatField7 = m_config.vUrTarget;
	tsOutputChannels.debugIntField1 = static_cast<int>(m_diagChannels.state);
	tsOutputChannels.debugIntField2 = static_cast<int>(m_diagChannels.diag);
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

float Cj125_new::GetUa() const
{
	return getVoltageDivided("cj125ua", m_config.adcUa) / 2;
}

float Cj125_new::GetUr() const
{
	return getVoltageDivided("cj125ur", m_config.adcUr) / 2;
}

float Cj125_new::ConvertLambda(float vUa) const
{
	constexpr float currentPerVolt = 1000 / 61.9f / 17;

	// Offset by lambda=1 point (1.5v typ)
	vUa = vUa - m_vUaOffset;

	float pumpCurrent = vUa * currentPerVolt;

	return interpolate2d("cj125Lsu", pumpCurrent, cjLSUBins, cjLSULambda, sizeof(cjLSUBins) / (sizeof(cjLSUBins[0])));
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
		&m_heaterPwm,
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

	if(m_config.enableCalibration)
	{
		Calibrate();
	}

	return true;
}

void Cj125_new::Calibrate()
{
	// If we couldn't go in to cal mode, return.
	if(!m_spi.BeginCalibration())
	{
		return;
	}

	chThdSleepMilliseconds(50);

	float sum = 0;

	for (int i = 0; i < 50; i++)
	{
		chThdSleepMilliseconds(10);
		sum += getVoltageDivided("cj125ua", m_config.adcUa) / 2;
	}

	m_vUaOffset = sum / 50;

	m_spi.EndCalibration();
}

const cj125_config defaultConfig
{
	false,	// disable
	true,	// lsu 4.9
	false,
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
