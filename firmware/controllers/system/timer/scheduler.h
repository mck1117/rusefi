/**
 * @file	scheduler.h
 *
 * @date May 18, 2014
 * @author Andrey Belomutskiy, (c) 2012-2019
 */
#pragma once

#include "global.h"

typedef void (*schfunc_t)(void *);

class action_s {
public:
	action_s() = default;

	// Allow implicit conversion from schfunc_t to action_s
	action_s(schfunc_t callback) : action_s(callback, nullptr) { }
	action_s(schfunc_t callback, void *param) : callback(callback), param(param) { }

	void execute();
	schfunc_t getCallback() const;
	void * getArgument() const;

private:
	schfunc_t callback = nullptr;
	void *param = nullptr;
};

/**
 * This structure holds information about an event scheduled in the future: when to execute what callback with what parameters
 */
struct scheduling_s {
#if EFI_SIGNAL_EXECUTOR_SLEEP
	virtual_timer_t timer;
#endif /* EFI_SIGNAL_EXECUTOR_SLEEP */

	/**
	 * timestamp represented as 64-bit value of ticks since MCU start
	 */
	volatile efitime_t momentX = 0;
	bool isScheduled = false;

	/**
	 * Scheduler implementation uses a sorted linked list of these scheduling records.
	 */
	scheduling_s *nextScheduling_s = nullptr;

	action_s action;
};

struct ExecutorInterface;

class SchedulingBatch {
private:
	// only called from ExecutorInterface
	SchedulingBatch(ExecutorInterface* ei) : m_executor(ei) { }
	friend class ExecutorInterface;

public:
	~SchedulingBatch();

	void scheduleByTimestamp(scheduling_s* scheduling, efitimeus_t timeUs, action_s action);
private:
	scheduling_s* m_head = nullptr;
	ExecutorInterface* m_executor;
};

struct ExecutorInterface {
	friend class SchedulingBatch;

	/**
	 * see also scheduleByAngle
	 */
	virtual void scheduleByTimestamp(scheduling_s *scheduling, efitimeus_t timeUs, action_s action) = 0;
	virtual void scheduleForLater(scheduling_s *scheduling, int delayUs, action_s action) = 0;

	SchedulingBatch startBatch() {
		return SchedulingBatch(this);
	}

protected:
	virtual bool enqueueTask(scheduling_s* scheduling) = 0;

	virtual void doExecute() = 0;
};
