// ============================================================================
// CannonCruise - FMV Behavior Apply (ApplyFMVBehavior.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyFMVBehavior.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0045F580 (CreateFMVBehavior),
//            FUN_0045F950 (ApplyFMVBehavior)
// ============================================================================

#include "ApplyFMVBehavior.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include <cstring>

namespace RWS
{

ApplyFMVBehavior::ApplyFMVBehavior(const CAttributePacket& packet)
    : m_bLoop(FALSE),
      m_fStartTime(0.0f),
      m_fEndTime(0.0f),
      m_bTransition(FALSE)
{
    m_sVideoFile[0] = '\0';
    // Binary constructor (FUN_0045F580) does:
    // 1. Zero-out the object
    // 2. Read video file path from packet attributes
    // 3. Read start time, end time, and transition flag from packet
    // 5. Store all settings for FMV playback configuration


    // Read FMV behavior attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyFMVBehavior));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Video file path string
            {
                const char* filePath = reinterpret_cast<const char*>(
                    it->GetCommandData());
                // Safe copy with null terminator
                if (filePath)
                {
                    strncpy(m_sVideoFile, filePath, 255);
                    m_sVideoFile[255] = '\0';
                }
                else
                {
                    m_sVideoFile[0] = '\0';
                }
            }
            break;
        case 1:
            // Start time in video (seconds)
            m_fStartTime = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 2:
            // End time in video (seconds)
            m_fEndTime = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Transition enable flag
            m_bTransition = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyFMVBehavior::~ApplyFMVBehavior()
{
}

void ApplyFMVBehavior::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_0045F950) handles FMV behavior events:
    // - Video play/stop messages
    // - Start/end time changes
    // - Transition effect toggles
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyFMVBehavior::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_0045F950) applies FMV behavior attributes:
    // - Reads video file path, start/end times, and transition flag
    // - Configures the FMV playback system with the new settings
    // - Prepares the FMV for playback with the specified parameters

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would configure the FMV playback system.

    // If a video file was specified, prepare it for playback
    if (m_sVideoFile[0] != '\0')
    {
        // TODO: Load and configure the FMV video for playback
        // with the specified start/end times and transition effects.
    }

    return TRUE;
}

} // namespace RWS