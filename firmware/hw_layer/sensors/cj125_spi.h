#include "global.h"

#include "engine.h"

class Cj125Spi
{
private:
	bool m_hasSpi = false;
    const cj125_spi_config& m_config;
    SPIDriver* m_driver;
	OutputPin m_csPin;

	SPIConfig m_spiConfig;

	/**
	 * Raw register read
	 */
	uint8_t ReadRegister(uint8_t reg) const;

	/**
	 * Raw register write
	 */
	void WriteRegister(uint8_t reg, uint8_t value);

	void SetMode(uint8_t mode);
public:
	Cj125Spi(const cj125_spi_config& config);

	bool Init();
	bool Identify();
	uint8_t Diagnostic() const;

	bool BeginCalibration();
	void EndCalibration();
};
