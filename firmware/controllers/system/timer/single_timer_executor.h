/**
 * @file single_timer_executor.h
 *
 * @date: Apr 18, 2014
 * @author Andrey Belomutskiy, (c) 2012-2017
 */

#ifndef SINGLETIMEREXECUTOR_H_
#define SINGLETIMEREXECUTOR_H_

#include "scheduler.h"
#include "event_queue.h"

class SingleTimerExecutor : public ExecutorInterface {
public:
	SingleTimerExecutor();
	void scheduleByTimestamp(scheduling_s *scheduling, efitimeus_t timeUs, action_s action) override;
	void scheduleForLater(scheduling_s *scheduling, int delayUs, action_s action) override;
	void onTimerCallback();
	int timerCallbackCounter;
	int scheduleCounter;
	int doExecuteCounter;
private:
	EventQueue queue;
	bool reentrantFlag;
	
	void scheduleTimerCallback();

	void doExecute() override;
	bool enqueueTask(scheduling_s* task) override;
};

void initSingleTimerExecutorHardware(void);
void executorStatistics();

#endif /* SINGLETIMEREXECUTOR_H_ */
