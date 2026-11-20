#pragma once

// ============================================================================
// CannonCruise - Tower Catapult Apply (ApplyTowerCatapult.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyTowerCatapult.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_004bc3f0 — CTowerCatapult constructor
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyTowerCatapult
 * @brief Applies tower catapult attributes and configures tower durability.
 *
 * Handles the Apply logic for tower catapults, reading durability settings,
 * and configuration from the attribute packet.
 *
 * **Binary references:**
 * - CreateTowerCatapult: `FUN_004bc3f0`
 * - ApplyTowerCatapult: `FUN_004bc3f0`
 */
class ApplyTowerCatapult : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyTowerCatapult);
    RWS_DECLARE_CLASS(ApplyTowerCatapult);

    /**
     * @brief Constructor — initializes tower catapult apply handler.
     * @param packet Attribute packet with tower settings.
     */
    ApplyTowerCatapult(const CAttributePacket& packet);

    virtual ~ApplyTowerCatapult();

    /**
     * @brief Applies tower catapult events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure tower catapult.
     * @param packet Packet containing tower settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwReal m_fHealth;       ///< Tower health pool
    RwBool m_bDestroyed;    ///< Whether the tower is destroyed
    RwV3d m_vWorldPos;      ///< Tower world position
};

} // namespace RWS