// ============================================================================
// CannonCruise - HUD overlay (HUD.h)
// Reference layout (from the original game screenshot):
//   Bottom-left : ammo dial (GUI_022 frame + GUI_012 globe) + counter
//   Bottom      : red health bar with wooden end caps (GUI_026/GUI_024/GUI_028)
//   Middle-left : fish icon (GUI_031) + fish count
//   Bottom-right: compass ring (GUI_020) + north-up needle + tower blips
// Falls back to quad-fallback when no texture is present. Text uses the ShowText bitmap font.
// ============================================================================
#pragma once

#include "rwcore.h"

namespace RWS {

/** @brief Initializes HUD state (ammo=175, fish=0, health=1, score=0, towers=0). */
void HUD_Initialize(void);

/** @brief Uses ammo (clamped at 0). */
void HUD_UseAmmo(RwInt32 amount);

/** @brief Remaining cannonballs. */
RwInt32 HUD_GetAmmo(void);

/** @brief Adds collected fish. */
void HUD_AddFish(RwInt32 amount);

/** @brief Collected fish count. */
RwInt32 HUD_GetFish(void);

/** @brief Sets player health ratio [0..1]. */
void HUD_SetHealth(RwReal ratio);

/** @brief Current player health ratio [0..1]. */
RwReal HUD_GetHealth(void);

/** @brief Registers a tower as destroyed: +500 score; mission towers also
 *         advance the "TOWERS n/total" counter. */
void HUD_RegisterTowerDestroyed(RwBool bMissionTower = TRUE);

/** @brief Sets how many towers the mission requires (shown as n/total). */
void HUD_SetTowersTotal(RwInt32 total);

/** @brief Number of towers the mission requires. */
RwInt32 HUD_GetTowersTotal(void);

/** @brief Applies a score delta, clamped to [0, INT32_MAX]. */
void HUD_AddScore(RwInt32 amount);

/** @brief Total score so far. */
RwInt32 HUD_GetScore(void);

/** @brief Total number of towers destroyed so far. */
RwInt32 HUD_GetTowersDestroyed(void);

/** @brief Sets ship heading (yaw, game convention) for the compass needle. */
void HUD_SetShipHeading(RwReal yaw);

/** @brief Sets a compass blip (index 0..63): world-space bearing from ship
 *         (forward = +Z of bearing vector), and whether it is still alive. */
void HUD_SetTowerBlip(RwInt32 index, RwReal bearingX, RwReal bearingZ,
                      RwBool alive);

/** @brief Triggers a brief "NO AMMO" flash on the dial. */
void HUD_FlashNoAmmo(void);

/** @brief Adds the HUD draws to the im2d overlay pass. */
void HUD_Render(void);

} // namespace RWS
