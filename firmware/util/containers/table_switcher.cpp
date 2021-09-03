#include "pch.h"

#include "table_switcher.h"

uint8_t TableSwitcher::s_switches[] = { 0 };

static ValueProvider3D* switchTables[8] = { nullptr };

TableSwitcher::TableSwitcher(const ValueProvider3D& defaultTable, TableSwitch function)
	: m_default(defaultTable)
	, m_function(function)
{
}

float TableSwitcher::getValue(float xColumn, float yRow) const {
	auto switchIdx = s_switches[static_cast<size_t>(m_function)];

	if (switchIdx == 0) {
		return m_default.getValue(xColumn, yRow);
	}

	return switchTables[switchIdx]->getValue(xColumn, yRow);
}
