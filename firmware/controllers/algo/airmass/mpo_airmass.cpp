#include "global.h"
#include "engine.h"
#include "mpo_airmass.h"

EXTERN_ENGINE;

float MpoAirmass::estimateThrottleFlow(int rpm, float airTemp) const {
	// Estimate MAP at the current throttle position
	float estThrottleMap = m_mapEstimationTable->getValue(rpm, TPS_2_BYTE_PACKING_MULT * Sensor::get(SensorType::Tps1).value_or(0));
	float estThrottleVe = getVe(rpm, estThrottleMap);
	float estThrottleAirmass = getAirmassImpl(
			estThrottleVe,
			estThrottleMap,
			airTemp
			PASS_ENGINE_PARAMETER_SUFFIX
		);

	// Estimate the total mass flow based on that airmass
	float massPerRev = estThrottleAirmass * CONFIG(specs.cylindersCount) / 2;
	return massPerRev * rpm / 60;
}

float MpoAirmass::getFeedback(float estimatedMap, float dt) {
	float timeConstant = 0.25f;
	float integratorGain = 1 / timeConstant;

	float measuredMap = Sensor::get(SensorType::Map).value_or(0);
	float error = estimatedMap - measuredMap;

	float integrator = error * integratorGain * dt + m_integrator;
	m_integrator = integrator;

	float proportional = error * 0.05f;

	return integrator;
}

float idealGasLaw(float volume, float pressure, float temperature) {
	return volume * pressure / (AIR_R * temperature);
}

float reverseIdealGas(float mass, float volume, float temperature) {
	return AIR_R * mass * temperature / volume;
}

AirmassResult MpoAirmass::getAirmass(int rpm) const {
	float tChargeK = ENGINE(engineState.sd.tChargeK);
	if (cisnan(tChargeK)) {
		warning(CUSTOM_ERR_TCHARGE_NOT_READY2, "tChargeK not ready"); // this would happen before we have CLT reading for example
		return {};
	}

	float dt = FAST_CALLBACK_PERIOD_MS / 1000.0f;
	float manifoldVolumeM3 = 0.01f;		// 10 liters

	float throttleFlow = estimateThrottleFlow(rpm, tChargeK);

	float currentMass = m_manifoldAirMass;
	// Estimate MAP based on the mass of air in the manifold
	float estimatedMap = reverseIdealGas(currentMass, manifoldVolumeM3, tChargeK);

	throttleFlow *= getFeedback(estimatedMap, dt);

	// Now compute normal speed density using estimated map
	float ve = getVe(rpm, estimatedMap);
	float airMass = getAirmassImpl(ve, estimatedMap, tChargeK PASS_ENGINE_PARAMETER_SUFFIX);

	float portFlow = airMass * CONFIG(specs.cylindersCount) * rpm / 120;

	float netManifoldFlow = throttleFlow - portFlow;
	float massDelta = netManifoldFlow * dt;

	currentMass += massDelta;
	m_manifoldAirMass = currentMass;

	
	return {
		airMass,
		estimatedMap	// AFR/VE table Y axis
	};
}
