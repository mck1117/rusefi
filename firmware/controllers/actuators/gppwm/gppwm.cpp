
#include "global.h"
#include "engine.h"

#include "gppwm_channel.h"
#include "pwm_generator_logic.h"

EXTERN_ENGINE;

static GppwmChannel channels[4];
static OutputPin pins[4];
static SimplePwm outputs[4];
static gppwm_Map3D_t tables[4];

void initGpPwm(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	for (size_t i = 0; i < efi::size(channels); i++) {
		auto& cfg = CONFIG(gppwm)[i];

		// If no pin, don't enable this channel.
		if (cfg.pin == GPIO_UNASSIGNED) continue;

		// Determine frequency and whether PWM is enabled
		float freq = cfg.pwmFrequency;
		bool usePwm = freq > 0;

		// Setup pin & pwm
		pins[i].initPin("gp pwm", cfg.pin);
		startSimplePwm(&outputs[i], "gp pwm", &engine->executor, &pins[i], freq, 0);

		// Set up this channel's lookup table
		tables[i].init(cfg.table, cfg.loadBins, cfg.rpmBins);

		// Finally configure the channel
		channels[i].init(usePwm, &outputs[i], &tables[i], &cfg);
	}
}

void updateGppwm(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	for (size_t i = 0; i < efi::size(channels); i++) {
		channels[i].update();
	}
}
