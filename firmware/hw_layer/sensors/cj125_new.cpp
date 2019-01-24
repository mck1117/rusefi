#include "cj125_new.h"

#include "engine.h"
#include "adc_inputs.h"
#include "adc_math.h"
#include "voltage.h"
#include "pwm_generator.h"
#include "LambdaConverter.h"


// Heater control parameters

// Heater params for Idle(cold), Preheating and Control stages
// See http://www.waltech.com/wideband-files/boschsensordatasheet.htm
#define CJ125_MAXIMUM_HEATER_VOLTAGE	12.0f	// Do not allow more than 12v effective heater voltage
#define CJ125_HEATER_IDLE_VOLTAGE		0.0f	// Disable heat during idle state
#define CJ125_HEATER_PREHEAT_VOLTAGE	2.0f	// 2v preheat until condensation period is over

// Parameters for heater closed loop transition and limits
#define CJ125_UR_PREHEAT_THR			2.0f	// Ur > 2.0 Volts is too cold to control with PID
#define CJ125_UR_OVERHEAT_THR			0.5f	// Ur < 0.5 Volts is overheat: after closed loop = something went wrong
#define CJ125_UR_UNDERHEAT_THR			2.2f	// Ur > 2.2v Volts is too cold: after closed loop = something went wrong

// Timeout parameters
#define CJ125_PREHEAT_DURATION (US2NT(US_PER_SECOND_LL) * 5)	// Heat for this long (gently) to heat off condensation before switching to the ramp
#define CJ125_WARMUP_TIMEOUT (US2NT(US_PER_SECOND_LL) * 25)		// We try to warm up for this long before giving up

// Calibration limits
#define CJ125_UA_CAL_MIN 	(1.4f)	// minimum acceptable vUa (1.5v typ)
#define CJ125_UA_CAL_MAX 	(1.6f)	// maximum acceptable vUa (1.5v typ)
#define CJ125_UR_CAL_MIN 	(0.9f)	// minimum acceptable vUr (1.0v typ)
#define CJ125_UR_CAL_MAX 	(1.1f)	// maximum acceptable vUr (1.0v typ)



extern TunerStudioOutputChannels tsOutputChannels;

EXTERN_ENGINE
;

void Cj125_new::PeriodicTask(efitime_t nowNt)
{
	// TODO: hack hack hack
	memcpy(&m_heaterPidConfig, &m_config.heaterPid, sizeof(pid_s));

	// Handle heater state machine
	{
		float vUr = GetUr();
		m_diagChannels.vUr = vUr;

		bool isStopped = engine->rpmCalculator.isStopped(PASS_ENGINE_PARAMETER_SIGNATURE);

		// Figure out which state we should be in
		State nextState = TransitionFunction(m_state, vUr, nowNt - m_lastStateChangeTime, !isStopped, m_lastError);

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

	if (engineConfiguration->debugMode == DBG_CJ125)
	{
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
}

/* static */ Cj125_new::State Cj125_new::TransitionFunction(Cj125_new::State currentState, float vUr, efitime_t timeInState, bool engineTurning, Cj125_new::ErrorType& outError)
{

	switch (currentState) {
		case State::Disabled:
			return State::Disabled;
		case State::Idle:
			// If the engine's turning, time to heat the sensor.
			if(engineTurning) {
				return State::Preheat;
			}

			return State::Idle;
		case State::Preheat:
			// If the engine stopped, turn off the sensor!
			if (!engineTurning)
			{
				return State::Idle;
			}

			// After preheat delay, switch to warmup.
			if(timeInState > CJ125_PREHEAT_DURATION)
			{
				return State::Warmup;
			}

			return State::Preheat;
		case State::Warmup:
			// If the engine stopped, turn off the sensor!
			if (!engineTurning)
			{
				return State::Idle;
			}

			// If it's been too long, something's wrong
			if(timeInState > CJ125_WARMUP_TIMEOUT)
			{
				outError = ErrorType::HeaterTimeout;
				return State::Error;
			}

			// If the sensor is sufficiently warm for PID, switch to that
			if(vUr < CJ125_UR_PREHEAT_THR) {
				return State::Running;
			}

			return State::Warmup;
		case State::Running:
			// If the engine stopped, turn off the sensor!
			if (!engineTurning)
			{
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
	float vUaDelta = vUa - m_vUaOffset;

	switch (m_sensorType)
	{
		case SensorType::BoschLsu42:
			return LambdaConverterLsu42::ConvertLambda(vUaDelta, 17);
		case SensorType::BoschLsu49:
			return LambdaConverterLsu49::ConvertLambda(vUaDelta, 17);
	}

	return 0.0f;
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

	m_lastStateChangeTime = getTimeNowNt();

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
		sum += GetUa();
	}

	m_vUaOffset = sum / 50;

	// Check that the measured vUa @ lambda=1 value is reasonable
	if(m_vUaOffset > CJ125_UA_CAL_MAX ||
		m_vUaOffset < CJ125_UA_CAL_MIN)
	{
		m_state = State::Error;
		m_lastError = ErrorType::CalibrationFailureUa;
	}

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
	// Heater PID
	{
		8,		// kP
		0.003f,	// kI
		0,		// kD
		0,		// offset
		0,		// period
		0,		// min
		12		// max
	},
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

Cj125_new::Cj125_new(const cj125_config& config)
	: PeriodicController("cj125", LOWPRIO, 50)
	, m_config(config)
	, m_spi(config.spi)
	, m_state(config.enable ? State::Idle : State::Disabled)
	, m_lastError(ErrorType::None)
	, m_sensorType(config.isLsu49 ? SensorType::BoschLsu49 : SensorType::BoschLsu42)
	, m_heaterPid(&m_heaterPidConfig)
{
}
