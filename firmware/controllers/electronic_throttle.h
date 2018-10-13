/**
 * @file	electronic_throttle.h
 *
 * @date Dec 7, 2013
 * @author Andrey Belomutskiy, (c) 2012-2017
 */

#ifndef ELECTRONIC_THROTTLE_H_
#define ELECTRONIC_THROTTLE_H_

#include "engine.h"
void initElectronicThrottle(void);
void setDefaultEtbParameters(void);
void setEtbPFactor(float value);
void setEtbIFactor(float value);
void setEtbDFactor(float value);
bool isETBRestartNeeded(void);
void stopETBPins(void);
void startETBPins(void);
void onConfigurationChangeElectronicThrottleCallback(engine_configuration_s *previousConfiguration);

#endif /* ELECTRONIC_THROTTLE_H_ */
