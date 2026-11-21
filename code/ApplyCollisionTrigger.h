#pragma once

// ============================================================================
// CannonCruise - Collision Trigger Apply (ApplyCollisionTrigger.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyCollisionTrigger.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyCollisionTrigger
 * @brief Applies collision trigger attributes and configures collision events.
 *
 * Handles the Apply logic for collision triggers, reading collision settings,
 * filter masks, and event flags from the attribute packet.
 *
 * **Binary references:**
 * - CreateCollisionTrigger: collision system initialization
 * - ApplyCollisionTrigger: collision system apply routine
 */
class ApplyCollisionTrigger : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyCollisionTrigger);
    RWS_DECLARE_CLASS(ApplyCollisionTrigger);

    /**
     * @brief Constructor — initializes collision trigger apply handler.
     * @param packet Attribute packet with collision settings.
     */
    ApplyCollisionTrigger(const CAttributePacket& packet);

    virtual ~ApplyCollisionTrigger();

    /**
     * @brief Applies collision trigger events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure collision trigger.
     * @param packet Packet containing collision settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bOneShot;      ///< Whether trigger is one-shot
    RwBool m_bActive;       ///< Whether trigger is active
    RwV3d m_vTriggerPos;    ///< Trigger world position
    RwReal m_fTriggerRadius; ///< Trigger radius
};

} // namespace RWS