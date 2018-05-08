/**
 * @file	poten.c
 * @brief	MCP42010 digital potentiometer driver
 *
 * @date Mar 16, 2013
 * @author Andrey Belomutskiy, (c) 2012-2018
 */

#include "main.h"
#include "poten.h"
#include "eficonsole.h"
#include "pin_repository.h"
#include "engine_configuration.h"
#include "hardware.h"
#include "mpu_util.h"

/**
 * MCP42010 digital potentiometer driver
 *
 *
 * 1	CS		pin select						PB12		PA10
 * 2	SCK		serial clock					PA5			PC10
 * 3	SI		serial input		(MOSI)		PA7			PC12
 * 4	Vss 	ground
 * 5	PB1
 * 6	PW1
 * 7	PA1
 * 8	PA0
 * 9	PW0
 * 10	PB0
 * 11	RS		Reset
 *
 * 14	Vdd 	V input
 *
 * Rwa = 10000 * (256 - d) / 256 + 52
 * d = 256 - (Rwa - 52) * 256 / 10000
 *
 */

/* Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).*/
#define SPI_POT_CONFIG SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_DFF

static Logging * logger;

#if EFI_POTENTIOMETER || defined(__DOXYGEN__)
static Mcp42010Driver potConfig[DIGIPOT_COUNT];

void initPotentiometer(Mcp42010Driver *driver, SPIDriver *spi, brain_pin_e csPin) {
	driver->spiConfig.cr1 = SPI_POT_CONFIG;
	driver->spi = spi;
	initSpiCs(&driver->spiConfig, csPin);
}

static int getPotStep(int resistanceWA) {
	return 256 - (int) ((resistanceWA - 52) * 256 / 10000);
}

static void sendToPot(Mcp42010Driver *driver, int channel, int value) {
	lockSpi(SPI_NONE);
	spiStart(driver->spi, &driver->spiConfig);
	spiSelect(driver->spi);
	int word = (17 + channel) * 256 + value;
	spiSend(driver->spi, 1, &word);
	spiUnselect(driver->spi);
	spiStop(driver->spi);
	unlockSpi();
}

void setPotResistance(Mcp42010Driver *driver, int channel, int resistance) {
	int value = getPotStep(resistance);

	scheduleMsg(logger, "Sending to potentiometer%d: %d for R=%d", channel, value, resistance);
	sendToPot(driver, channel, value);
}

static void setPotResistanceCommand(int index, int value) {
	setPotResistance(&potConfig[index / 2], index % 2, value);
}

static void setPotValue1(int value) {
	sendToPot(&potConfig[0], 1, value);
}

#endif /* EFI_POTENTIOMETER */

void initPotentiometers(Logging *sharedLogger, board_configuration_s *boardConfiguration) {
	logger = sharedLogger;
#if EFI_POTENTIOMETER
	if (boardConfiguration->digitalPotentiometerSpiDevice == SPI_NONE) {
		scheduleMsg(logger, "digiPot spi disabled");
		return;
	}
	turnOnSpi(boardConfiguration->digitalPotentiometerSpiDevice);

	for (int i = 0; i < DIGIPOT_COUNT; i++) {
		brain_pin_e csPin = boardConfiguration->digitalPotentiometerChipSelect[i];
		if (csPin == GPIO_UNASSIGNED) {
			continue;
                }

		initPotentiometer(&potConfig[i], getSpiDevice(boardConfiguration->digitalPotentiometerSpiDevice),
				csPin);
	}

	addConsoleActionII("pot", setPotResistanceCommand);

	addConsoleActionI("potd1", setPotValue1);

	setPotResistance(&potConfig[0], 0, 3000);
	setPotResistance(&potConfig[0], 1, 7000);
#else
	print("digiPot logic disabled\r\n");
#endif
}
