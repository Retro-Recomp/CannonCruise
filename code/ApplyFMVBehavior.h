#pragma once

// ============================================================================
// CannonCruise - FMV Behavior Apply (ApplyFMVBehavior.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyFMVBehavior.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_0045F950 - FMV behavior apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyFMVBehavior
 * @brief Applies FMV (Full Motion Video) behavior attributes and configures
 *        video playback settings.
 *
 * Handles the Apply logic for FMV behavior, reading video file paths,
 * playback settings, and transition parameters from the attribute packet.
 *
 * **Binary references:**
 * - CreateFMVBehavior: `FUN_0045F580`
 * - ApplyFMVBehavior: `FUN_0045F950`
 */
class ApplyFMVBehavior : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyFMVBehavior);
    RWS_DECLARE_CLASS(ApplyFMVBehavior);

    /**
     * @brief Constructor — initializes FMV behavior apply handler.
     * @param packet Attribute packet with FMV settings.
     */
    ApplyFMVBehavior(const CAttributePacket& packet);

    /**
     * @brief Applies FMV behavior events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure FMV behavior.
     * @param packet Packet containing FMV settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwString m_sVideoFile;      ///< Video file path
    RwBool m_bLoop;             ///< Whether video should loop
    RwReal m_fStartTime;        ///< Start time in video (seconds)
    RwReal m_fEndTime;          ///< End time in video (seconds)
    RwBool m_bTransition;       ///< Whether to use transition effects
};

} // namespace RWS