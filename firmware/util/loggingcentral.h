/**
 * @file	loggingcentral.h
 *
 * @date Mar 8, 2015
 * @author Andrey Belomutskiy, (c) 2012-2020
 */
#pragma once

class Logging;

void startLoggingProcessor();

const char* swapOutputBuffers(size_t *actualOutputBufferSize);

namespace priv
{
	// internal implementation, use scheduleMsg below
	void efiPrintfInternal(const char *fmt, ...);
}

// "normal" logging messages need a header and footer, so put them in
// the format string at compile time
#define scheduleMsg(logging, fmt, ...) priv::efiPrintfInternal(PROTOCOL_MSG DELIMETER fmt DELIMETER, ##__VA_ARGS__)

/**
 * This is the legacy function to copy the contents of a local Logging object in to the output buffer
 */
void scheduleLogging(Logging *logging);

// Stores the result of one call to efiPrintfInternal in the queue to be copied out to the output buffer
struct LogLineBuffer {
	char buffer[128];
};

template <size_t TBufferSize>
class LogBuffer {
public:
	void writeLine(LogLineBuffer* line);
	void writeLogger(Logging* logging);

	size_t length();
	void reset();
	const char* get();

private:
	void writeInternal(const char* buffer);

	char m_buffer[TBufferSize];
	char* m_writePtr = m_buffer;
};
