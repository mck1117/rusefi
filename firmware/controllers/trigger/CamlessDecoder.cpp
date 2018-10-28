#include "TriggerDecoder.h"

#include "global.h"

template <class TCrank>
void CamlessDecoder<TCrank>::Reset()
{
    _flip = false;
    this->_crankDecoder.Reset();
}

template <class TCrank>
bool CamlessDecoder<TCrank>::ShouldDecodeTriggerEdge(trigger_event_e const signal)
{
    return this->_crankDecoder->ShouldDecodeTriggerEdge(signal);
}

template <class TCrank>
TriggerResult CamlessDecoder<TCrank>::DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt)
{
    // Decode the crankshaft position first
    auto result = this->_crankDecoder->DecodeTriggerEdge(signal, nowNt);

    // Only bother once we're synced--don't report any phase info until then
    if(result.synced)
    {
        // If the crank went "backward" or stayed in the same spot, we're
        // now on the next revolution of the crank, so we need to either continue
        // from 359.9 up to 360, or from 719.9 back around to 0
        // We could stay in the same spot with a single-tooth-per-rev crank wheel
        if(result.phase <= _lastPhase)
        {
            // Flip to the other half of the firing order
            _flip = !_flip;
        }

        _lastPhase = result.phase;

        // if we're on the back half of the firing order, add 360.
        if(_flip)
        {
            result.phase += 360.0f;
        }
    }

    return result;
}
