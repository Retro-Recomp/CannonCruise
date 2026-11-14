// ============================================================================
// CannonCruise - Loading Screen (LoadingScreen.h)
// Phase 4.6ab — the Loading.anm scene in GLOBAL.RWS: parchment strip, wave
// border, Hugo's boat drifting right while the "LOADING" text shows (SCR.mov frames 4/7).
//
// Shown for a few seconds after global boot, BEFORE the world is AddClump'ed.
// The art (Load_NT_001/003/005, rwID_2DMAESTRO texture) is already loaded during
// the GLOBAL boot phase; otherwise the fallback drawing (dark background + bitmap text)
// kicks in. CANNONCRUISE_LOADING_FRAMES=0 → always skip.
// ============================================================================
#pragma once

#include "rwcore.h"

/// Draws the loading screen for `frames` frames. Ends early if Esc/Space/Enter or
/// the window closes.
/// @return FALSE → the app should shut down; TRUE → continue (world setup begins).
RwBool LoadingScreen_Play(int frames, int screenW, int screenH);
