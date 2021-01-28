#include "init.h"
#include "adc_inputs.h"
#include "adc_subscription.h"
#include "engine.h"
#include "error_handling.h"
#include "global.h"
#include "function_pointer_sensor.h"
#include "ego.h"
#include "linear_func.h"
#include "functional_sensor.h"

EXTERN_ENGINE;

struct GetAfrWrapper {
	DECLARE_ENGINE_PTR;

	float getLambda() {
		return getAfr(PASS_ENGINE_PARAMETER_SIGNATURE) / 14.7f;
	}
};

static GetAfrWrapper afrWrapper;

static FunctionPointerSensor lambdaSensor(SensorType::Lambda1,
[]() {
	return afrWrapper.getLambda();
});

#if EFI_CAN_SUPPORT
#include "AemXSeriesLambda.h"
static AemXSeriesWideband aem1(0, SensorType::Lambda1);
static AemXSeriesWideband aem2(1, SensorType::Lambda2);
#endif

static LinearFunc analogLambdaFunc;
FunctionalSensor analogLambdaSensor1(SensorType::Lambda1, MS2NT(100));

static bool configureAnalogLambda(LinearFunc& func, FunctionalSensor& sensor, afr_sensor_s& cfg, adc_channel_e channel) {
	if (CONFIG(afr_type) == ES_NarrowBand) {
		// handle narrowband later...
		return false;
	}

	if (!isAdcChannelValid(channel)) {
		return false;
	}

	func.configure(cfg.v1, cfg.value1 / 14.7f, cfg.v2, cfg.value2 / 14.7f, 0.5f, 2.0f);

	sensor.setFunction(func);

	AdcSubscription::SubscribeSensor(sensor, channel, 10);

	return true;
}

void initLambda(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	INJECT_ENGINE_REFERENCE(&afrWrapper);

#if EFI_CAN_SUPPORT
	if (CONFIG(enableAemXSeries)) {
		registerCanSensor(aem1);
		registerCanSensor(aem2);

		return;
	}
#endif

	if (configureAnalogLambda(analogLambdaFunc, analogLambdaSensor1, CONFIG(afr), CONFIG(afr.hwChannel))) {
		// Analog configured, don't need legacy
		return;
	}

	lambdaSensor.Register();
}
