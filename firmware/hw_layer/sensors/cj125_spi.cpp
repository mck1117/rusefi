#include "cj125_spi.h"

#include "hardware.h"

// CJ125 SPI Registers
#define	IDENT_REG_RD					0x48 // Read Identity Register
#define	INIT_REG1_WR					0x56 // Write To Initialization Register 1
#define	INIT_REG2_WR					0x5A // Write To Initialization Register 2
#define	INIT_REG1_RD					0x6C // Read Initialization Register 1
#define	DIAG_REG_RD						0x78 // Read Diagnostics Register
#define	INIT_REG2_RD					0x7E // Read Initialization Register 2

// SPI register values
#define	CJ125_INIT1_NORMAL_8			0x88 // 0b10001000 (Normal mode, Amplification 8)
#define	CJ125_INIT1_NORMAL_17			0x89 // 0b10001001 (Normal mode, Amplification 17)
#define	CJ125_INIT1_CALBRT				0x9D // 0b10011101 (Calibration mode, LA=1, RA=1)

#define	CJ125_INIT2_NORMAL				0x00 // 0b00000000, (Normal mode)
#define	CJ125_INIT2_DIAG 				0x10 // 0b00010000, (Extended diagnostics mode, SET_DIA_Q=1)
#define	CJ125_INIT2_RESET				0x40 // 0b01000000, SRESET=1

#define	CJ125_DIAG_NORM					0xFF // no errors

#define	CJ125_IDENT						0x60 // 96
#define	CJ125_IDENT_MASK 				0xF8 // Last 3 bits depend on silicon stepping


uint8_t Cj125Spi::ReadRegister(uint8_t reg) const
{
    spiSelect(m_driver);

    spiSend(m_driver, 1, &reg);
    spiReceive(m_driver, 1, &reg);

    spiUnselect(m_driver);

    return reg;
}

void Cj125Spi::WriteRegister(uint8_t reg, uint8_t value)
{
    spiSelect(m_driver);

    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;

    spiSend(m_driver, 2, buf);

    spiUnselect(m_driver);
}

Cj125Spi::Cj125Spi(const cj125_spi_config& config)
    : m_config(config)
	, m_spiConfig(
		{
			NULL,
			/* HW dependent part.*/
			NULL, 0, SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2
		})
{
}

bool Cj125Spi::Init()
{
	// Check that we have a device...
	if(m_config.spiDevice == SPI_NONE)
	{
		return false;
	}

	// ...and a CS pin
	if(m_config.csPin == GPIO_UNASSIGNED)
	{
		return false;
	}

	m_csPin.initPin("cj125 cs", m_config.csPin, &m_config.csMode);
	// CS idles high
	m_csPin.setValue(true);

	m_spiConfig.ssport = getHwPort("cj125", m_config.csPin);
	m_spiConfig.sspad = getHwPin("cj125", m_config.csPin);

	m_driver = getSpiDevice(m_config.spiDevice);
	spiStart(m_driver, &m_spiConfig);

	return true;
}

bool Cj125Spi::Identify()
{
	// read Ident register
	uint8_t ident = ReadRegister(IDENT_REG_RD) & CJ125_IDENT_MASK;
	if (CJ125_IDENT != ident) {
		//scheduleMsg(logger, "cj125: Error! Wrong ident! Cannot communicate with CJ125!");
		return false;
	}

	// set initial registers
	WriteRegister(INIT_REG1_WR, CJ125_INIT1_NORMAL_17);
	WriteRegister(INIT_REG2_WR, CJ125_INIT2_DIAG);
	// check if regs are ok
	uint8_t init1 = ReadRegister(INIT_REG1_RD);
	uint8_t init2 = ReadRegister(INIT_REG2_RD);

	//scheduleMsg(logger, "cj125: Check ident=0x%x diag=0x%x init1=0x%x init2=0x%x", ident, diag, init1, init2);

	if (init1 != CJ125_INIT1_NORMAL_17 || init2 != CJ125_INIT2_DIAG) {
		//scheduleMsg(logger, "cj125: Error! Cannot set init registers! Cannot communicate with CJ125!");
		return false;
	}

	// Check diagnostic register
	uint8_t diag = ReadRegister(DIAG_REG_RD);
	if (diag != CJ125_DIAG_NORM) {
		//scheduleMsg(logger, "cj125: Diag error!");

		return false;
	}

	return true;
}
