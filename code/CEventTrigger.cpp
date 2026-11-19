// ============================================================================
// CannonCruise - Event Trigger (CEventTrigger.cpp)
// Original path: D:\Projects\CannonCruisePC\code\EventTrigger.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_00473480 (ctor), FUN_00473580 (HandleEvents),
//            FUN_004053F0 (memset), FUN_0040B300 (strcpy/field store)
// ============================================================================

#include "CEventTrigger.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

CEventTrigger::CEventTrigger(const CAttributePacket& packet)
    : m_bEnabled(TRUE)
{
    // Binary constructor (FUN_00473480) does:
    // 1. Zero-out the object
    // 2. Read input event ID from packet attributes
    // 3. Read output event ID from packet attributes
    // 4. Store both event IDs for runtime routing

    memset(this, 0, sizeof(*this));
    m_bEnabled = TRUE;

    // Read input event ID from attribute packet
    // Based on binary: sub_40B300 called with attribute to store event ID
    // The packet contains: input event ID at one offset, output at another

    // For now, zero the event IDs - they'll be set via Apply/attributes
    m_eventInput = CEventId{ 0 };
    m_eventOutput = CEventId{ 0 };
}

CEventTrigger::~CEventTrigger()
{
}

void CEventTrigger::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_00473580) routes input event to output event:
    // if (msg.Id == m_eventInput) { fire m_eventOutput; }

    // For now, just forward if IDs are set
    // TODO: full event routing integration when event system is wired
    (void)msg;
}

RwBool CEventTrigger::Apply(const CAttributePacket& packet)
{
    // Binary (sub_473580 / loc_473600) applies attribute packet:
    // - Reads input event ID from packet and stores in m_eventInput
    // - Reads output event ID from packet and stores in m_eventOutput
    // - Enables/disables routing based on packet flags

    // Read input event ID from packet
    // The packet has commands for event IDs; read them similar to CTrigger::HandleAttributes
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(CEventTrigger));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Input event ID (first attribute)
            m_eventInput = *reinterpret_cast<const CEventId*>(
                it->GetCommandData());
            break;
        case 1:
            // Output event ID (second attribute)
            m_eventOutput = *reinterpret_cast<const CEventId*>(
                it->GetCommandData());
            break;
        case 2:
            // Enable/disable flag
            m_bEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }

    return TRUE;
}

} // namespace RWS