/**
 * @file	can_hw.cpp
 * @brief	CAN bus low level code
 *
 * todo: this file should be split into two - one for CAN transport level ONLY and
 * another one with actual messages
 *
 * @date Dec 11, 2013
 * @author Andrey Belomutskiy, (c) 2012-2018
 */

#include "global.h"

#if EFI_CAN_SUPPORT

#include "engine_configuration.h"
#include "pin_repository.h"
#include "can_hw.h"
#include "string.h"
#include "obd2.h"
#include "mpu_util.h"
#include "allsensors.h"
#include "vehicle_speed.h"
#include "sensor_reader.h"
#include "fuel_math.h"

EXTERN_ENGINE
;

static int canReadCounter = 0;
static int canWriteOk = 0;
static int canWriteNotOk = 0;
static bool isCanEnabled = false;
static LoggingWithStorage logger("CAN driver");
static THD_WORKING_AREA(canTreadStack, UTILITY_THREAD_STACK_SIZE);

// Values below calculated with http://www.bittiming.can-wiki.info/
// Pick ST micro bxCAN
// Clock rate of 42mhz for f4, 54mhz for f7
#ifdef STM32F4XX
// These have an 85.7% sample point
#define CAN_BTR_250 (CAN_BTR_SJW(0) | CAN_BTR_BRP(11) | CAN_BTR_TS1(10) | CAN_BTR_TS2(1))
#define CAN_BTR_500 (CAN_BTR_SJW(0) | CAN_BTR_BRP(5)  | CAN_BTR_TS1(10) | CAN_BTR_TS2(1))
#define CAN_BTR_1k0 (CAN_BTR_SJW(0) | CAN_BTR_BRP(2)  | CAN_BTR_TS1(10) | CAN_BTR_TS2(1))
#elif defined(STM32F7XX)
// These have an 88.9% sample point
#define CAN_BTR_250 (CAN_BTR_SJW(0) | CAN_BTR_BRP(11) | CAN_BTR_TS1(14) | CAN_BTR_TS2(1))
#define CAN_BTR_500 (CAN_BTR_SJW(0) | CAN_BTR_BRP(5)  | CAN_BTR_TS1(14) | CAN_BTR_TS2(1))
#define CAN_BTR_1k0 (CAN_BTR_SJW(0) | CAN_BTR_BRP(2)  | CAN_BTR_TS1(14) | CAN_BTR_TS2(1))
#else
#error Please define CAN BTR settings for your MCU!
#endif

/*
 * 500KBaud
 * automatic wakeup
 * automatic recover from abort mode
 * See section 22.7.7 on the STM32 reference manual.
 * 
 * 29 bit would be CAN_TI0R_EXID (?) but we do not mention it here
 * CAN_TI0R_STID "Standard Identifier or Extended Identifier"? not mentioned as well
 */

static const CANConfig canConfig250 = {
CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
CAN_BTR_250 };

static const CANConfig canConfig500 = {
CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
CAN_BTR_500 };

static const CANConfig canConfig1000 = {
CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP,
CAN_BTR_1k0 };


static CANRxFrame rxBuffer;
CANTxFrame txmsg;

static void printPacket(CANRxFrame *rx) {
//	scheduleMsg(&logger, "CAN FMI %x", rx->FMI);
//	scheduleMsg(&logger, "TIME %x", rx->TIME);
	scheduleMsg(&logger, "Got CAN message: SID %x/%x %x %x %x %x %x %x %x %x", rx->SID, rx->DLC, rx->data8[0], rx->data8[1],
			rx->data8[2], rx->data8[3], rx->data8[4], rx->data8[5], rx->data8[6], rx->data8[7]);

	if (rx->SID == CAN_BMW_E46_CLUSTER_STATUS) {
		int odometerKm = 10 * (rx->data8[1] << 8) + rx->data8[0];
		int odometerMi = (int) (odometerKm * 0.621371);
		scheduleMsg(&logger, "GOT odometerKm %d", odometerKm);
		scheduleMsg(&logger, "GOT odometerMi %d", odometerMi);
		int timeValue = (rx->data8[4] << 8) + rx->data8[3];
		scheduleMsg(&logger, "GOT time %d", timeValue);
	}
}

static void setShortValue(CANTxFrame *txmsg, int value, int offset) {
	txmsg->data8[offset] = value;
	txmsg->data8[offset + 1] = value >> 8;
}

void setTxBit(int offset, int index) {
	txmsg.data8[offset] = txmsg.data8[offset] | (1 << index);
}

