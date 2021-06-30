#pragma once

#include "speed_density_base.h"

class MpoAirmass : public SpeedDensityBase {
public:
	explicit MpoAirmass(const ValueProvider3D& veTable, const ValueProvider3D& mapEstimationTable)
		: SpeedDensityBase(veTable)
		, m_mapEstimationTable(&mapEstimationTable)
	{}

	AirmassResult getAirmass(int rpm) override;

private:
	float estimateThrottleFlow(int rpm, float airTemp) const;
	float getFeedback(float estimatedMap, float dt);

	const ValueProvider3D* const m_mapEstimationTable;

	float m_manifoldAirMass = 0;
	float m_integrator = 1;
};
