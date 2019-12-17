/**
 * @file settings.h
 * @brief This file is about configuring engine via the human-readable protocol
 *
 * @date Dec 30, 2012
 * @author Andrey Belomutskiy, (c) 2012-2017
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "engine.h"

void initSettings(void);
void printSpiState(Logging *logger, const engine_configuration_s *engineConfiguration);
void printConfiguration(const engine_configuration_s *engineConfiguration);
void scheduleStopEngine(void);
void setCallFromPitStop(int durationMs);
void printTPSInfo(void);
void setEngineType(int value);
/**
 * See also getEngine_type_e()
 */
const char* getConfigurationName(engine_type_e engineType);

#endif /* SETTINGS_H_ */
