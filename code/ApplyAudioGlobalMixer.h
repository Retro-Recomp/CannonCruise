#pragma once

// ============================================================================
// CannonCruise - Audio Global Mixer Apply (ApplyAudioGlobalMixer.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioGlobalMixer.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0044d590 - audio global mixer apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyAudioGlobalMixer
 * @brief Applies audio global mixer attributes and configures global audio mixing.
 *
 * Handles the Apply logic for the audio global mixer, reading global audio
 * mixing settings such as master volume, reverb Send level, DSP level, and
 * speaker configuration from the attribute packet.
 *
 * **Binary references:**
 * - CreateAudioGlobalMixer: `FUN_0044d590`
 * - ApplyAudioGlobalMixer: `FUN_0044e500`
 */
class ApplyAudioGlobalMixer : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyAudioGlobalMixer);
    RWS_DECLARE_CLASS(ApplyAudioGlobalMixer);

    /**
     * @brief Constructor — initializes audio global mixer apply handler.
     * @param packet Attribute packet with audio mixer settings.
     */
    ApplyAudioGlobalMixer(const CAttributePacket& packet);

    /**
     * @brief Applies audio global mixer events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure audio global mixer.
     * @param packet Packet containing audio mixer settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwReal m_fMasterVolume;       ///< Master volume (0.0 to 1.0)
    RwReal m_fReverbSend;         ///< Reverb send level (0.0 to 1.0)
    RwReal m_fDSPLevel;           ///< DSP effect level (0.0 to 1.0)
    RwInt32 m_iSpeakerConfig;     ///< Speaker configuration (0=stereo, 1=surround 5.1, etc.)
    RwBool m_bDSPEnabled;         ///< Whether DSP effects are enabled
    RwBool m_bReverbEnabled;      ///< Whether reverb effects are enabled
};

} // namespace RWS