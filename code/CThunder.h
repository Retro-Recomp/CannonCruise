#pragma once

// ============================================================================
// CannonCruise - Thunder (CThunder.h)
// Original path: D:\Projects\CannonCruisePC\code\Thunder.h
// Reverse-engineered via Ghidra and CC_ida.asm analysis.
// Binary ref: ctor FUN_004BB5C0, audio FUN_004BBB70, render FUN_004BBF90
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include "RwsUtils.h"

namespace RWS
{

/**
 * @class CThunder
 * @brief Thunder/lightning effect entity.
 *
 * Represents a thunder sound and visual effect in the game world.
 * Binary-driven constructor sets up audio playback ("Audio_Thunder"),
 * render messages (MSG_THUNDER0_RENDER, MSG_THUNDER1_RENDER), and
 * float-based timing/intensity fields used by the renderer.
 *
 * **Binary references:**
 * - Ctor: `FUN_004BB5C0`
 * - Audio init: `FUN_004BBB70`
 * - Audio play: `SAudioPlayData*` + `"Audio_Thunder"`
 * - Render strings: `MSG_THUNDER0_RENDER`, `MSG_THUNDER1_RENDER`
 * - Helper funcs: `FUN_004D0C90`, `FUN_004CF020`
 */
class CThunder : public CEventHandler
{
public:
    RWS_MAKENEWCLASS(CThunder);
    RWS_DECLARE_CLASS(CThunder);

    /**
     * @brief Constructor — initializes thunder effect.
     * @param pos World position (unused in binary but stored for symmetry).
     */
    CThunder(const RwV3d& pos = RwV3d{ 0.0f, 0.0f, 0.0f });

    /**
     * @brief Destructor — stops audio and cleans up.
     */
    virtual ~CThunder();

    /**
     * @brief Play the thunder sound.
     */
    void PlaySound();

    /**
     * @brief Render the thunder visual effect.
     * @param renderMsg Render message ID (MSG_THUNDER0_RENDER or MSG_THUNDER1_RENDER).
     */
    void RenderEffect(RwInt32 renderMsg);

    /**
     * @brief Update thunder intensity/timer.
     * @param deltaTime Elapsed time in seconds.
     */
    void Update(RwReal deltaTime);

    /**
     * @brief Check if thunder is active.
     * @return TRUE if thunder effect is playing.
     */
    RwBool IsActive() const { return m_bActive; }

private:
    RwV3d    m_pos;          ///< World position
    RwBool   m_bActive;      ///< Thunder active flag
    // Float timing/intensity fields (binary layout):
    // +30h: float (1.0f = 3F800000h) — base intensity
    // +34h: dword from sub_4D0C90 — decay/gate value
    // +38h: dword from sub_4CF020 — additional modifier
    // +3Ch: float (1.0f = 3F800000h)
    // +40h: float (~0.8f = 3DCCCCCDh)
    // +44h: float (~0.8f = 3DCCCCCDh)
    // +48h: RwInt32 (1)
    // +4Ch: RwInt32 (5) — maybe note count or subdivisions
    // +50h: float (0.5f = 3F000000h) — half-intensity decay
    // +54h: float (0.5f = 3F000000h)
    // +58h: float (0.5f = 3F000000h)
    // +64h: RwInt32 (0)
    // +70h: RwInt32 (0)
    // +78h: RwInt32 (0)
    // +Audio fields: "Audio_Thunder" string, SAudioPlayData*
};

} // namespace RWS