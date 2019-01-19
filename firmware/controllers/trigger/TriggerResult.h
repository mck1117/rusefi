#pragma once

#include "rusefi_types.h"

struct TriggerResult
{
    bool synced;
    angle_t phase;
    angle_t nextPhaseGap;
    float rpm;
};
