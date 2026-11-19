// ============================================================================
// CannonCruise - Menu Behavior Apply (ApplyMenuBehavior.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyMenuBehavior.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "ApplyMenuBehavior.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyMenuBehavior::ApplyMenuBehavior(const CAttributePacket& packet)
    : m_bPaused(FALSE),
      m_nSelectedIndex(0)
{
    memset(this, 0, sizeof(*this));

    // Read menu behavior attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyMenuBehavior));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Paused flag
            m_bPaused = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 1:
            // Selected index
            m_nSelectedIndex = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 2:
            // Menu name
            {
                const char* name = reinterpret_cast<const char*>(
                    it->GetCommandData());
                if (name)
                {
                    strncpy(m_sMenuName, name, 63);
                    m_sMenuName[63] = '\0';
                }
                else
                {
                    m_sMenuName[0] = '\0';
                }
            }
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyMenuBehavior::~ApplyMenuBehavior()
{
}

void ApplyMenuBehavior::HandleEvents(const RWS::CMsg& msg)
{
    // Binary menu system handles menu events:
    // - Pause/resume toggles
    // - Navigation changes
    // - Selection confirmations
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyMenuBehavior::Apply(const CAttributePacket& packet)
{
    // Binary menu system applies menu behavior attributes:
    // - Reads paused flag, selected index, and menu name from packet
    // - Configures the menu system with the new settings
    // - Updates the menu display if name changed

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the menu system.

    // Configure menu behavior settings
    // TODO: Update menu system with new settings:
    // - Paused: m_bPaused
    // - Selected index: m_nSelectedIndex
    // - Menu name: m_sMenuName

    return TRUE;
}

} // namespace RWS