#include "global.h"
#include "engine.h"
#include "mpo_airmass.h"
#include "tunerstudio_outputs.h"

EXTERN_ENGINE;

#define AIR_R 0.28705f

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
	return estThrottleAirmass * CONFIG(specs.cylindersCount) * rpm / (120 * 1000);
}

float MpoAirmass::getFeedback(float estimatedMap, float dt) {
	float integratorGain = CONFIG(mpoKi);

	float measuredMap = Sensor::get(SensorType::Map).value_or(0);
	float error = measuredMap - estimatedMap;

	float integrator = error * integratorGain * dt + m_integrator;
	integrator = clampF(0.1f, integrator, 10);
	m_integrator = integrator;

	float proportional = error * CONFIG(mpoKp);

	return clampF(0.01f, proportional + integrator, 100);
}

float reverseIdealGas(float mass, float volume, float temperature) {
	return AIR_R * mass * temperature / volume;
}

AirmassResult MpoAirmass::getAirmass(int rpm) {
	float tChargeK = ENGINE(engineState.sd.tChargeK);
	if (cisnan(tChargeK)) {
		warning(CUSTOM_ERR_TCHARGE_NOT_READY2, "tChargeK not ready"); // this would happen before we have CLT reading for example
		return {};
	}

	//Sensor::setMockValue(SensorType::Map, 80);

	constexpr float dt = FAST_CALLBACK_PERIOD_MS / 1000.0f;
	float manifoldVolumeM3 = CONFIG(mpoManifoldVolume) / 1000; // liters -> m^3

	float throttleFlow = estimateThrottleFlow(rpm, tChargeK);

	if (rpm == 0) {
		// If the engine is stopped, the manifold mass is simply calculated from the true MAP
		m_manifoldAirMass = idealGasLaw(
			manifoldVolumeM3,
			Sensor::get(SensorType::Map).value_or(0),
			tChargeK
		);

		m_integrator = 1;
	}

	float currentMass = m_manifoldAirMass;
	// Estimate MAP based on the mass of air in the manifold
	float estimatedMap = reverseIdealGas(currentMass, manifoldVolumeM3, tChargeK);

	float feedback = getFeedback(estimatedMap, dt);
	throttleFlow *= feedback;

	// Now compute normal speed density using estimated map
	float ve = getVe(rpm, estimatedMap);
	float airMass = getAirmassImpl(ve, estimatedMap, tChargeK PASS_ENGINE_PARAMETER_SUFFIX);

	float portFlow = airMass * CONFIG(specs.cylindersCount) * rpm / (120 * 1000);

	float netManifoldFlow = throttleFlow - portFlow;
	float massDelta = netManifoldFlow * dt;

	currentMass += massDelta;

	// clamp to non-negative manifold mass
	currentMass = maxF(0, currentMass);

	m_manifoldAirMass = currentMass;

	// Model is now updated, re-estimate port flow 
	float estimatedMap2 = reverseIdealGas(currentMass, manifoldVolumeM3, tChargeK);
	float ve2 = getVe(rpm, estimatedMap2);
	float airMass2 = getAirmassImpl(ve2, estimatedMap2, tChargeK PASS_ENGINE_PARAMETER_SUFFIX);

#if EFI_TUNER_STUDIO
	if (CONFIG(debugMode) == DBG_MANIFOLD_PRESSURE_ESTIMATE) {
		tsOutputChannels.debugFloatField1 = 1000 * throttleFlow;
		tsOutputChannels.debugFloatField2 = 1000 * portFlow;
		tsOutputChannels.debugFloatField3 = feedback;
		tsOutputChannels.debugFloatField4 = estimatedMap2;
	}
#endif

	return {
		airMass2,
		estimatedMap2	// AFR/VE table Y axis
	};
}
