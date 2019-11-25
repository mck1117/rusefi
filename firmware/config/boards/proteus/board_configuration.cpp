/**
 * @file boards/proteus/board_configuration.cpp
 *
 *
 * @brief Configuration defaults for the Proteus board
 *
 * See https://github.com/mck1117/proteus
 *
 * @author Matthew Kennedy, (c) 2019
 */

#include "global.h"
#include "engine.h"
#include "engine_math.h"
#include "allsensors.h"
#include "fsio_impl.h"
#include "engine_configuration.h"
#include "efilib.h"

EXTERN_ENGINE;

static void setInjectorPins() {
	boardConfiguration->injectionPins[0] = GPIOD_7;
	boardConfiguration->injectionPins[1] = GPIOG_9;
	boardConfiguration->injectionPins[2] = GPIOG_10;
	boardConfiguration->injectionPins[3] = GPIOG_11;
	boardConfiguration->injectionPins[4] = GPIOG_12;
	boardConfiguration->injectionPins[5] = GPIOG_13;
	boardConfiguration->injectionPins[6] = GPIOG_14;
	boardConfiguration->injectionPins[7] = GPIOB_4;

	// Disable remainder
	for (int i = 8; i < efi::size(boardConfiguration->injectionPins);i++) {
		boardConfiguration->injectionPins[i] = GPIO_UNASSIGNED;
	}

	boardConfiguration->injectionPinMode = OM_DEFAULT;
}

static void setIgnitionPins() {
	boardConfiguration->ignitionPins[0] = GPIOD_4;
	boardConfiguration->ignitionPins[1] = GPIOD_3;
	boardConfiguration->ignitionPins[2] = GPIOC_9;
	boardConfiguration->ignitionPins[3] = GPIOC_8;
	boardConfiguration->ignitionPins[4] = GPIOC_7;
	boardConfiguration->ignitionPins[5] = GPIOG_8;
	boardConfiguration->ignitionPins[6] = GPIOG_7;
	boardConfiguration->ignitionPins[7] = GPIOG_6;

	// disable remainder
	for (int i = 8; i < efi::size(boardConfiguration->ignitionPins); i++) {
		boardConfiguration->ignitionPins[i] = GPIO_UNASSIGNED;
	}

	boardConfiguration->ignitionPinMode = OM_DEFAULT;
}

static void setLedPins() {
	engineConfiguration->communicationLedPin = GPIOE_4;
	engineConfiguration->runningLedPin = GPIOE_5;
	boardConfiguration->triggerErrorPin = GPIOE_6;
}

static void setupVbatt() {
	// 5.6k high side/10k low side = 1.56 ratio divider
	engineConfiguration->analogInputDividerCoefficient = 15.6f / 10.0f;

	// 47k high side/10k low side = 5.7 battery volts per ADC volt
	engineConfiguration->vbattDividerCoeff = (57.0f / 10.0f);
	//engineConfiguration->vbattAdcChannel = TODO;

	engineConfiguration->adcVcc = 3.3f;
}

static void setupEtb() {
	// TLE9201 driver
	// This chip has three control pins:
	// DIR - sets direction of the motor
	// PWM - pwm control (enable high, coast low)
	// DIS - disables motor (enable low)

	// PWM pin
	boardConfiguration->etb1.controlPin1 = GPIOD_12;
	// DIR pin
	boardConfiguration->etb1.directionPin1 = GPIOD_10;

	// set_fsio_output_pin 7 PC8
#if EFI_FSIO
	// set_rpn_expression 8 "1"
	// disable ETB by default
	setFsio(7, GPIOD_11, "1" PASS_CONFIG_PARAMETER_SUFFIX);
	// enable ETB
	// set_rpn_expression 8 "0"
	//setFsio(7, GPIOD_11, "0" PASS_CONFIG_PARAMETER_SUFFIX);
#endif /* EFI_FSIO */

	// Unused
	boardConfiguration->etb1.directionPin2 = GPIO_UNASSIGNED;

	// we only have pwm/dir, no dira/dirb
	engineConfiguration->etb1_use_two_wires = false;

	engineConfiguration->etbFreq = 800;
}

static void setupDefaultSensorInputs() {
	// trigger inputs
	boardConfiguration->triggerInputPins[0] = GPIOC_6;	// Digital (hall) #1
	boardConfiguration->triggerInputPins[1] = GPIO_UNASSIGNED;
	boardConfiguration->triggerInputPins[2] = GPIO_UNASSIGNED;
	engineConfiguration->camInputs[0] = GPIOE_11;		// Digital (hall) #2

	// CLT/IAT/aux pullups
	engineConfiguration->clt.config.bias_resistor = 2700;
	engineConfiguration->iat.config.bias_resistor = 2700;
	engineConfiguration->auxTempSensor1.config.bias_resistor = 2700;
	engineConfiguration->auxTempSensor2.config.bias_resistor = 2700;
}

void setPinConfigurationOverrides(void) {
}

void setSerialConfigurationOverrides(void) {
	boardConfiguration->useSerialPort = false;
	engineConfiguration->binarySerialTxPin = GPIO_UNASSIGNED;
	engineConfiguration->binarySerialRxPin = GPIO_UNASSIGNED;
	engineConfiguration->consoleSerialTxPin = GPIO_UNASSIGNED;
	engineConfiguration->consoleSerialRxPin = GPIO_UNASSIGNED;
}


/**
 * @brief   Board-specific configuration code overrides.
 *
 * See also setDefaultEngineConfiguration
 *
 * @todo    Add your board-specific code, if any.
 */
void setBoardConfigurationOverrides(void) {
	setInjectorPins();
	setIgnitionPins();
	setLedPins();
	setupVbatt();
	setupEtb();

	// "required" hardware is done - set some reasonable defaults
	setupDefaultSensorInputs();

	// Some sensible defaults for other options
	setOperationMode(engineConfiguration, FOUR_STROKE_CRANK_SENSOR);
	engineConfiguration->trigger.type = TT_TOOTHED_WHEEL_60_2;
	engineConfiguration->useOnlyRisingEdgeForTrigger = true;
	setAlgorithm(LM_SPEED_DENSITY PASS_CONFIG_PARAMETER_SUFFIX);

	// GM LS firing order
	engineConfiguration->specs.cylindersCount = 8;
	engineConfiguration->specs.firingOrder = FO_1_8_7_2_6_5_4_3;

	engineConfiguration->ignitionMode = IM_WASTED_SPARK;
	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_BATCH;
}

void setAdcChannelOverrides(void) {
}

/**
 * @brief   Board-specific SD card configuration code overrides. Needed by bootloader code.
 * @todo    Add your board-specific code, if any.
 */
void setSdCardConfigurationOverrides(void) {
}
