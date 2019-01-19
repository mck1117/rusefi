#pragma once

#include "TriggerResult.h"

typedef void (*ShaftPositionListenerNew)(const TriggerResult& triggerState DECLARE_ENGINE_PARAMETER_SUFFIX);

void addTriggerEventListenerNew(ShaftPositionListenerNew handler, const char *name, Engine *engine);