void commonTxInit(int eid) {
	memset(&txmsg, 0, sizeof(txmsg));
	txmsg.IDE = CAN_IDE_STD;
	txmsg.EID = eid;
	txmsg.RTR = CAN_RTR_DATA;
	txmsg.DLC = 8;
}

/**
 * send CAN message from txmsg buffer
 */
static void sendCanMessage2(int size) {
	CANDriver *device = detectCanDevice(CONFIG(canRxPin),
			CONFIG(canTxPin));
	if (device == NULL) {
		warning(CUSTOM_ERR_CAN_CONFIGURATION, "CAN configuration issue");
		return;
	}
	txmsg.DLC = size;
	// 1 second timeout
	msg_t result = canTransmit(device, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(1000));
	if (result == MSG_OK) {
		canWriteOk++;
	} else {
		canWriteNotOk++;
	}
}

/**
 * send CAN message from txmsg buffer, using default packet size
 */
void sendCanMessage() {
	sendCanMessage2(8);
}

static void canDashboardBMW(void) {
	//BMW Dashboard
	commonTxInit(CAN_BMW_E46_SPEED);
	setShortValue(&txmsg, 10 * 8, 1);
	sendCanMessage();

	commonTxInit(CAN_BMW_E46_RPM);
	setShortValue(&txmsg, (int) (GET_RPM() * 6.4), 2);
	sendCanMessage();

	commonTxInit(CAN_BMW_E46_DME2);
	setShortValue(&txmsg, (int) ((getCoolantTemperature() + 48.373) / 0.75), 1);
	sendCanMessage();
}

static void canMazdaRX8(void) {
	commonTxInit(CAN_MAZDA_RX_STEERING_WARNING);
	// todo: something needs to be set here? see http://rusefi.com/wiki/index.php?title=Vehicle:Mazda_Rx8_2004
	sendCanMessage();

	commonTxInit(CAN_MAZDA_RX_RPM_SPEED);

	float kph = getVehicleSpeed();

	setShortValue(&txmsg, SWAP_UINT16(GET_RPM() * 4), 0);
	setShortValue(&txmsg, 0xFFFF, 2);
	setShortValue(&txmsg, SWAP_UINT16((int )(100 * kph + 10000)), 4);
	setShortValue(&txmsg, 0, 6);
	sendCanMessage();

	commonTxInit(CAN_MAZDA_RX_STATUS_1);
	txmsg.data8[0] = 0xFE; //Unknown
	txmsg.data8[1] = 0xFE; //Unknown
	txmsg.data8[2] = 0xFE; //Unknown
	txmsg.data8[3] = 0x34; //DSC OFF in combo with byte 5 Live data only seen 0x34
	txmsg.data8[4] = 0x00; // B01000000; // Brake warning B00001000;  //ABS warning
	txmsg.data8[5] = 0x40; // TCS in combo with byte 3
	txmsg.data8[6] = 0x00; // Unknown
	txmsg.data8[7] = 0x00; // Unused
	sendCanMessage();

	commonTxInit(CAN_MAZDA_RX_STATUS_2);
	txmsg.data8[0] = (uint8_t)(getCoolantTemperature() + 69); //temp gauge //~170 is red, ~165 last bar, 152 centre, 90 first bar, 92 second bar
	txmsg.data8[1] = ((int16_t)(engine->engineState.vssEventCounter*(engineConfiguration->vehicleSpeedCoef*0.277*2.58))) & 0xff;
	txmsg.data8[2] = 0x00; // unknown
	txmsg.data8[3] = 0x00; //unknown
	txmsg.data8[4] = 0x01; //Oil Pressure (not really a gauge)
	txmsg.data8[5] = 0x00; //check engine light
	txmsg.data8[6] = 0x00; //Coolant, oil and battery
	if ((GET_RPM()>0) && (engine->sensors.vBatt<13)) {
		setTxBit(6, 6); // battery light
	}
	if (getCoolantTemperature() > 105) {
		setTxBit(6, 1); // coolant light, 101 - red zone, light means its get too hot
	}
	//oil pressure warning lamp bit is 7
	txmsg.data8[7] = 0x00; //unused
	sendCanMessage();
}

static void canDashboardFiat(void) {
	//Fiat Dashboard
	commonTxInit(CAN_FIAT_MOTOR_INFO);
	setShortValue(&txmsg, (int) (getCoolantTemperature() - 40), 3); //Coolant Temp
	setShortValue(&txmsg, GET_RPM() / 32, 6); //RPM
	sendCanMessage();
}

