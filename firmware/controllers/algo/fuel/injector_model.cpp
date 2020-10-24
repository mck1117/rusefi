#include "injector_model.h"
#include "map.h"

EXTERN_ENGINE;

void InjectorModelBase::prepare() {
	m_massFlowRate = getInjectorMassFlowRate();
	float deadtime = getDeadtime();
	m_deadtime = deadtime;

	postState(deadtime);
}

constexpr float convertToGramsPerSecond(float ccPerMinute) {
	float ccPerSecond = ccPerMinute / 60;
	return ccPerSecond * 0.72f;	// 0.72g/cc fuel density
}

constexpr float adjustInjectorFlow(float referenceFlow, float referencePressure, float actualPressure) {
	// Flow thru an orifice is proportional to the square root of the pressure ratio
	float flowRatio = sqrtf(actualPressure / referencePressure);

	return referenceFlow * flowRatio;
}

float InjectorModel::getInjectorDifferentialPressure() const {
	float intakePressure = getMap(PASS_ENGINE_PARAMETER_SIGNATURE);
	float railPressure = 0;
	
	auto mode = CONFIG(injectorCompensationMode);

	if (mode == InjectorComp_FixedRail) {
		// In case of fixed rail pressure, the rail pressure is the ref pressure + 1 atm
		railPressure = CONFIG(injectorReferencePressure) + 101.325f;
	} else if (mode == InjectorComp_UseSensor) {
		// TODO: handle failed sensor?
		railPressure = Sensor::get(SensorType::FuelPressureInjector).value_or(0);
	}

	return railPressure - intakePressure;
}

float InjectorModel::getInjectorMassFlowRate() const {
	// TODO: injector flow dependent upon rail pressure (and temperature/ethanol content?)
	auto injectorVolumeFlow = CONFIG(injector.flow);
	auto referenceMassFlow = convertToGramsPerSecond(injectorVolumeFlow);

	if (CONFIG(injectorCompensationMode) == InjectorComp_None) {
		// If no sensor, just return the reference flow.
		return referenceMassFlow;
	}

	float actualPressure = getInjectorDifferentialPressure();

	return adjustInjectorFlow(
		referenceMassFlow,
		CONFIG(injectorReferencePressure),
		actualPressure
	);
}

float InjectorModel::getDeadtime() const {
	return interpolate2d(
		"lag",
		ENGINE(sensors.vBatt),
		engineConfiguration->injector.battLagCorrBins,
		engineConfiguration->injector.battLagCorr
	);
}

void InjectorModel::postState(float deadtime) const {
	engine->engineState.running.injectorLag = deadtime;
}

float InjectorModelBase::getInjectionDuration(float fuelMassGram) const {
	// TODO: support injector nonlinearity correction

	floatms_t baseDuration = fuelMassGram / m_massFlowRate * 1000;
	return baseDuration + m_deadtime;
}
