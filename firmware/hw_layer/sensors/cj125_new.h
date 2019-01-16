

#include "PeriodicController.h"
#include "cj125_spi.h"
#include "pwm_generator_logic.h"
#include "pid.h"

class Cj125_new : public PeriodicController<UTILITY_THREAD_STACK_SIZE>
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

	const cj125_config& m_config;
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
	// PeriodicController implementation
	bool OnStarted() override;
	void PeriodicTask(efitime_t nowNt) override;

	// Heater control state machine
	static State TransitionFunction(State currentState, float vUr, efitime_t timeInState, ErrorType& outError);
	void OnStateChanged(State nextState);
	void OutputFunction(State state, float vUr);

	// Individual functions

	// analog inputs
	float GetUr() const;
	float GetUa() const;

	// Lambda conversion
	float ConvertLambda(float vUa) const;

	// heater
	void SetHeaterEffectiveVoltage(float volts);

	// Calibration
	void Calibrate();
public:
	Cj125_new(const cj125_config& config)
		: PeriodicController("cj125", LOWPRIO, 50)
		, m_config(config)
		, m_spi(config.spi)
		, m_state(config.enable ? State::Idle : State::Disabled)
		, m_lastError(ErrorType::None)
		, m_sensorType(config.isLsu49 ? SensorType::BoschLsu49 : SensorType::BoschLsu42)
		, m_heaterPid(&m_heaterPidConfig)
	{
	}

	float GetLambda() const;
	const DiagnosticChannels& GetDiagChannels() const { return m_diagChannels; }
};
