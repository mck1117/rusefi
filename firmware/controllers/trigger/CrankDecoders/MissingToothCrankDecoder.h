
#include "TriggerDecoder.h"

class MissingToothTriggerDecoder : public CrankshaftTriggerDecoder
{
private:
    uint8_t _teeth, _normalTeeth, _missing;

    // How many short teeth have we seen
    // since the last long tooth?
    uint8_t _normalToothCounter;

    efitime_t _lastEdge;

    float _normalToothAngle, _longToothAngle;


    enum class MTSyncState
    {
        Idle,
        Partial,
        Synced,
    };

    MTSyncState _state;

    efitime_t _expectedMinNormal, _expectedMaxNormal,
              _expectedMinLong, _expectedMaxLong;

    enum class ToothType
    {
        Invalid,
        Normal,
        Long,
    };
    
public:
    MissingToothTriggerDecoder() = delete;
    MissingToothTriggerDecoder(uint8_t teeth, uint8_t missing);
    virtual void Reset() override;
protected:
    virtual bool ShouldDecodeTriggerEdge(trigger_event_e const signal) override;
    virtual TriggerResult DecodeTriggerEdge(trigger_event_e const signal, const efitime_t nowNt) override;

    virtual void UpdateExpectedToothLength(efitime_t lastPeriod);
    
    MissingToothTriggerDecoder::ToothType CheckToothType(efitime_t period);
};
