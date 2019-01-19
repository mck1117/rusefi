#pragma once

#include "error_handling.h"
#include "rusefi_enums.h"
#include "rusefi_types.h"

#include "TriggerResult.h"

#include <optional>

class TriggerDecoder
{
public:
    // This lets us confirm that the top-level decoder is the correct
    // flavor for the engine we're running.  If we're running a two-stroke,
    // we need a crank-only decoder, and if we're running a four-stroke,
    // we need a cam decoder.
    //
    // Crank decoder = emits phase in range [0, 360)
    // Cam decoder   = emits phase in range [0, 720)
    enum class TriggerDecoderType
    {
        Crank,
        Cam,
    };

    TriggerDecoder() = default;

    // Copying a trigger is probably very bad! Disallow!
    TriggerDecoder(const TriggerDecoder&) = delete;
    TriggerDecoder& operator=(const TriggerDecoder&) = delete;
    
    ~TriggerDecoder() = default;

    std::optional<TriggerResult> HandleTriggerEdge(trigger_event_e const signal, const efitime_t nowNt);

    virtual TriggerDecoderType GetTriggerType() = 0;

    virtual void Reset() = 0;
protected:
    /**
     * Determines if the trigger should attempt to decode this type of edge.
     * 
     * \param[in] signal    The candidate edge to be decoded.
     * \return              True if this edge is 'interesting' to the decoder, otherwise false.
     */
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) = 0;

    /**
     * Decodes the passed trigger edge, at the specified time.
     * Only called if ShouldDecodeTriggerEdge returned true.
     * 
     * \param[in] signal    The type of edge to be decoded.
     * \param[in] nowNt     The current time.
     * \return              The trigger result containing sync status and current engine phase information.
     */
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) = 0;
};

class CrankshaftTriggerDecoder : public TriggerDecoder
{
public:
    virtual TriggerDecoder::TriggerDecoderType GetTriggerType() override final { return TriggerDecoderType::Crank; }
};

class CamshaftTriggerDecoder : public TriggerDecoder
{
public:
    virtual TriggerDecoder::TriggerDecoderType GetTriggerType() override final { return TriggerDecoderType::Cam; }
};

#include <utility>

template <class TCrank>
class CamshaftWheelAdder : public CamshaftTriggerDecoder
{
protected:
    TCrank _crankDecoder;

public:
    CamshaftWheelAdder() = delete;

    template <class... _Types>
    CamshaftWheelAdder(_Types&&... crankArgs)
        : _crankDecoder(std::forward<_Types>(crankArgs)...)
    {
        // Only allow adding a camshaft if we're consuming a crank decoder
        if(TriggerDecoderType::Crank != _crankDecoder.GetTriggerType())
        {
            firmwareError(CUSTOM_ERR_ASSERT, "Camshaft wheel added initialized with something that isn't a crank decoder");
        }
    }
};

template <class TCrank>
class CamlessDecoder final : public CamshaftWheelAdder<TCrank>
{
private:
    bool _flip;
    angle_t _lastPhase;
public:
    template <class... _Types>
    CamlessDecoder(_Types&&... crankArgs) 
        : CamshaftWheelAdder<TCrank>(std::forward<_Types>(crankArgs)...),
          _flip(false),
          _lastPhase(-1) { }

    virtual void Reset() override;

protected:
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) override;
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) override;
};

template <class TCrank>
class SingleToothCamDecoder final : public CamshaftWheelAdder<TCrank>
{
private:
    bool _flip;
    angle_t _lastPhase;
public:
    template <class... _Types>
    SingleToothCamDecoder(_Types&&... crankArgs) 
        : CamshaftWheelAdder<TCrank>(std::forward<_Types>(crankArgs)...),
          _flip(false),
          _lastPhase(-1) { }

    virtual void Reset() override;

protected:
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) override;
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) override;
};

class GmLs24xCrankTriggerDecoder : public CrankshaftTriggerDecoder
{
public:
    GmLs24xCrankTriggerDecoder();
    virtual void Reset() override { }
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) override;
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) override;
};

class NissanZ32CasDecoder : public CamshaftTriggerDecoder
{
public:
    NissanZ32CasDecoder();
    virtual void Reset() override { }
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) override;
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) override;
};
