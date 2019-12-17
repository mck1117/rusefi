/**
 * @file	stepper.h
 *
 * @date Dec 24, 2014
 * @author Andrey Belomutskiy, (c) 2012-2018
 */
#ifndef STEPPER_H_
#define STEPPER_H_

#include "global.h"
#include "efi_gpio.h"
#include "backup_ram.h"

class StepperMotor {
public:
	StepperMotor();
	void initialize(brain_pin_e stepPin, brain_pin_e directionPin, pin_output_mode_e directionPinMode, float reactionTime, int totalSteps,
			brain_pin_e enablePin, pin_output_mode_e enablePinMode, Logging *sharedLogger);
	void pulse();
	void setTargetPosition(int targetPosition);
	int getTargetPosition() const;
	void setDirection(bool isIncrementing);

	OutputPin directionPin, stepPin, enablePin;
	int currentPosition;
	bool currentDirection;
	float reactionTime;
	int totalSteps;
private:
	int targetPosition;

	pin_output_mode_e directionPinMode, stepPinMode, enablePinMode;

	THD_WORKING_AREA(stThreadStack, UTILITY_THREAD_STACK_SIZE);
};

#endif /* STEPPER_H_ */
