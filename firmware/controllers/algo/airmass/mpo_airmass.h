#pragma once

#include "airmass.h"

class MpoAirmass : public SpeedDensityBase {
public:
	explicit SpeedDensityAirmass(const ValueProvider3D& veTable, const ValueProvider3D& mapEstimationTable)
		: SpeedDensityBase(veTable)
		, m_mapEstimationTable(&mapEstimationTable)
	{}

	AirmassResult getAirmass(int rpm) const override;

private:
	float estimateThrottleFlow(int rpm, float airTemp) const;

	const ValueProvider3D* const m_mapEstimationTable;

	float m_manifoldAirMass = 0;

	float m_integrator = 1;
};
