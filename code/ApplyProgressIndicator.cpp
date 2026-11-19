// ============================================================================
// CannonCruise - Progress Indicator Apply (ApplyProgressIndicator.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyProgressIndicator.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_00443C40 (CreateProgressIndicator),
//            FUN_00443D00 (ApplyProgressIndicator)
// ============================================================================

#include "ApplyProgressIndicator.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyProgressIndicator::ApplyProgressIndicator(const CAttributePacket& packet)
    : m_fProgress(0.0f),
      m_progressMode(0),
      m_bVisible(TRUE)
{
    // Binary constructor (FUN_00443C40) does:
    // 1. Zero-out the object
    // 2. Read progress percentage from packet attributes
    // 3. Read progress mode and visibility from packet
    // 5. Initialize status text to empty

    memset(this, 0, sizeof(*this));
    m_bVisible = TRUE;

    // Read progress indicator attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyProgressIndicator));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Progress percentage (0.0 to 1.0)
            m_fProgress = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 1:
            // Progress display mode (0=percentage, 1=text only, 2=dual)
            m_progressMode = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 2:
            // Status text display
            {
                const char* text = reinterpret_cast<const char*>(
                    it->GetCommandData());
                // Safe copy with null terminator
                if (text)
                {
                    strncpy(m_sStatusText, text, 127);
                    m_sStatusText[127] = '\0';
                }
                else
                {
                    m_sStatusText[0] = '\0';
                }
            }
            break;
        case 3:
            // Visibility flag
            m_bVisible = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyProgressIndicator::~ApplyProgressIndicator()
{
}

void ApplyProgressIndicator::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_00443C40) handles progress indicator events:
    // - Progress update messages
    // - Mode changes
    // - Visibility toggles
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyProgressIndicator::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_00443D00) applies progress indicator attributes:
    // - Reads progress percentage, mode, and visibility from packet
    // - Updates the progress display with the new settings
    // - Shows or hides the progress indicator based on visibility flag

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the progress display.

    // If visible, update the progress display
    if (m_bVisible)
    {
        // TODO: Update the progress display with new percentage and mode
    }

    return TRUE;
}

} // namespace RWS