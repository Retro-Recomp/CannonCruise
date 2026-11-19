#pragma once

// ============================================================================
// CannonCruise - Difficulty Levels Apply (ApplyDifficultyLevels.h)
// Original path: D:\Projects\CannonCruisePC\code\ApplyDifficultyLevels.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: FUN_00443B10 - difficulty levels apply
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"

namespace RWS
{

/**
 * @class ApplyDifficultyLevels
 * @brief Applies difficulty levels attributes and configures game difficulty.
 *
 * Handles the Apply logic for difficulty levels, reading difficulty
 * setting, enemy count, and time limits from the attribute packet.
 *
 * **Binary references:**
 * - CreateDifficultyLevels: `FUN_00443B10`
 * - ApplyDifficultyLevels: `FUN_00443E00`
 */
class ApplyDifficultyLevels : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(ApplyDifficultyLevels);
    RWS_DECLARE_CLASS(ApplyDifficultyLevels);

    /**
     * @brief Constructor — initializes difficulty levels apply handler.
     * @param packet Attribute packet with difficulty settings.
     */
    ApplyDifficultyLevels(const CAttributePacket& packet);

    /**
     * @brief Applies difficulty levels events.
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Apply attribute packet to configure difficulty levels.
     * @param packet Packet containing difficulty settings.
     * @return TRUE if applied successfully.
     */
    RwBool Apply(const CAttributePacket& packet);

private:
    RwInt32 m_difficultySetting;   ///< Difficulty level (0=Easy, 1=Normal, 2=Hard, 3=Expert)
    RwInt32 m_enemyCount;          ///< Number of enemies enabled
    RwReal m_fTimeLimit;           ///< Time limit in seconds (0=unlimited)
    RwBool m_bFriendlyFire;        ///< Whether friendly fire is enabled
};

} // namespace RWS