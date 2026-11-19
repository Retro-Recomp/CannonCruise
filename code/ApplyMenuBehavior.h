#pragma once

// ============================================================================
// CannonCruise - Menu Behavior Apply (ApplyMenuBehavior.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyMenuBehavior.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyMenuBehavior
 * @brief Applies menu behavior attributes and configures menu interactions.
 *
 * Handles the Apply logic for menu behaviors, reading menu settings,
 * interaction flags, and navigation parameters from the attribute packet.
 *
 * **Binary references:**
 * - CreateMenuBehavior: menu system initialization
 * - ApplyMenuBehavior: menu system apply routine
 */
class ApplyMenuBehavior : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyMenuBehavior);
    RWS_DECLARE_CLASS(ApplyMenuBehavior);

    /**
     * @brief Constructor — initializes menu behavior apply handler.
     * @param packet Attribute packet with menu settings.
     */
    ApplyMenuBehavior(const CAttributePacket& packet);

    /**
     * @brief Applies menu behavior events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure menu behavior.
     * @param packet Packet containing menu settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bPaused;           ///< Whether menu is paused
    RwInt32 m_nSelectedIndex;   ///< Currently selected menu index
    RwString m_sMenuName;       ///< Menu name identifier
};

} // namespace RWS