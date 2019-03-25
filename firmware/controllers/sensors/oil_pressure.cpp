/**
 *  @author Matthew Kennedy, (c) 2017
 */

#include "oil_pressure.h"
#include "SensorConsumer.h"

static SensorConsumer oilpressure(SensorType::OilPressure);

float getOilPressure() {
    return oilpressure.Get().Value;
}
