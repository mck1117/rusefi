

#include "interpolation.h"

// Cj125 pump current amplifier parameters
#define CJ125_PUMP_SHUNT_RESISTOR		(61.9f)
#define CJ125_PUMP_CURRENT_FACTOR		(1000.0f)

// Pump current, mA
static const float cjLSU49Bins[] = { 
	// LSU 4.9
	-2.0f, -1.602f, -1.243f, -0.927f, -0.8f, -0.652f, -0.405f, -0.183f, -0.106f, -0.04f, 0, 0.015f, 0.097f, 0.193f, 0.250f, 0.329f, 0.671f, 0.938f, 1.150f, 1.385f, 1.700f, 2.000f, 2.150f, 2.250f
};
// Lambda value
static const float cjLSU49Lambda[] = {
	// LSU 4.9
	0.65f, 0.7f, 0.75f, 0.8f, 0.822f, 0.85f, 0.9f, 0.95f, 0.97f, 0.99f, 1.003f, 1.01f, 1.05f, 1.1f, 1.132f, 1.179f, 1.429f, 1.701f, 1.990f, 2.434f, 3.413f, 5.391f, 7.506f, 10.119f
};

// Pump current, mA
static const float cjLSU42Bins[] = {
	// LSU 4.2
	-1.85f, -1.08f, -0.76f, -0.47f, 0.0f, 0.34f, 0.68f, 0.95f, 1.4f
};
// Lambda value
static const float cjLSU42Lambda[] = {
	// LSU 4.2
	0.7f, 0.8f, 0.85f, 0.9f, 1.009f, 1.18f, 1.43f, 1.7f, 2.42f
};

static constexpr float GetPumpCurrent(float vUaDelta, float gain)
{
    float currentPerVolt = CJ125_PUMP_CURRENT_FACTOR / CJ125_PUMP_SHUNT_RESISTOR / gain;

	float pumpCurrent = vUaDelta * currentPerVolt;

    return pumpCurrent;
}

namespace LambdaConverterLsu42
{
    float ConvertLambda(float vUaDelta, float gain)
    {
        float pumpCurrent = GetPumpCurrent(vUaDelta, gain);

        return interpolate2d("cj125Lsu49", pumpCurrent, cjLSU42Bins, cjLSU42Lambda, sizeof(cjLSU42Bins) / (sizeof(cjLSU42Bins[0])));
    }
};

namespace LambdaConverterLsu49
{
    float ConvertLambda(float vUaDelta, float gain)
    {
        float pumpCurrent = GetPumpCurrent(vUaDelta, gain);

        return interpolate2d("cj125Lsu49", pumpCurrent, cjLSU49Bins, cjLSU49Lambda, sizeof(cjLSU49Bins) / (sizeof(cjLSU49Bins[0])));
    }
};
