// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.h)
// Phase 4.6s (ship follow) -- 2026-10-17.
//
// The original CShipPlayer model ({GUID}.dff) is unavailable. This module
// provides a clean-room low-poly longboat with layered planks, rigging, oars,
// a deck cannon and crew figure; its kinematic Bullet body mirrors movement.
//
// Motion model (experimental, not binary-inspired):
//   - throttle: W=+1 (ileri), S=-0.5 (geri)
//   - speed -> throttle * kMaxSpeed (smooth transition, exp)
//   - rudder: A=+1 (port), D=-1 (starboard); yaw rate proportional to speed
//   - position: forward vector on the y plane + wave bob
// ============================================================================
#pragma once

#include "rwcore.h"

struct PlayerShipState {
    RwV3d pos;      ///< World position (y = water surface + bob)
    RwReal yaw;     ///< Radians; forward vector = (sin(yaw), 0, cos(yaw))
    RwReal speed;   ///< Current forward speed (units/sec)
    RwReal throttle;///< -1..+1
    RwReal rudder;  ///< -1..+1 (positive = port/starboard turn)
};

/// Spawns the ship into the scene. If pPos is null, (0,0,0) is used.
void PlayerShip_Spawn(const RwV3d *pPos, RwReal yaw);

/// Sets the outer XZ water-area bounds; the hull is kept inside with clearance.
/// Returns FALSE and disables map bounds when the extents are invalid or too small.
RwBool PlayerShip_SetMapBounds(RwReal minX, RwReal maxX,
                               RwReal minZ, RwReal maxZ);

/// Removes the kinematic rigid body created at spawn from the PhysicsWorld
/// and frees it (to avoid Bullet's empty-vector assertion in the dtor
/// when the application shuts down).
void PlayerShip_Shutdown(void);

/// throttle/rudder -> kinematic step. waveT is the wave phase (sec).
void PlayerShip_Update(RwReal deltaTime, RwReal waveT,
                       RwReal throttle, RwReal rudder);

/// Active scene state (FALSE if Spawn was not called).
RwBool PlayerShip_GetState(PlayerShipState *pOut);

/// Low-poly longboat rendering -- rw::im3d TRILIST (real GPU rendering in GL3
/// builds; in NULL builds drawing is skipped).
void PlayerShip_Render();
