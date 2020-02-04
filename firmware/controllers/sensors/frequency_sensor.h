#pragma once

#include "functional_sensor.h"
#include "efi_gpio.h"

class FrequencySensor final : public FunctionalSensor
{
public:
	void listenPin(brain_pin_e pin);


private:
	static void callbackAdapter(void*);
	void callback();

	ioline_t m_ioLine;
};
