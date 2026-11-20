#pragma once

// ============================================================================
// CannonCruise - Particle Manager Apply (ApplyParticleManager.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyParticleManager.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_00443E80 - particle manager apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyParticleManager
 * @brief Applies particle manager attributes and controls particle systems.
 *
 * Handles the Apply logic for the particle manager, which controls
 * global particle systems such as fire, smoke, and magic effects.
 * Reads system type, enable flags, and parameter settings from
 * the attribute packet.
 *
 * **Binary references:**
 * - CreateParticleManager: `FUN_00453E80`
 * - ApplyParticleManager: `FUN_00443E80`
 */
class ApplyParticleManager : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyParticleManager);
    RWS_DECLARE_CLASS(ApplyParticleManager);

    /**
     * @brief Constructor — initializes particle manager apply handler.
     * @param packet Attribute packet with particle system settings.
     */
    ApplyParticleManager(const CAttributePacket& packet);

    virtual ~ApplyParticleManager();

    /**
     * @brief Applies particle manager events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure particle manager.
     * @param packet Packet containing particle system settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwInt32 m_systemType;      ///< Particle system type (fire, smoke, magic)
    RwBool m_bEnabled;         ///< Whether particle systems are enabled
    RwInt32 m_maxParticles;    ///< Maximum number of active particles
    RwReal m_fEmissionRate;    ///< Particles emitted per second
};

} // namespace RWS