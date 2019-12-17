#include "functional_sensor.h"

void FunctionalSensor::postRawValue(float inputValue) {
	// If no function is set, this sensor isn't valid.
	if (!m_function) {
		invalidate();
		return;
	}

	auto r = m_function->convert(inputValue);

	// This has to happen so that we set the valid bit after
	// the value is stored, to prevent the data race of reading
	// an old invalid value
	if (r.Valid) {
		setValidValue(r.Value);
	} else {
		invalidate();
	}
}
