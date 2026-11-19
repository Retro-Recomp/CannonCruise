// ============================================================================
// CannonCruise - Fernando Actor Apply (ApplyFernandoActor.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyFernandoActor.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "ApplyFernandoActor.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyFernandoActor::ApplyFernandoActor(const CAttributePacket& packet)
    : m_bActive(TRUE)
{
    memset(this, 0, sizeof(*this));

    // Read Fernando actor attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyFernandoActor));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Active flag
            m_bActive = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 1:
            // World position
            m_vWorldPos.x = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            m_vWorldPos.y = *reinterpret_cast<const RwReal*>(
                it->GetCommandData() + sizeof(RwReal));
            m_vWorldPos.z = *reinterpret_cast<const RwReal*>(
                it->GetCommandData() + 2 * sizeof(RwReal));
            break;
        case 2:
            // Skin name
            {
                const char* name = reinterpret_cast<const char*>(
                    it->GetCommandData());
                if (name)
                {
                    strncpy(m_sSkinName, name, 63);
                    m_sSkinName[63] = '\0';
                }
                else
                {
                    m_sSkinName[0] = '\0';
                }
            }
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyFernandoActor::~ApplyFernandoActor()
{
}

void ApplyFernandoActor::HandleEvents(const RWS::CMsg& msg)
{
    // Actor system handles actor events:
    // - State changes
    // - Position updates
    // - Skin changes
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyFernandoActor::Apply(const CAttributePacket& packet)
{
    // Actor system applies Fernando actor attributes:
    // - Reads active flag, position, and skin name from packet
    // - Configures the actor system with the new settings
    // - Updates the actor visuals if skin changed

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the actor system.

    // Configure actor settings
    // TODO: Update actor system with new settings:
    // - Active: m_bActive
    // - Position: m_vWorldPos
    // - Skin name: m_sSkinName

    return TRUE;
}

} // namespace RWS