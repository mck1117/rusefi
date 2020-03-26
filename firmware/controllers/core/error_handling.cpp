/**
 * @file error_handling.cpp
 *
 * @date Apr 1, 2014
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "engine.h"
#include "os_access.h"

#if EFI_SIMULATOR || EFI_PROD_CODE
//todo: move into simulator global
#include "memstreams.h"
static MemoryStream warningStream;
static MemoryStream firmwareErrorMessageStream;
#endif /* EFI_SIMULATOR || EFI_PROD_CODE */

#define WARNING_BUFFER_SIZE 80
static char warningBuffer[WARNING_BUFFER_SIZE];
static volatile bool isWarningStreamInitialized = false;

#if EFI_HD44780_LCD
#include "lcd_HD44780.h"
#endif /* EFI_HD44780_LCD */

static LoggingWithStorage logger("error handling");

EXTERN_ENGINE;

#define WARNING_PREFIX "WARNING: "

extern int warningEnabled;
extern bool main_loop_started;

// todo: rename to fatalErrorMessage?
static fatal_msg_t errorMessageBuffer;
bool hasFirmwareErrorFlag = false;

const char *dbg_panic_file;
int dbg_panic_line;

char *getFirmwareError(void) {
	return (char*) errorMessageBuffer;
}

#if EFI_PROD_CODE

extern ioportid_t errorLedPort;
extern ioportmask_t errorLedPin;

/**
 * low-level function is used here to reduce stack usage
 */
#define ON_FATAL_ERROR() \
		palWritePad(errorLedPort, errorLedPin, 1); \
		turnAllPinsOff(); \
		enginePins.communicationLedPin.setValue(1);
#endif /* EFI_PROD_CODE */

#if EFI_SIMULATOR || EFI_PROD_CODE

void chDbgPanic3(const char *msg, const char * file, int line) {
	if (hasFatalError())
		return;
	dbg_panic_file = file;
	dbg_panic_line = line;
#if CH_DBG_SYSTEM_STATE_CHECK
	ch.dbg.panic_msg = msg;
#endif /* CH_DBG_SYSTEM_STATE_CHECK */

#if EFI_PROD_CODE
	ON_FATAL_ERROR();
#else
	printf("chDbgPanic3 %s %s%d", msg, file, line);
	exit(-1);
#endif

#if EFI_HD44780_LCD
	lcdShowPanicMessage((char *) msg);
#endif /* EFI_HD44780_LCD */

	if (!main_loop_started) {
		print("fatal %s %s:%d\r\n", msg, file, line);
//		chThdSleepSeconds(1);
		chSysHalt("Main loop did not start");
	}
}

// todo: look into chsnprintf
// todo: move to some util file & reuse for 'firmwareError' method
static void printToStream(MemoryStream *stream, const char *fmt, va_list ap) {
	stream->eos = 0; // reset
	chvprintf((BaseSequentialStream *) stream, fmt, ap);

	// Terminate, but don't write past the end of the buffer
	int terminatorLocation = minI(stream->eos, stream->size - 1);
	stream->buffer[terminatorLocation] = '\0';
}

static void printWarning(const char *fmt, va_list ap) {
	resetLogging(&logger); // todo: is 'reset' really needed here?
	appendMsgPrefix(&logger);

	logger.append(WARNING_PREFIX);

	printToStream(&warningStream, fmt, ap);

	logger.append(warningBuffer);
	append(&logger, DELIMETER);
	scheduleLogging(&logger);
}

#else
WarningCodeState unitTestWarningCodeState;

#endif /* EFI_SIMULATOR || EFI_PROD_CODE */

/**
 * OBD_PCM_Processor_Fault is the general error code for now
 *
 * @returns TRUE in case there were warnings recently
 */
