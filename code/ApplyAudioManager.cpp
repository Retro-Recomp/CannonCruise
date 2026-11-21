// ============================================================================
// CannonCruise - Audio Manager Apply (ApplyAudioManager.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioManager.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0044c990 (CreateAudioManager),
//            FUN_0044d500 (ApplyAudioManager)
// ============================================================================

#include "ApplyAudioManager.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyAudioManager::ApplyAudioManager(const CAttributePacket& packet)
    : m_fMasterVolume(1.0f),
      m_bReverbEnabled(FALSE),
      m_iSpeakerConfig(0),
      m_bDSPEnabled(FALSE)
{
    // Binary constructor (FUN_0044c990) does:
    // 1. Zero-out the object
    // 2. Read master volume from packet attributes
    // 3. Read reverb enable flag from packet
    // 4. Read speaker configuration from packet
    // 5. Read DSP enable flag from packet


    // Read audio manager attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyAudioManager));

    while (!it.IsFinished())
    {
        switch (it->GetCommandId())
        {
        case 0:
            // Master volume (0.0 to 1.0)
            m_fMasterVolume = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 1:
            // Reverb enable flag
            m_bReverbEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 2:
            // Speaker configuration (0=stereo, 1=surround 5.1, etc.)
            m_iSpeakerConfig = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 3:
            // DSP enable flag
            m_bDSPEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyAudioManager::~ApplyAudioManager()
{
}

void ApplyAudioManager::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_0044d500) handles audio manager events:
    // - Master volume change messages
    // - Reverb enable/disable messages
    // - Speaker configuration change messages
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyAudioManager::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_0044d500) applies audio manager attributes:
    // - Reads master volume, reverb enable, speaker config, and DSP enable
    // - Configures the global audio manager with the new settings
    // - Applies the new audio settings system-wide

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the global audio manager.

    // Apply the new audio manager settings system-wide
    // TODO: Update global audio manager with new settings:
    // - Master volume: m_fMasterVolume
    // - Reverb enabled: m_bReverbEnabled
    // - Speaker config: m_iSpeakerConfig
    // - DSP enabled: m_bDSPEnabled

    return TRUE;
}

} // namespace RWS