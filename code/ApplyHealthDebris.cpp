// ============================================================================
// CannonCruise - Health Debris Apply (ApplyHealthDebris.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyHealthDebris.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "ApplyHealthDebris.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyHealthDebris::ApplyHealthDebris(const CAttributePacket& packet)
    : m_fLifetime(5.0f),
      m_fMass(1.0f),
      m_bActive(TRUE)
{

    // Read health debris attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyHealthDebris));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Lifetime in seconds
            m_fLifetime = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 1:
            // Initial velocity
            m_vVelocity.x = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            m_vVelocity.y = *reinterpret_cast<const RwReal*>(
                static_cast<const RwUInt8*>(it->GetCommandData()) + sizeof(RwReal));
            m_vVelocity.z = *reinterpret_cast<const RwReal*>(
                static_cast<const RwUInt8*>(it->GetCommandData()) + 2 * sizeof(RwReal));
            break;
        case 2:
            // Debris mass
            m_fMass = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Active flag
            m_bActive = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyHealthDebris::~ApplyHealthDebris()
{
}

void ApplyHealthDebris::HandleEvents(const RWS::CMsg& msg)
{
    // Debris system handles debris events:
    // - Lifetime ticks
    // - Removal events
    // - Collision events
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyHealthDebris::Apply(const CAttributePacket& packet)
{
    // Binary debris system applies health debris attributes:
    // - Reads lifetime, velocity, mass, and active flag from packet
    // - Configures the debris system with the new settings
    // - Updates the debris simulation if active

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the debris system.

    // Configure debris settings
    // TODO: Update debris system with new settings:
    // - Lifetime: m_fLifetime
    // - Velocity: m_vVelocity
    // - Mass: m_fMass
    // - Active: m_bActive

    return TRUE;
}

} // namespace RWS