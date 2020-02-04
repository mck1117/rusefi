#include "frequency_sensor.h"

#include "digital_input_exti.h"

/*static*/ void FrequencySensor::callbackAdapter(void* data)
{
	auto fs = reinterpret_cast<FrequencySensor*>(data);
}

void FrequencySensor::callback() {
	// do the time sensitive things as early as possible!
	efitick_t stamp = getTimeNowLowerNt();
	bool rise = (palReadLine(m_ioLine) == PAL_HIGH);

	// ... process edge appropriately ...

	this->postRawValue(calculatedFrequency, stamp);
}

void FrequencySensor::listenPin(brain_pin_e pin)
{
	efiExtiEnablePin("freq sensor", pin, PAL_EVENT_MODE_BOTH_EDGES, callbackAdapter, this);
	m_ioLine = PAL_LINE(getHwPort("freq sensor", pin), getHwPin("freq sensor", pin));
}
