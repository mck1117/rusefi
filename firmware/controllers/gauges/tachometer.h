/*
 * @file tachometer.h
 *
 * @date Aug 18, 2015
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "engine.h"

void initTachometer(DECLARE_ENGINE_PARAMETER_SIGNATURE);
void updateTachometer(int rpm DECLARE_ENGINE_PARAMETER_SUFFIX);
