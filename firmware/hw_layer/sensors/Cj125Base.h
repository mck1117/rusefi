#pragma once

#include "cj125_spi.h"
#include "pwm_generator_logic.h"
#include "pid.h"

#define CJ125_MAXIMUM_HEATER_VOLTAGE	12.0f	// Do not allow more than 12v effective heater voltage

class Cj125Base
{
	enum class State : uint8_t
	{
		Disabled = 0,
		Idle,
		Preheat,
		Warmup,
		Running,
		Error,
	};

	enum class ErrorType : uint8_t
	{
		None = 0,
		HeaterTimeout,
		Underheat,
		Overheat,
		WrongIdentification,
		WrongInit,
		CalibrationFailureUa,
		CalibrationFailureUr,
	};

	enum class SensorType : uint8_t
	{
		BoschLsu42 = 0,
		BoschLsu49,
	};


	struct DiagnosticChannels
	{
		float vUr, vUa;
		float heaterDuty;
		State state;
		ErrorType lastError;
		uint8_t diag;
	};

	const cj125_controller_config& m_config;

	Cj125Spi& m_spi;

	State m_state;
	ErrorType m_lastError;
	SensorType m_sensorType;

	Logging* m_logger;

	pid_s m_heaterPidConfig;
	Pid m_heaterPid;

	efitime_t m_lastStateChangeTime;

	float m_warmupRampVoltage;

	float m_vUaOffset = 1.5f;
	volatile float m_convertedLambda;
private:
	// Heater control state machine
	static State TransitionFunction(State currentState, float vUr, efitime_t timeInState, bool engineStopped, ErrorType& outError);
	void OnStateChanged(State nextState);
	void OutputFunction(State state, float vUr);

	// Individual functions

	// Lambda conversion
	float ConvertLambda(float vUa) const;

	// Calibration
	void Calibrate();
protected:
	DiagnosticChannels m_diagChannels;

	bool Init();
	void Update(efitime_t nowNt);

	// analog inputs
	virtual float GetUr() const = 0;
	virtual float GetUa() const = 0;

	// heater output
	virtual void SetHeaterEffectiveVoltage(float volts) = 0;

	virtual float GetPeriod() const = 0;
public:
	explicit Cj125Base(const cj125_controller_config& config, Cj125Spi& spi);

	float GetLambda() const;
	const DiagnosticChannels& GetDiagChannels() const { return m_diagChannels; }
};
