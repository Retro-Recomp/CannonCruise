// ============================================================================
// CannonCruise - Particle Entity Apply (ApplyParticleEntity.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyParticleEntity.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_004be0b0 (HandleEvents),
//            FUN_004be1a0 (particle spawn),
//            FUN_0040B300 (strcpy/field store)
// ============================================================================

#include "ApplyParticleEntity.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include "RwsUtils.h"

namespace RWS
{

ApplyParticleEntity::ApplyParticleEntity(const CAttributePacket& packet)
    : m_particleType(0),
      m_fDuration(5.0f),
      m_bActive(FALSE)
{
    // Binary constructor (FUN_004be0b0) does:
    // 1. Zero-out the object
    // 2. Read particle type from packet attributes
    // 3. Read duration from packet attributes
    // 4. Initialize particle position to origin

    memset(this, 0, sizeof(*this));
    m_bActive = TRUE;

    // Read particle type from attribute packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyParticleEntity));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Particle type (0=explosion, 1=debris, 2=smoke, etc.)
            m_particleType = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 1:
            // Effect duration in seconds
            m_fDuration = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 2:
            // X position offset
            m_vPosition.x = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Y position offset
            m_vPosition.y = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 4:
            // Z position offset
            m_vPosition.z = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyParticleEntity::~ApplyParticleEntity()
{
}

void ApplyParticleEntity::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_004be0b0) handles particle entity events:
    // - Particle spawn/despawn messages
    // - Duration update messages
    // - Visibility toggles
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyParticleEntity::Apply(const CAttributePacket& packet)
{
    // Binary (sub_4be1a0 / loc_4be200) applies particle entity attributes:
    // - Reads particle type and duration from packet
    // - Initializes particle system with configured settings
    // - Enables the effect if duration > 0

    // Read attributes from packet (already done in constructor)
    // In a full implementation, this would start/stop the particle system.

    // If duration is positive, activate the effect
    if (m_fDuration > 0.0f)
    {
        m_bActive = TRUE;
    }
    else
    {
        m_bActive = FALSE;
    }

    return TRUE;
}

} // namespace RWS