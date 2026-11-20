// ============================================================================
// CannonCruise - Audio Object Apply (ApplyAudioObject.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioObject.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0044bef0 (CreateAudioObject),
//            FUN_0044c900 (ApplyAudioObject)
// ============================================================================

#include "ApplyAudioObject.h"
#include "framework/core/eventhandler/ceventhandler.h"
#include <cstring>

namespace RWS
{

ApplyAudioObject::ApplyAudioObject(const CAttributePacket& packet)
    : m_bLoop(FALSE),
      m_fVolume(1.0f),
      m_fPitch(1.0f)
{
    m_sSoundFile[0] = '\0';
    // Binary constructor (FUN_0044bef0) does:
    // 1. Zero-out the object
    // 2. Read sound file path from packet attributes
    // 3. Read volume, loop flag, and pitch from packet
    // 5. Store all settings for later playback


    // Read audio object attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyAudioObject));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Sound file path string
            {
                const char* filePath = reinterpret_cast<const char*>(
                    it->GetCommandData());
                // Safe copy with null terminator
                if (filePath)
                {
                    strncpy(m_sSoundFile, filePath, 255);
                    m_sSoundFile[255] = '\0';
                }
                else
                {
                    m_sSoundFile[0] = '\0';
                }
            }
            break;
        case 1:
            // Volume (0.0 to 1.0)
            m_fVolume = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 2:
            // Pitch shift (0.5 to 2.0 typically)
            m_fPitch = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Loop flag
            m_bLoop = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyAudioObject::~ApplyAudioObject()
{
}

void ApplyAudioObject::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_0044bef0) handles audio object events:
    // - Sound play/stop messages
    // - Volume/pitch change messages
    // - Loop enable/disable messages
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyAudioObject::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_0044c900) applies audio object attributes:
    // - Reads sound file path, volume, pitch, and loop flag from packet
    // - Configures the audio system with the new settings
    // - Prepares the sound for playback with the specified parameters

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would configure the audio system.

    // If a sound file was specified, prepare it for playback
    if (m_sSoundFile[0] != '\0')
    {
        // TODO: Load and prepare the sound file for playback
        // with the specified volume, pitch, and loop settings.
    }

    return TRUE;
}

} // namespace RWS