bool warning(obd_code_e code, const char *fmt, ...) {
	if (hasFirmwareErrorFlag)
		return true;

#if EFI_SIMULATOR
	printf("sim_warning %s\r\n", fmt);
#endif /* EFI_SIMULATOR */

#if EFI_SIMULATOR || EFI_PROD_CODE
	if (!isWarningStreamInitialized) {
		firmwareError(CUSTOM_ERR_ASSERT, "warn stream not initialized for %d", code);
		return false;
	}
	engine->engineState.warnings.addWarningCode(code);

	// todo: move this logic into WarningCodeState?
	efitimesec_t now = getTimeNowSeconds();
	if (engine->engineState.warnings.isWarningNow(now, false) || !warningEnabled)
		return true; // we just had another warning, let's not spam
	engine->engineState.warnings.timeOfPreviousWarning = now;

	va_list ap;
	va_start(ap, fmt);
	printWarning(fmt, ap);
	va_end(ap);
#else
	// todo: we need access to 'engine' here so that we can migrate to real 'engine->engineState.warnings'
	unitTestWarningCodeState.addWarningCode(code);
	printf("unit_test_warning: ");
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\r\n");

#endif /* EFI_SIMULATOR || EFI_PROD_CODE */
	return false;
}

char *getWarning(void) {
	return warningBuffer;
}


#if EFI_CLOCK_LOCKS
uint32_t lastLockTime;
/**
 * Maximum time before requesting lock and releasing lock at the end of critical section
 */
uint32_t maxLockedDuration = 0;

/**
 * this depends on chdebug.h patch
 #if CH_DBG_SYSTEM_STATE_CHECK == TRUE
-#define _dbg_enter_lock() (ch.dbg.lock_cnt = (cnt_t)1)
-#define _dbg_leave_lock() (ch.dbg.lock_cnt = (cnt_t)0)
+#define _dbg_enter_lock() {(ch.dbg.lock_cnt = (cnt_t)1);  ON_LOCK_HOOK;}
+#define _dbg_leave_lock() {ON_UNLOCK_HOOK;(ch.dbg.lock_cnt = (cnt_t)0);}
 #endif
 */
void onLockHook(void) {
	lastLockTime = getTimeNowLowerNt();
}

void onUnlockHook(void) {
	uint32_t lockedDuration = getTimeNowLowerNt() - lastLockTime;
	if (lockedDuration > maxLockedDuration) {
		maxLockedDuration = lockedDuration;
	}
//	if (lockedDuration > 2800) {
//		// un-comment this if you want a nice stop for a breakpoint
//		maxLockedDuration = lockedDuration + 1;
//	}
}

#endif /* EFI_CLOCK_LOCKS */

/**
 * This method should be invoked really early in firmware initialization cycle.
 *
 * Implementation can only do trivial things like changing memory state. No hardware or OS access allowed
 * within this method.
 */
void initErrorHandlingDataStructures(void) {
#if EFI_SIMULATOR || EFI_PROD_CODE
	msObjectInit(&warningStream, (uint8_t *) warningBuffer, WARNING_BUFFER_SIZE, 0);
	msObjectInit(&firmwareErrorMessageStream, errorMessageBuffer, sizeof(errorMessageBuffer), 0);
#endif
	isWarningStreamInitialized = true;
}

void firmwareError(obd_code_e code, const char *fmt, ...) {
#if EFI_PROD_CODE
	if (hasFirmwareErrorFlag)
		return;
	engine->engineState.warnings.addWarningCode(code);
#ifdef EFI_PRINT_ERRORS_AS_WARNINGS
	va_list ap;
	va_start(ap, fmt);
	printWarning(fmt, ap);
	va_end(ap);
#endif
	ON_FATAL_ERROR()
	;
	hasFirmwareErrorFlag = true;
	if (indexOf(fmt, '%') == -1) {
		/**
		 * in case of simple error message let's reduce stack usage
		 * because chvprintf might be causing an error
		 */
		strncpy((char*) errorMessageBuffer, fmt, sizeof(errorMessageBuffer) - 1);
		errorMessageBuffer[sizeof(errorMessageBuffer) - 1] = 0; // just to be sure
	} else {
		// todo: look into chsnprintf once on Chibios 3
		firmwareErrorMessageStream.eos = 0; // reset
		va_list ap;
		va_start(ap, fmt);
		chvprintf((BaseSequentialStream *) &firmwareErrorMessageStream, fmt, ap);
		va_end(ap);
		// todo: reuse warning buffer helper method
		firmwareErrorMessageStream.buffer[firmwareErrorMessageStream.eos] = 0; // need to terminate explicitly
	}
#else
	printf("firmwareError [%s]\r\n", fmt);

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\r\n");

#if EFI_SIMULATOR || EFI_UNIT_TEST
	exit(-1);
#endif /* EFI_SIMULATOR */
#endif
}

