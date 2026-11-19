#pragma once

// ============================================================================
// CannonCruise - Mission Indicator Apply (ApplyMissionIndicator.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyMissionIndicator.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyMissionIndicator
 * @brief Applies mission indicator attributes and controls mission display.
 *
 * Handles the Apply logic for mission indicators, reading mission progress,
 * completion flags, and display parameters from the attribute packet.
 *
 * **Binary references:**
 * - CreateMissionIndicator: mission system initialization
 * - ApplyMissionIndicator: mission system apply routine
 */
class ApplyMissionIndicator : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyMissionIndicator);
    RWS_DECLARE_CLASS(ApplyMissionIndicator);

    /**
     * @brief Constructor — initializes mission indicator apply handler.
     * @param packet Attribute packet with mission settings.
     */
    ApplyMissionIndicator(const CAttributePacket& packet);

    /**
     * @brief Applies mission indicator events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure mission indicator.
     * @param packet Packet containing mission settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwBool m_bCompleted;      ///< Whether mission is completed
    RwInt32 m_nScore;         ///< Mission score
    RwString m_sMissionName;  ///< Mission name identifier
};

} // namespace RWS