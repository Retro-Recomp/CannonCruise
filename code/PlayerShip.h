// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.h)
// Phase 4.6s (ship follow) -- 2026-10-17.
//
// Actual player ship: CShipPlayer (ShipPlayer.cpp, binary
// sub_4AE6C0; 0x488 bytes) is registered with the factory but its 3D model ({GUID}.dff)
// is not available to us -- like the world mesh, the external build-output asset is missing.
// This module provides a LOW-POLYGON placeholder ship (hull + mast) and a
// simple kinematic motion model so the game loop can be verified.
// The physics body (Bullet) will be connected in a later phase
// (CPhysicsEntity::SetPosition/GetPosition are ready).
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

/// Removes the kinematic rigid body created at spawn from the PhysicsWorld
/// and frees it (to avoid Bullet's empty-vector assertion in the dtor
/// when the application shuts down).
void PlayerShip_Shutdown(void);

/// throttle/rudder -> kinematic step. waveT is the wave phase (sec).
void PlayerShip_Update(RwReal deltaTime, RwReal waveT,
                       RwReal throttle, RwReal rudder);

/// Active scene state (FALSE if Spawn was not called).
RwBool PlayerShip_GetState(PlayerShipState *pOut);

/// Placeholder hull rendering -- rw::im3d TRILIST (real GPU rendering in GL3 builds;
/// in NULL builds index generation is verified, drawing is skipped).
void PlayerShip_Render();
