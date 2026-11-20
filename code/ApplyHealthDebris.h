#pragma once

// ============================================================================
// CannonCruise - Health Debris Apply (ApplyHealthDebris.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyHealthDebris.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyHealthDebris
 * @brief Applies health debris attributes and configures debris behavior.
 *
 * Handles the Apply logic for health debris, reading debris settings,
 * lifetime, and spawn parameters from the attribute packet.
 *
 * **Binary references:**
 * - CreateHealthDebris: debris system initialization
 * - ApplyHealthDebris: debris system apply routine
 */
class ApplyHealthDebris : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyHealthDebris);
    RWS_DECLARE_CLASS(ApplyHealthDebris);

    /**
     * @brief Constructor — initializes health debris apply handler.
     * @param packet Attribute packet with debris settings.
     */
    ApplyHealthDebris(const CAttributePacket& packet);

    virtual ~ApplyHealthDebris();

    /**
     * @brief Applies health debris events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure health debris.
     * @param packet Packet containing debris settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwReal m_fLifetime;       ///< Debris lifetime in seconds
    RwV3d m_vVelocity;        ///< Initial debris velocity
    RwReal m_fMass;           ///< Debris mass
    RwBool m_bActive;         ///< Whether debris is active
};

} // namespace RWS