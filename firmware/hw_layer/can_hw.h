/**
 * @file	can_hw.h
 *
 * @date Dec 11, 2013
 * @author Andrey Belomutskiy, (c) 2012-2019
 */

#pragma once

#include "efifeatures.h"
#if EFI_TUNER_STUDIO
#include "tunerstudio_configuration.h"
#endif /* EFI_TUNER_STUDIO */

// CAN Bus ID for broadcast
/**
 * e46 data is from http://forums.bimmerforums.com/forum/showthread.php?1887229
 *
 * Same for Mini Cooper? http://vehicle-reverse-engineering.wikia.com/wiki/MINI
 *
 * All the below packets are using 500kb/s
 *
 */
#define CAN_BMW_E46_SPEED 0x153
#define CAN_BMW_E46_RPM 0x316
#define CAN_BMW_E46_DME2 0x329
#define CAN_BMW_E46_CLUSTER_STATUS 0x613
#define CAN_BMW_E46_CLUSTER_STATUS_2 0x615
#define CAN_FIAT_MOTOR_INFO 0x561
#define CAN_MAZDA_RX_RPM_SPEED 0x201
#define CAN_MAZDA_RX_STEERING_WARNING 0x300
#define CAN_MAZDA_RX_STATUS_1 0x212
#define CAN_MAZDA_RX_STATUS_2 0x420
// https://wiki.openstreetmap.org/wiki/VW-CAN
#define CAN_VAG_RPM 0x280
#define CAN_VAG_CLT 0x288
#define CAN_VAG_CLT_V2 0x420
#define CAN_VAG_IMMO 0x3D0

#define CAN_AEM_0 0x01F0A000
#define CAN_AEM_3 0x01F0A003
#define CAN_AEM_4 0x01F0A004
#define CAN_AEM_5 0x01F0A005
#define CAN_AEM_6 0x01F0A006

void initCan(void);
void commonTxInit(int eid);
void sendCanMessage();
void setCanType(int type);
void setTxBit(int offset, int index);

#if EFI_CAN_SUPPORT
void stopCanPins(DECLARE_ENGINE_PARAMETER_SIGNATURE);
void startCanPins(DECLARE_ENGINE_PARAMETER_SIGNATURE);
void enableFrankensoCan(DECLARE_ENGINE_PARAMETER_SIGNATURE);
#if EFI_TUNER_STUDIO
void postCanState(TunerStudioOutputChannels *tsOutputChannels);
#endif /* EFI_TUNER_STUDIO */
#endif /* EFI_CAN_SUPPORT */
