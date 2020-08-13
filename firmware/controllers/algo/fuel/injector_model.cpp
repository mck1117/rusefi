#include "injector_model.h"
#include "map.h"

EXTERN_ENGINE;

void InjectorModelBase::prepare() {
	m_massFlowRate = getInjectorMassFlowRate();
	m_deadtime = getDeadtime();
}

constexpr float convertToGramsPerSecond(float ccPerMinute) {
	float ccPerSecond = ccPerMinute / 60;
	return ccPerSecond * 0.72f;	// 0.72g/cc fuel density
}

float getConfiguredPressure() {
	return 400;
}

float getCfgRailPressure() {
	return 350;
}

float getActualRailPressure() {
	return 500;
}

float InjectorModel::getInjectorMassFlowRate() const {
	auto configuredFlow = convertToGramsPerSecond(CONFIG(injector.flow));

	int mode = 0;

	// 0 = - directly use configured flow rate
	// 1 = - map-ref'd rail pressure, spec flow at known pressure (ie, injector is specified at some other pressure than you actually run)
	// 2 = - constant fuel rail pressure, spec flow at known pressure (ie, NB miata where the press reg has no MAP ref)
	// 3 = - sensed absolute fuel rail pressure, spec flow at known pressure (ie, GDI system with absolute fuel pressure sensor)
	// 4 = - sensed relative fuel rail pressure, spec flow at known pressure (ie, Ford with MAP-relative fuel pressure sensor)

	if (mode == 0) {
		return configuredFlow;
	}

	// The injector has the configured flow at this pressure
	float flowPressureReference = getConfiguredPressure();

	float railPressure;

	switch (mode) {
	case 1:
		railPressure = getCfgRailPressure();
		break;
	case 2:
		// Since rail pressure is constant, as MAP goes up, injector differential pressure drops
		// at 1atm, the injectors flow the configured amount
		railPressure = getCfgRailPressure() + (101.325f - getMap(PASS_ENGINE_PARAMETER_SIGNATURE));
		break;
	case 3:
		railPressure = getActualRailPressure() - getMap(PASS_ENGINE_PARAMETER_SIGNATURE);
		break;
	case 4:
		railPressure = getActualRailPressure();
		break;
	}

	// Flow rate varies with the square root of the pressure - correct flow rate based on the pressure difference
	return configuredFlow * sqrt(railPressure / flowPressureReference);
}

float InjectorModel::getDeadtime() const {
	return interpolate2d(
		"lag",
		ENGINE(sensors.vBatt),
		engineConfiguration->injector.battLagCorrBins,
		engineConfiguration->injector.battLagCorr
	);
}

float InjectorModelBase::getInjectionDuration(float fuelMassGram) const {
	// TODO: support injector nonlinearity correction

	floatms_t baseDuration = fuelMassGram / m_massFlowRate * 1000;
	return baseDuration + m_deadtime;
}