static void canDashboardVAG(void) {
	//VAG Dashboard
	commonTxInit(CAN_VAG_RPM);
	setShortValue(&txmsg, GET_RPM() * 4, 2); //RPM
	sendCanMessage();

	commonTxInit(CAN_VAG_CLT);
	setShortValue(&txmsg, (int) ((getCoolantTemperature() + 48.373) / 0.75), 1); //Coolant Temp
	sendCanMessage();

	commonTxInit(CAN_VAG_CLT_V2);
		setShortValue(&txmsg, (int) ((getCoolantTemperature() + 48.373) / 0.75), 4); //Coolant Temp
		sendCanMessage();

	commonTxInit(CAN_VAG_IMMO);
		setShortValue(&txmsg, 0x80, 1);
		sendCanMessage();
}

// https://www.aemelectronics.com/files/pdf/AEMNet%20170116_Public.pdf
static void canAem() {
	struct msg01f0a000 {
		uint16_t engineSpeed;
		uint16_t deprecated;
		uint16_t throttlePosition;
		int8_t iat;
		int8_t clt;
	};

	int rpm = GET_RPM();

	{
		commonTxInit(CAN_AEM_0);

		auto msg = reinterpret_cast<msg01f0a000*>(txmsg.data8);

		msg->engineSpeed = rpm * 0.39063f;
		msg->throttlePosition = 65536 * (getTPS() / 100.0f);
		msg->iat = getIntakeAirTemperature();
		msg->clt = getCoolantTemperature();

		sendCanMessage();
	}

	struct msg01f0a003 {
		uint8_t lambda1;
		uint8_t lambda2;
		uint16_t vss;
		uint8_t gear;
		uint8_t timing;
		uint16_t battery;
	};

	{
		commonTxInit(CAN_AEM_3);

		auto msg = reinterpret_cast<msg01f0a003*>(txmsg.data8);

		msg->lambda1 = 256 * ((getAfr() / 14.7f) - 0.5f);
		msg->vss = getVehicleSpeed() * 0.6213f;	// mph
		msg->timing = engine->engineState.timingAdvance;
		msg->battery = getVBatt() * 4073.32f;

		sendCanMessage();
	}

	struct msg01f0a004 {
		uint16_t map;
		uint8_t ve;
		uint8_t fuelPressure;
		uint8_t oilPressure;
		uint8_t lambdaTarget;
	};

	{
		commonTxInit(CAN_AEM_4);

		auto msg = reinterpret_cast<msg01f0a004*>(txmsg.data8);

		msg->map = getMap() * 10;
		msg->ve = engine->engineState.currentRawVE;

		SensorReader<SensorType::OilPressure> oilp(0);
		msg->oilPressure = oilp.getOrDefault() / 4;
		msg->lambdaTarget = engine->engineState.targetAFR; 

		sendCanMessage();
	}

	struct msg01f0a005 {
		uint16_t launchRampTime;
		uint16_t massAirFlow;
		uint16_t massAirFlowPerRev;
	};

	{
		commonTxInit(CAN_AEM_5);

		auto msg = reinterpret_cast<msg01f0a005*>(txmsg.data8);

		msg->massAirFlow = engine->engineState.airFlow * 20;
		msg->massAirFlowPerRev = engine->engineState.sd.airMassInOneCylinder * 2000;

		sendCanMessage();
	}

	struct msg01f0a006 {
		uint8_t inj1InjectorPulse;
		uint8_t inj1LambdaFeedback;
		uint8_t injectorDuty;
		uint8_t modeSw;
		uint8_t waterPressure;
		uint8_t panPressure;
		uint16_t estimatedTorque;
	};

	{
		commonTxInit(CAN_AEM_6);

		auto msg = reinterpret_cast<msg01f0a006*>(txmsg.data8);

		msg->inj1InjectorPulse = engine->actualLastInjection * 10;
		msg->injectorDuty = getInjectorDutyCycle(rpm);
		
		sendCanMessage();
	}
}

static void canInfoNBCBroadcast(can_nbc_e typeOfNBC) {
	switch (typeOfNBC) {
	case CAN_BUS_NBC_BMW:
		canDashboardBMW();
		break;
	case CAN_BUS_NBC_FIAT:
		canDashboardFiat();
		break;
	case CAN_BUS_NBC_VAG:
		canDashboardVAG();
		break;
	case CAN_BUS_MAZDA_RX8:
		canMazdaRX8();
		break;
	case CAN_BUS_NBC_AEM:
		canAem();
		break;
	default:
		break;
	}
}

static void canRead(void) {
	CANDriver *device = detectCanDevice(CONFIG(canRxPin),
			CONFIG(canTxPin));
	if (device == NULL) {
		warning(CUSTOM_ERR_CAN_CONFIGURATION, "CAN configuration issue");
		return;
	}
//	scheduleMsg(&logger, "Waiting for CAN");
	msg_t result = canReceive(device, CAN_ANY_MAILBOX, &rxBuffer, TIME_MS2I(1000));
	if (result == MSG_TIMEOUT) {
		return;
	}

	canReadCounter++;
	printPacket(&rxBuffer);
	obdOnCanPacketRx(&rxBuffer);
}

