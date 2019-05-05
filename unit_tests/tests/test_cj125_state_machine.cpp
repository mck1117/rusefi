/**
 * test_cj125_state_machine.cpp
 * 
 *  Created on 23 January 2019
 * @author Matthew Kennedy, (c) 2019
 */

#include "efifeatures.h"
#include "gtest/gtest.h"

#include "global.h"
#include "cj125_new.h"

class Cj125HeaterStateMachineTest : public ::testing::Test {};

TEST_F(Cj125HeaterStateMachineTest, Disabled)
{
    Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Disabled, 0, 0, false, e);
    EXPECT_EQ(newState, Cj125_new::State::Disabled);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, IdleStay)
{
    Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // Engine isn't turning, should stay in idle.
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Idle, 0, 0, false, e);
    EXPECT_EQ(newState, Cj125_new::State::Idle);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, IdleTransitionPreheat)
{
    Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // Engine is turning, should transition to preheat.
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Idle, 0, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Preheat);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, PreheatStay)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In preheat, but not timed out yet
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Preheat, 0, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Preheat);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, PreheatTransitionIdle)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In preheat, but engine stopped, return to idle
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Preheat, 0, 0, false, e);
    EXPECT_EQ(newState, Cj125_new::State::Idle);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, PreheatTransitionWarmup)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In preheat, time is up, transition to warmup
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Preheat, 0, US2NT(1e6 * 6), true, e);
    EXPECT_EQ(newState, Cj125_new::State::Warmup);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, WarmupStay)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, but not warm yet
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Warmup, 3.0f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Warmup);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, WarmupTransitionIdle)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, transition to engine off
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Warmup, 3.0f, 0, false, e);
    EXPECT_EQ(newState, Cj125_new::State::Idle);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, WarmupTransitionError)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, heater timed out, transition to error
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Warmup, 3.0f, US2NT(1e6 * 40), true, e);
    EXPECT_EQ(newState, Cj125_new::State::Error);
    EXPECT_EQ(e, Cj125_new::ErrorType::HeaterTimeout);
}

TEST_F(Cj125HeaterStateMachineTest, WarmupTransitionRunning)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, sensor is up to temp, transition to running
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Warmup, 1.5f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Running);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, RunningStay)
{
        Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, heater timed out, transition to error
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Running, 1.5f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Running);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, RunningOverheat)
{
    Cj125_new::ErrorType e;
    // In warmup, heater timed out, transition to error
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Running, 0.1f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Error);
    EXPECT_EQ(e, Cj125_new::ErrorType::Overheat);
}

TEST_F(Cj125HeaterStateMachineTest, RunningUnderheat)
{
    Cj125_new::ErrorType e;
    // In warmup, heater timed out, transition to error
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Running, 3.0f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Error);
    EXPECT_EQ(e, Cj125_new::ErrorType::Underheat);
}

TEST_F(Cj125HeaterStateMachineTest, RunningTransitionIdle)
{
    Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // In warmup, heater timed out, transition to error
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Running, 1.5f, 0, false, e);
    EXPECT_EQ(newState, Cj125_new::State::Idle);
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}

TEST_F(Cj125HeaterStateMachineTest, ErrorStay)
{
    Cj125_new::ErrorType e = Cj125_new::ErrorType::None;
    // Stay in error state
    Cj125_new::State newState = Cj125_new::TransitionFunction(Cj125_new::State::Error, 1.5f, 0, true, e);
    EXPECT_EQ(newState, Cj125_new::State::Error);

    // Error should be unaltered once we're in the error state
    EXPECT_EQ(e, Cj125_new::ErrorType::None);
}
