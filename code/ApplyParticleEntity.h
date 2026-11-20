#pragma once

// ============================================================================
// CannonCruise - Particle Entity Apply (ApplyParticleEntity.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyParticleEntity.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_004be0b0 - particle entity event handling
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyParticleEntity
 * @brief Applies particle entity attributes and manages particle effects.
 *
 * Handles the Apply logic for particle entities such as explosions,
 * debris, and environmental effects. Reads particle type, duration,
 * and visibility flags from the attribute packet.
 *
 * **Binary references:**
 * - HandleEvents: `FUN_004be0b0`
 * - Particle spawning: `FUN_004be1a0`
 */
class ApplyParticleEntity : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyParticleEntity);
    RWS_DECLARE_CLASS(ApplyParticleEntity);

    /**
     * @brief Constructor — initializes particle entity apply handler.
     * @param packet Attribute packet with particle settings.
     */
    ApplyParticleEntity(const CAttributePacket& packet);

    virtual ~ApplyParticleEntity();

    /**
     * @brief Applies particle entity events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure particle entity.
     * @param packet Packet containing particle settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwInt32 m_particleType;    ///< Type of particle effect (explosion, debris, etc.)
    RwReal m_fDuration;        ///< Effect duration in seconds
    RwBool m_bActive;          ///< Whether the effect is active
    RwV3d m_vPosition;         ///< Effect world position
};

} // namespace RWS