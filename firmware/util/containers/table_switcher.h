#pragma once

#include "table_helper.h"

enum class TableSwitch : uint8_t {
	BoostOpenLoop = 0,
	BoostTarget = 1,

	// Update me!
	LastFunction = 2
};

class TableSwitcher : public ValueProvider3D {
public:
	TableSwitcher(const ValueProvider3D& defaultTable, TableSwitch function);
	float getValue(float xColumn, float yRow) const override;

private:
	const ValueProvider3D& m_default;
	const TableSwitch m_function;

	static uint8_t s_switches[static_cast<size_t>(TableSwitch::LastFunction)];
};
