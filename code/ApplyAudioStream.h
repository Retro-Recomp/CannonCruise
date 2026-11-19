#pragma once

// ============================================================================
// CannonCruise - Audio Stream Apply (ApplyAudioStream.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyAudioStream.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0044d9d0 - audio stream apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyAudioStream
 * @brief Applies audio stream attributes and configures streaming audio.
 *
 * Handles the Apply logic for streaming audio, reading source file paths,
 * buffer sizes, and playback flags from the attribute packet.
 *
 * **Binary references:**
 * - CreateAudioStream: `FUN_004575B0`
 * - ApplyAudioStream: `FUN_0044d9d0`
 */
class ApplyAudioStream : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyAudioStream);
    RWS_DECLARE_CLASS(ApplyAudioStream);

    /**
     * @brief Constructor — initializes audio stream apply handler.
     * @param packet Attribute packet with audio stream settings.
     */
    ApplyAudioStream(const CAttributePacket& packet);

    /**
     * @brief Applies audio stream events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure audio stream.
     * @param packet Packet containing audio stream settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bLoop;                ///< Whether stream should loop
    RwReal m_fVolume;              ///< Stream volume (0.0 to 1.0)
    RwReal m_fPlaybackRate;      ///< Playback rate (1.0 = normal speed)
    RwString m_sSourceFile;        ///< Source audio file path
    RwUInt32 m_dwBufferSize;     ///< Audio buffer size in bytes
};

} // namespace RWS