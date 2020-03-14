
#include "closed_loop_fuel.h"

#include "global_shared.h"
#include "engine.h"

#include "closed_loop_fuel_cell.h"

EXTERN_ENGINE;

ClosedLoopFuelCellImpl cell;

static const closed_loop_fuel_cfg cfg =
{
    0.005f
};

float fuelClosedLoopCorrection() {
    if (!CONFIG(fuelClosedLoopCorrectionEnabled)) {
        return 1.0f;
    }

    cell.configure(&cfg);

    cell.update();
	return cell.getAdjustment();
}
