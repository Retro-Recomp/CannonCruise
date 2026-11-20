// ============================================================================
// CannonCruise - Audio Global Mixer Apply (ApplyAudioGlobalMixer.cpp)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioGlobalMixer.cpp
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary refs: FUN_0044d590 (CreateAudioGlobalMixer),
//            FUN_0044e500 (ApplyAudioGlobalMixer)
// ============================================================================

#include "ApplyAudioGlobalMixer.h"
#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

ApplyAudioGlobalMixer::ApplyAudioGlobalMixer(const CAttributePacket& packet)
    : m_fMasterVolume(1.0f),
      m_fReverbSend(0.5f),
      m_fDSPLevel(0.5f),
      m_iSpeakerConfig(0),
      m_bDSPEnabled(FALSE),
      m_bReverbEnabled(FALSE)
{
    // Binary constructor (FUN_0044d590) does:
    // 1. Zero-out the object
    // 2. Read master volume from packet attributes
    // 3. Read reverb send level from packet
    // 4. Read DSP level from packet
    // 5. Read speaker configuration from packet
    // 6. Read DSP enable flag from packet
    // 7. Read reverb enable flag from packet


    // Read audio global mixer attributes from packet
    CAttributeCommandIterator it(packet, RWS_CLASS_INDEX(ApplyAudioGlobalMixer));

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
            // Reverb send level (0.0 to 1.0)
            m_fReverbSend = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 2:
            // DSP level (0.0 to 1.0)
            m_fDSPLevel = *reinterpret_cast<const RwReal*>(
                it->GetCommandData());
            break;
        case 3:
            // Speaker configuration (0=stereo, 1=surround 5.1, etc.)
            m_iSpeakerConfig = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData());
            break;
        case 4:
            // DSP enable flag
            m_bDSPEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        case 5:
            // Reverb enable flag
            m_bReverbEnabled = *reinterpret_cast<const RwInt32*>(
                it->GetCommandData()) != 0;
            break;
        default:
            break;
        }
        ++it;
    }
}

ApplyAudioGlobalMixer::~ApplyAudioGlobalMixer()
{
}

void ApplyAudioGlobalMixer::HandleEvents(const RWS::CMsg& msg)
{
    // Binary (FUN_0044e500) handles audio global mixer events:
    // - Master volume change messages
    // - Reverb send level change messages
    // - DSP level change messages
    // - Speaker configuration change messages
    // For now, just log the event type.
    (void)msg;
}

RwBool ApplyAudioGlobalMixer::Apply(const CAttributePacket& packet)
{
    // Binary (FUN_0044e500) applies audio global mixer attributes:
    // - Reads master volume, reverb send, DSP level, speaker config, and enable flags
    // - Configures the global audio mixer with the new settings
    // - Applies the new audio mixing settings system-wide

    // Apply the read attributes (already done in constructor)
    // In a full implementation, this would update the global audio mixer.

    // Apply the new audio global mixer settings system-wide
    // TODO: Update global audio mixer with new settings:
    // - Master volume: m_fMasterVolume
    // - Reverb send level: m_fReverbSend
    // - DSP level: m_fDSPLevel
    // - Speaker config: m_iSpeakerConfig
    // - DSP enabled: m_bDSPEnabled
    // - Reverb enabled: m_bReverbEnabled

    return TRUE;
}

} // namespace RWS