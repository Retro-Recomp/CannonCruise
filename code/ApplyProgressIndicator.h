#pragma once

// ============================================================================
// CannonCruise - Progress Indicator Apply (ApplyProgressIndicator.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyProgressIndicator.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_00443C40 - progress indicator apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyProgressIndicator
 * @brief Applies progress indicator attributes and controls progress display.
 *
 * Handles the Apply logic for progress indicators, reading progress
 * percentage, message text, and display mode from the attribute packet.
 * Used for loading screens, level initialization progress, and
 * in-game status displays.
 *
 * **Binary references:**
 * - CreateProgressIndicator: `FUN_00443C40`
 * - ApplyProgressIndicator: `FUN_00443D00`
 */
class ApplyProgressIndicator : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyProgressIndicator);
    RWS_DECLARE_CLASS(ApplyProgressIndicator);

    /**
     * @brief Constructor — initializes progress indicator apply handler.
     * @param packet Attribute packet with progress settings.
     */
    ApplyProgressIndicator(const CAttributePacket& packet);

    /**
     * @brief Applies progress indicator events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure progress indicator.
     * @param packet Packet containing progress settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwReal m_fProgress;        ///< Progress percentage (0.0 to 1.0)
    RwString m_sStatusText;    ///< Status text display
    RwInt32 m_progressMode;    ///< Display mode (0=percentage, 1=text, 2=dual)
    RwBool m_bVisible;         ///< Whether the progress indicator is visible
};

} // namespace RWS