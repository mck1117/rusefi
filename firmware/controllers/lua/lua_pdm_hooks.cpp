#include "pch.h"

#include "rusefi_lua.h"
#include "lua_hooks.h"

class Flasher {
public:
	Flasher(float period) : m_halfPeriod(period / 2) { }

	void reset() {
		m_state = false;
		m_timer.reset();
	}

	bool get() {
		// Every half period, reset the timer and toggle state
		if (m_timer.hasElapsedSec(m_halfPeriod)) {
			m_timer.reset();

			m_state = !m_state;
		}

		return m_state;
	}

private:
	bool m_state = false;
	Timer m_timer;
	const float m_halfPeriod;
};

static OutputPin pdmPins[8];

void configurePdmHooks(lua_State* l) {
	// Turn signals maybe?
	LuaClass<Flasher> luaFlasher(l, "Flasher");
	luaFlasher
		.ctor<float>()
		.fun("reset", &Flasher::reset)
		.fun("get", &Flasher.get);

	lua_register(l, "setPdm", [](lua_State* l) {
		int channel = luaL_checkinteger(l, 1);
		int value = luaL_checkinteger(l, 2);

		if (channel < 0 || channel >= PDM_COUNT) {
			luaL_error(l, "Unexpected PDM channel %d", channel);
		}

		// Conform to boolean
		bool bValue = value != 0;

		pdmPins[channel].setValue(bValue);
	});

	// Initialize all PDM pins by default
	for (size_t i = 0; i < efi::size(pdmPins); i++) {
		pdmPins[i].initPin("pdm", GPIOI_0 + i);
	}
}

void luaDeInitPdmPins() {
	for (size_t i = 0; i < efi::size(pdmPins); i++) {
		pdmPins[i].deInit();
	}
}
