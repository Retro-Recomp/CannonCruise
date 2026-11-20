#pragma once

// ============================================================================
// CannonCruise - Bird Sea Gull Apply (ApplyBirdSeaGull.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyBirdSeaGull.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/cclassfactory.h"

namespace RWS
{

/**
 * @class ApplyBirdSeaGull
 * @brief Applies sea gull attributes and configures bird behavior.
 *
 * Handles the Apply logic for sea gull birds, reading bird settings,
 * flight parameters, and audio cues from the attribute packet.
 *
 * **Binary references:**
 * - CreateBirdSeaGull: bird system initialization
 * - ApplyBirdSeaGull: bird system apply routine
 */
class ApplyBirdSeaGull : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyBirdSeaGull);
    RWS_DECLARE_CLASS(ApplyBirdSeaGull);

    /**
     * @brief Constructor — initializes sea gull apply handler.
     * @param packet Attribute packet with bird settings.
     */
    ApplyBirdSeaGull(const CAttributePacket& packet);

    virtual ~ApplyBirdSeaGull();

    /**
     * @brief Applies sea gull events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure sea gull.
     * @param packet Packet containing bird settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    char m_sAudioCue[256];     ///< Audio cue for gull sounds
    RwReal m_fFlightHeight;   ///< Default flight height
    RwReal m_fSpeed;          ///< Default flight speed
    RwBool m_bMigration;      ///< Whether bird migrates
};

} // namespace RWS