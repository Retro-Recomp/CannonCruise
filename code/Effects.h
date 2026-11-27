#ifndef CANNONCRUISE_EFFECTS_H
#define CANNONCRUISE_EFFECTS_H

// ============================================================================
// CannonCruise - Hit effects (Effects.h)
//
// Short-lived camera-facing sprites for cannonball impacts: a fire flash with
// rising smoke on ships/towers/land and a white splash on water. Drawn with
// the level's "vikingfireball" texture (the original particle bank,
// GameBank_PC.ppb, is not decoded yet).
// ============================================================================

#include "rwcore.h"

enum EffectKind {
    EFFECT_EXPLOSION = 0, ///< fire flash + smoke (ship, tower, ground hit)
    EFFECT_SPLASH = 1,    ///< white water splash
    EFFECT_MUZZLE = 2,    ///< small flash + puff at the cannon mouth
    EFFECT_WAKE = 3       ///< foam puff left behind a moving ship
};

/// Starts an effect at a world position.
void Effects_Spawn(EffectKind kind, RwReal x, RwReal y, RwReal z);

/// Advances all effects by dt seconds.
void Effects_Update(RwReal dt);

/// Draws the live effects (between BeginFrame/EndFrame, after the 3D scene).
void Effects_Render(void);

/// Removes every effect (level change).
void Effects_Clear(void);

#endif // CANNONCRUISE_EFFECTS_H
