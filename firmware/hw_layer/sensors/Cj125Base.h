#pragma once

#include "PeriodicController.h"
#include "cj125_spi.h"
#include "pwm_generator_logic.h"
#include "pid.h"

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

	DiagnosticChannels m_diagChannels;

protected:
	const cj125_config& m_config;
private:
	Cj125Spi m_spi;

	State m_state;
	ErrorType m_lastError;
	SensorType m_sensorType;

	SimplePwm m_heaterPwm;
	OutputPin m_heaterPin;

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

	// heater
	void SetHeaterEffectiveVoltage(float volts);

	// Calibration
	void Calibrate();
protected:
	bool Init();
	void Update(efitime_t nowNt);

	// analog inputs
	virtual float GetUr() const = 0;
	virtual float GetUa() const = 0;

	virtual float GetPeriod() const = 0;
public:
	explicit Cj125Base(const cj125_config& config);

	float GetLambda() const;
	const DiagnosticChannels& GetDiagChannels() const { return m_diagChannels; }
};

struct Cj125_new : public Cj125Base, public PeriodicController<UTILITY_THREAD_STACK_SIZE>
{
	explicit Cj125_new(const cj125_config& config)
		: Cj125Base(config)
		, PeriodicController("cj125", LOWPRIO, 50)
	{
	}

	// PeriodicController implementation
	bool OnStarted() override
	{
		return Init();
	}

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
};