static void writeStateToCan(void) {
	canInfoNBCBroadcast(engineConfiguration->canNbcType);
}

static msg_t canThread(void *arg) {
	(void)arg;
	chRegSetThreadName("CAN");
	while (true) {
		if (engineConfiguration->canWriteEnabled)
			writeStateToCan();

		if (engineConfiguration->canReadEnabled)
			canRead(); // todo: since this is a blocking operation, do we need a separate thread for 'write'?

		if (engineConfiguration->canSleepPeriodMs < 10) {
			warning(CUSTOM_OBD_LOW_CAN_PERIOD, "%d too low CAN", engineConfiguration->canSleepPeriodMs);
			engineConfiguration->canSleepPeriodMs = 50;
		}

		chThdSleepMilliseconds(engineConfiguration->canSleepPeriodMs);
	}
#if defined __GNUC__
	return -1;
#endif
}

static void canInfo(void) {
	if (!isCanEnabled) {
		scheduleMsg(&logger, "CAN is not enabled, please enable & restart");
		return;
	}

	scheduleMsg(&logger, "CAN TX %s", hwPortname(CONFIG(canTxPin)));
	scheduleMsg(&logger, "CAN RX %s", hwPortname(CONFIG(canRxPin)));
	scheduleMsg(&logger, "type=%d canReadEnabled=%s canWriteEnabled=%s period=%d", engineConfiguration->canNbcType,
			boolToString(engineConfiguration->canReadEnabled), boolToString(engineConfiguration->canWriteEnabled),
			engineConfiguration->canSleepPeriodMs);

	scheduleMsg(&logger, "CAN rx_cnt=%d/tx_ok=%d/tx_not_ok=%d", canReadCounter, canWriteOk, canWriteNotOk);
}

void setCanType(int type) {
	engineConfiguration->canNbcType = (can_nbc_e)type;
	canInfo();
}

#if EFI_TUNER_STUDIO
void postCanState(TunerStudioOutputChannels *tsOutputChannels) {
	tsOutputChannels->debugIntField1 = isCanEnabled ? canReadCounter : -1;
	tsOutputChannels->debugIntField2 = isCanEnabled ? canWriteOk : -1;
	tsOutputChannels->debugIntField3 = isCanEnabled ? canWriteNotOk : -1;
}
#endif /* EFI_TUNER_STUDIO */

void enableFrankensoCan(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	CONFIG(canTxPin) = GPIOB_6;
	CONFIG(canRxPin) = GPIOB_12;
	engineConfiguration->canReadEnabled = false;
}

void stopCanPins(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	brain_pin_markUnused(activeConfiguration.canTxPin);
	brain_pin_markUnused(activeConfiguration.canRxPin);
}

void startCanPins(DECLARE_ENGINE_PARAMETER_SIGNATURE) {
	efiSetPadMode("CAN TX", CONFIG(canTxPin), PAL_MODE_ALTERNATE(EFI_CAN_TX_AF));
	efiSetPadMode("CAN RX", CONFIG(canRxPin), PAL_MODE_ALTERNATE(EFI_CAN_RX_AF));
}

void initCan(void) {
	isCanEnabled = (CONFIG(canTxPin) != GPIO_UNASSIGNED) && (CONFIG(canRxPin) != GPIO_UNASSIGNED);
	if (isCanEnabled) {
		if (!isValidCanTxPin(CONFIG(canTxPin)))
			firmwareError(CUSTOM_OBD_70, "invalid CAN TX %s", hwPortname(CONFIG(canTxPin)));
		if (!isValidCanRxPin(CONFIG(canRxPin)))
			firmwareError(CUSTOM_OBD_70, "invalid CAN RX %s", hwPortname(CONFIG(canRxPin)));
	}

	addConsoleAction("caninfo", canInfo);
	if (!isCanEnabled)
		return;

#if STM32_CAN_USE_CAN2
	// CAN1 is required for CAN2
	canStart(&CAND1, &canConfig500);
	canStart(&CAND2, &canConfig500);
#else
	canStart(&CAND1, &canConfig500);
#endif /* STM32_CAN_USE_CAN2 */

	chThdCreateStatic(canTreadStack, sizeof(canTreadStack), NORMALPRIO, (tfunc_t)(void*) canThread, NULL);

	startCanPins();

}

#endif /* EFI_CAN_SUPPORT */
