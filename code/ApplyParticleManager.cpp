// ============================================================================
// CannonCruise - Particle Manager Apply (ApplyParticleManager.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyParticleManager.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_00443E80 (ApplyParticleManager),
//            FUN_00453E80 (CreateParticleManager)
// ============================================================================

#include "ApplyParticleManager.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyParticleManager::ApplyParticleManager(const CAttributePacket& packet)
    : m_systemType(0),
      m_bEnabled(TRUE),
      m_maxParticles(100),
      m_fEmissionRate(10.0f)
{
    // Binary constructor (FUN_00443E80) does:
    // 1. Zero-out the object
    // 2. Read particle system type from packet attributes
    // 3. Read enable flag from packet
    // 4. Read max particles and emission rate

    memset(this, 0, sizeof(*this));
    m_bEnabled = TRUE;

    // Read particle system attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyParticleManager));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Particle system type (0=fire, 1=smoke, 2=magic, etc.)
            m_systemType = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 1:
            // Enable/disable flag
            m_bEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 2:
            // Maximum number of active particles
            m_maxParticles = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 3:
            // Emission rate (particles per second)
            m_fEmissionRate = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyParticleManager::~ApplyParticleManager()
{
}

void ApplyParticleManager::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_00443E80) handles particle manager events:
    // - System enable/disable messages
    // - Parameter update messages
    // - System spawn/despawn messages
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyParticleManager::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_00443E80) applies particle manager attributes:
    // - Reads system type, enable flag, max particles, emission rate
    // - Configures the global particle system manager
    // - Enables or disables particle effects globally

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the global particle system.

    // If enabled, configure the particle system manager
    if (m_bEnabled)
    {
        // TODO: Update global particle system manager with new settings
    }

    return TRUE;
}

} // namespace RWS