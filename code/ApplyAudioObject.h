#pragma once

// ============================================================================
// CannonCruise - Audio Object Apply (ApplyAudioObject.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioObject.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0044bef0 - audio object apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyAudioObject
 * @brief Applies audio object attributes and configures audio playback.
 *
 * Handles the Apply logic for audio objects, reading sound file paths,
 * volume, pitch, and playback flags from the attribute packet.
 *
 * **Binary references:**
 * - CreateAudioObject: `FUN_0044bef0`
 * - ApplyAudioObject: `FUN_0044c900`
 */
class ApplyAudioObject : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyAudioObject);
    RWS_DECLARE_CLASS(ApplyAudioObject);

    /**
     * @brief Constructor — initializes audio object apply handler.
     * @param packet Attribute packet with audio settings.
     */
    ApplyAudioObject(const CAttributePacket& packet);

    /**
     * @brief Applies audio object events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure audio object.
     * @param packet Packet containing audio settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bLoop;            ///< Whether sound should loop
    RwReal m_fVolume;          ///< Sound volume (0.0 to 1.0)
    RwReal m_fPitch;           ///< Sound pitch shift
    RwString m_sSoundFile;     ///< Sound file path
};

} // namespace RWS