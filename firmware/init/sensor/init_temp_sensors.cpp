
#include "adc_subscription.h"
#include "converters/func_chain.h"
#include "converters/linear_func.h"
#include "converters/resistance_func.h"
#include "converters/thermistor_func.h"
#include "engine.h"
#include "engine_configuration_generated_structures.h"
#include "functional_sensor.h"
#include "tunerstudio_configuration.h"

EXTERN_ENGINE;

using therm = ThermistorFunc;
using resist = ResistanceFunc;

struct FuncPair {
	LinearFunc linear;
	FuncChain<resist, therm> thermistor;
};

static SensorConverter &configureTempSensorFunction(thermistor_conf_s &cfg, FuncPair &p, bool isLinear) {
	if (isLinear) {
		p.linear.configure(cfg.resistance_1, cfg.tempC_1, cfg.resistance_2, cfg.tempC_2, -50, 250);

		return p.linear;
	} else /* sensor is thermistor */ {
		p.thermistor.get<resist>().configure(5.0f, cfg.bias_resistor);
		p.thermistor.get<therm>().configure(cfg);

		return p.thermistor;
	}
}

static void configureTempSensor(FunctionalSensor &sensor,
								FuncPair &p,
								ThermistorConf &config,
								bool isLinear) {
	auto channel = config.adcChannel;

	// Check if channel is set - ignore this sensor if not
	if (channel == EFI_ADC_NONE) {
		return;
	}

	// Configure the conversion function for this sensor
	sensor.setFunction(configureTempSensorFunction(config.config, p, isLinear));

	// Register & subscribe
	if (!sensor.Register()) {
		// uhh?
		return;
	}

	AdcSubscription::SubscribeSensor(sensor, channel);
}

static FunctionalSensor clt(SensorType::Clt);
static FunctionalSensor iat(SensorType::Iat);
static FunctionalSensor aux1(SensorType::AuxTemp1);
static FunctionalSensor aux2(SensorType::AuxTemp2);

static FuncPair fclt, fiat, faux1, faux2;

void initTempSensors() {
	configureTempSensor(clt,
						fclt,
						engineConfiguration->clt,
						engineConfiguration->useLinearCltSensor);

	configureTempSensor(iat,
						fiat,
						engineConfiguration->iat,
						engineConfiguration->useLinearIatSensor);

	configureTempSensor(aux1,
						faux1,
						engineConfiguration->auxTempSensor1,
						false);

	configureTempSensor(aux2,
						faux2,
						engineConfiguration->auxTempSensor2,
						false);
}
