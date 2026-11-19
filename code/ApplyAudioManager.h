#pragma once

// ============================================================================
// CannonCruise - Audio Manager Apply (ApplyAudioManager.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioManager.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0044c990 - audio manager apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyAudioManager
 * @brief Applies audio manager attributes and configures global audio settings.
 *
 * Handles the Apply logic for the audio manager, reading global audio
 * settings such as master volume, reverb enabled/disabled, and speaker
 * configuration from the attribute packet.
 *
 * **Binary references:**
 * - CreateAudioManager: `FUN_0044c990`
 * - ApplyAudioManager: `FUN_0044d500`
 */
class ApplyAudioManager : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyAudioManager);
    RWS_DECLARE_CLASS(ApplyAudioManager);

    /**
     * @brief Constructor — initializes audio manager apply handler.
     * @param packet Attribute packet with audio manager settings.
     */
    ApplyAudioManager(const CAttributePacket& packet);

    /**
     * @brief Applies audio manager events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure audio manager.
     * @param packet Packet containing audio manager settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwReal m_fMasterVolume;    ///< Master volume (0.0 to 1.0)
    RwBool m_bReverbEnabled;   ///< Whether reverb effects are enabled
    RwInt32 m_iSpeakerConfig;  ///< Speaker configuration (stereo, surround, etc.)
    RwBool m_bDSPEnabled;      ///< Whether DSP effects are enabled
};

} // namespace RWS