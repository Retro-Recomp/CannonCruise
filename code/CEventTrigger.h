#pragma once

// ============================================================================
// CannonCruise - Event Trigger (CEventTrigger.h)
// Original path: D:\Projects\CannonCruisePC\code\EventTrigger.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_00473580 - FUN_00473600
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class CEventTrigger
 * @brief Event router behavior — receives input event and fires output event.
 *
 * Allows level designers to connect entities unaware of each other from
 * RenderWare Studio editor. Reads input/output event IDs from the attribute
 * packet and routes events at runtime.
 *
 * **Binary references:**
 * - Ctor: `FUN_00473480`
 * - HandleEvents: `FUN_00473580`
 * - Attribute packet: input event ID, output event ID
 */
class CEventTrigger : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(CEventTrigger);
    RWS_DECLARE_CLASS(CEventTrigger);

    /**
     * @brief Constructor — initializes event trigger from attribute packet.
     * @param packet Attribute packet containing event ID settings.
     */
    CEventTrigger(const CAttributePacket& packet);

    /**
     * @brief Destructor — clears event registrations.
     */
    virtual ~CEventTrigger();

    /**
     * @brief Route input event to output event.
     * @param msg Incoming event message.
     * @return TRUE if event was handled.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet values to the behavior.
     * @param packet Attribute packet with event ID settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

    /**
     * @brief Enable/disable event routing.
     * @param enabled TRUE to enable event routing.
     */
    void SetEnabled(RwBool enabled) { m_bEnabled = enabled; }

    /**
     * @brief Check if event routing is enabled.
     * @return TRUE if enabled.
     */
    RwBool IsEnabled() const { return m_bEnabled; }

private:
    CEventId m_eventInput;    ///< Input event ID to listen for
    CEventId m_eventOutput;   ///< Output event ID to route to
    RwBool   m_bEnabled;      ///< Event routing enabled flag
};

} // namespace RWS