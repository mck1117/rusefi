
#include "MissingToothCrankDecoder.h"

#include "global.h"

std::optional<TriggerResult> TriggerDecoder::HandleTriggerEdge(trigger_event_e const signal, const efitime_t nowNt)
{
    if(this->ShouldDecodeTriggerEdge(signal))
    {
        return this->DecodeTriggerEdge(signal, nowNt);
    }
    else
    {
        return std::nullopt;
    }
}

MissingToothTriggerDecoder::MissingToothTriggerDecoder(uint8_t teeth, uint8_t missing)
    : _teeth(teeth),
      _normalTeeth(teeth - missing - 1),
      _missing(missing),
      _lastEdge(0)
{
    _normalToothAngle = 360.0f / teeth;
    _longToothAngle = _normalToothAngle * (missing + 1);

    Reset();
}

bool MissingToothTriggerDecoder::ShouldDecodeTriggerEdge(const trigger_event_e signal)
{
    // Only care about falling edge primary
    return (signal == SHAFT_PRIMARY_FALLING);
}

TriggerResult MissingToothTriggerDecoder::DecodeTriggerEdge(const trigger_event_e signal, const efitime_t nowNt)
{
    // Ensuring we only get the correct signal gets done by
    // ShouldDecodeTriggerEdge
    UNUSED(signal);
    
	TriggerResult result;

	result.synced = false;
	result.phase = 0;
	result.nextPhaseGap = 0;
	result.rpm = 0;

	// Compute width of the last tooth
	efitime_t period = nowNt - _lastEdge;
	_lastEdge = nowNt;

	// first pass idiot check for long teeth
	if(period > US2NT(MS2US(100)))
	{
		return result;
	}

	ToothType tooth = CheckToothType(period);

	switch (_state)
	{
	case MTSyncState::Idle:
		// start counting edges
		_state = MTSyncState::Partial;
		_normalToothCounter = 0;

		UpdateExpectedToothLength(period);

		break;
	case MTSyncState::Partial:
		if (tooth == ToothType::Normal)
		{
			// Count teeth
			_normalToothCounter++;

			// update expected next tooth
			UpdateExpectedToothLength(period);
		}
		else if (tooth == ToothType::Long)
		{
			// If we've only seen a few edges go by, ignore the long tooth
			if (_normalToothCounter > 5)
			{
				// Otherwise, we're synced!
				// We saw >5 short teeth go by, and then a long tooth.
				// The only thing possible is that we're synced.
				_state = MTSyncState::Synced;
				_normalToothCounter = 0;
			}
		}
		else
		{
			// We got a weird long or short tooth, reset everything and try again
			this->Reset();

			UpdateExpectedToothLength(period);
		}

		break;
	case MTSyncState::Synced:
		ToothType expectedToothType;

		// If we've counted enough normal teeth, next should be a long tooth
		if (_normalToothCounter == _normalTeeth)
		{
			expectedToothType = ToothType::Long;
		}
		else
		{
			expectedToothType = ToothType::Normal;
		}

		// If we got something we didn't expect, panic!
		if (expectedToothType != tooth)
		{
			this->Reset();
		}

		// Reset counter on a long tooth
		if (tooth == ToothType::Long)
		{
			_normalToothCounter = 0;
		}
		else
		{
			_normalToothCounter++;
			UpdateExpectedToothLength(period);
		}

		break;
	}

	// if we're synced, solve for the current angle and next angle
	if (_state == MTSyncState::Synced)
	{
		result.phase = _normalToothCounter * _normalToothAngle;
		result.nextPhaseGap = (_normalToothCounter == _normalTeeth) ? _longToothAngle : _normalToothAngle;
	}

	result.synced = (_state == MTSyncState::Synced);

	return result;
}

void MissingToothTriggerDecoder::UpdateExpectedToothLength(efitime_t lastPeriod)
{
	efitime_t oneFourth = lastPeriod / 4;

	// Set the window to expected +- 25%
	_expectedMinNormal = lastPeriod - oneFourth;
	_expectedMaxNormal = lastPeriod + oneFourth;

	// Calculate the "exact" theoretical length a long tooth
	// should be...
	efitime_t exactLongTooth = lastPeriod * (_missing + 1);
	// ...and 25% of that
	oneFourth = exactLongTooth / 4;

	// Set the window to expected +- 25%
	_expectedMinLong = exactLongTooth - oneFourth;
	_expectedMaxLong = exactLongTooth + oneFourth;
}

MissingToothTriggerDecoder::ToothType MissingToothTriggerDecoder::CheckToothType(efitime_t period)
{
	// Check first for normal tooth
	if (period >= _expectedMinNormal && period <= _expectedMaxNormal)
	{
		return ToothType::Normal;
	}

	// Then check if it's a long tooth
	if (period >= _expectedMinLong && period <= _expectedMaxLong)
	{
		return ToothType::Long;
	}

	// if it wasn't either of those, something went wrong
	return ToothType::Invalid;
}

void MissingToothTriggerDecoder::Reset()
{
	_state = MTSyncState::Idle;
	_normalToothCounter = 0;

	_lastEdge = 0;

	_expectedMinNormal = 0;
	_expectedMaxNormal = 0;
	_expectedMinLong = 0;
	_expectedMaxLong = 0;
}
