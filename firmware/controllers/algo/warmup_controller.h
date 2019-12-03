#pragma once

#include <cstdint>

// [clt] -> [initial multiplier]
// [clt] -> [hold cycles]
// [clt] -> [decay step cycles]
// [clt] -> [step multiplier]

class WarmupController
{
public:

	void configure();

    void onEngineCycle();
    void onEngineStarted();
    void reset();

    float getMultiplier() const;

private:
    //const warmup_config m_config;

    bool m_engineStart;

    float m_startupCoolantTemperature;

    uint32_t m_cycles = 0;
    float m_adjustment = 1.0f;

    uint32_t m_holdCycles = 0;
    uint32_t m_decayStepCycles = 4;
    float m_decayRatio = 0.95f;

    enum class State
    {
        Hold,
        Decay,
        Off,
    };

    State m_state = State::Off;
};
