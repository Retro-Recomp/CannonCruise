#pragma once

// ============================================================================
// CannonCruise - Fernando Actor Apply (ApplyFernandoActor.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyFernandoActor.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyFernandoActor
 * @brief Applies Fernando actor attributes and configures actor behavior.
 *
 * Handles the Apply logic for Fernando actor, reading actor settings,
 * state flags, and visual parameters from the attribute packet.
 *
 * **Binary references:**
 * - CreateFernandoActor: actor system initialization
 * - ApplyFernandoActor: actor system apply routine
 */
class ApplyFernandoActor : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyFernandoActor);
    RWS_DECLARE_CLASS(ApplyFernandoActor);

    /**
     * @brief Constructor — initializes Fernando actor apply handler.
     * @param packet Attribute packet with actor settings.
     */
    ApplyFernandoActor(const CAttributePacket& packet);

    virtual ~ApplyFernandoActor();

    /**
     * @brief Applies Fernando actor events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure Fernando actor.
     * @param packet Packet containing actor settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bActive;         ///< Whether actor is active
    RwV3d m_vWorldPos;        ///< Actor world position
    char m_sSkinName[256];     ///< Actor skin name
};

} // namespace RWS