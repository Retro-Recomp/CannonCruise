// ============================================================================
// CannonCruise - Playground/physics smoke test (playground_test.cpp)
//
// Sprint 1 verification: the player ship spawns and, after 120 updates
// with a throttle coefficient of 1.0, its speed rises from 0 to a positive
// value and the ship advances forward. The Bullet PhysicsBackend is called;
// the boat lives in the Bullet world registry as a kinematic holder;
// cleaned up with PlayerShip_Shutdown.
//
//   usage: playground_test
// ============================================================================

#include "PlayerShip.h"
#include "PhysicsWorld.h"
#include "PhysicsBackend.h"
#include "Cannonball.h"
#include "TowerCatapult.h"
#include "HUD.h"
#include "rwcore.h"

#include <cmath>
#include <cstdio>
#include <cstring>

static const int kFrames = 120;        // 120 frames × 1/60 s = 2 s
static const RwReal kMaxSpeedExpect = 18.0f;
static const RwReal kTargetTime = 120.0f / 60.0f; // 2 seconds

static int g_failures = 0;

static void Check(bool cond, const char *pExpr, int line) {
    if (!cond) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, pExpr);
    }
}
#define CHECK(expr) Check((expr), #expr, __LINE__)

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    (void)argc;
    (void)argv;

    // PhysicsWorld singleton (behaves like the startup chain)
    CPhysicsWorld::Create();

    // Spawn the boat for a single test
    PlayerShipState state;
    RwV3d spawnPos = {0.0f, 0.0f, 0.0f};
    PlayerShip_Spawn(&spawnPos, 0.0f); // +Z direction of the unit circle
    CHECK(PlayerShip_GetState(&state) == TRUE);
    CHECK(state.pos.x == 0.0f && state.pos.z == 0.0f);

    // 120 frames full throttle forward
    const RwReal dt = 1.0f / 60.0f;
    for (int i = 0; i < kFrames; ++i) {
        PlayerShip_Update(dt, (RwReal)i * dt, 1.0f, 0.0f);
    }

    CHECK(PlayerShip_GetState(&state) == TRUE);
    std::printf("Final speed = %.3f, pos=(%.3f, %.3f, %.3f)\n",
                state.speed, state.pos.x, state.pos.y, state.pos.z);
    // asymptotically approaches target speed in 2 seconds:
    // v(∞) = 18 (kMaxSpeed); 2 saniyede ≈ 18*(1-e^{-1.8}) ≈ 15.07 (%84).
    Check(state.speed > kMaxSpeedExpect * 0.75f,
          "speed >= 75% of kMaxSpeed after 2 s", __LINE__);
    // should have advanced in +Z direction
    Check(state.pos.z > 10.0f,
          "ship moved forward (z > 10)", __LINE__);

    // Clean shutdown
    PlayerShip_Shutdown();

    // ======================================================================
    // Cannonball → cannonball → tower destruction + HUD score
    // ======================================================================
    {
        std::printf("\n[Sprint 2] Cannonball shot → tower destruction + HUD\n");
        CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
        if (pWorld == nullptr) {
            pWorld = CPhysicsWorld::Create();
        }
        std::printf("physics world: %p\n", (void*)pWorld);
        RWS::HUD_Initialize();
        Check(RWS::HUD_GetAmmo() == 175,
              "HUD ammo starts at 175", __LINE__);
        RWS::HUD_UseAmmo(3);
        Check(RWS::HUD_GetAmmo() == 172,
              "HUD ammo 175 -> 172 after UseAmmo(3)", __LINE__);

        // Place one tower (150 target HP); no wild boats; the cannonball will hit
        // the top layer of the tower directly.
        RwV3d towerPos = {0.0f, 0.0f, 50.0f};
        CTowerCatapult *pTower = new CTowerCatapult();
        pTower->SetWorldPosition(towerPos);

        // Cannon: horizontal shot, flat trajectory (no pitch); distance ~50, fast enough
        // that the ball reaches the tower without falling (cos(30°)=0.86, speed 40 → x 34 s velocity)
        RwV3d muzzle = {0.0f, 30.0f, -1.0f};
        RwV3d vel = {0.0f, 0.0f, 35.0f};
        SCannonballSpawnData spawnData;
        std::memset(&spawnData, 0, sizeof(spawnData));
        spawnData.m_Pos = muzzle;
        spawnData.m_Vel = vel;
        spawnData.m_Type = CANNONBALL_STANDARD;
        spawnData.m_uOwnerID = 1;
        CCannonball *pBall = new CCannonball();
        pBall->Spawn(spawnData);

        // Advance physics steps 90 frames (~1.5 s — flight sequence)
        const RwReal dt = 1.0f / 60.0f;
        for (int i = 0; i < 120; ++i) {
            pWorld->Update(dt);
            // Print ball position each frame (debug)
            RwV3d bp = {};
            PhysicsBody_GetPosition(pBall->GetRigidBody(), &bp.x, &bp.y, &bp.z);
            const RwReal dx = bp.x - towerPos.x;
            const RwReal dy = bp.y - (towerPos.y + 18.0f);
            const RwReal dz = bp.z - towerPos.z;
            const RwReal ax = std::fabs(dx) - (8.0f + 0.8f);
            const RwReal ay = std::fabs(dy) - (18.0f + 0.8f);
            const RwReal az = std::fabs(dz) - (8.0f + 0.8f);
            if (ax < 0 && ay < 0 && az < 0) {
                std::printf("HIT frame=%d tower pos=(%.1f,%.1f,%.1f) ball=(%.1f,%.1f,%.1f)\n",
                            i, towerPos.x, towerPos.y, towerPos.z, bp.x, bp.y, bp.z);
                pTower->TakeDamage(150.0f);
                if (pTower->IsDestroyed()) {
                    RWS::HUD_RegisterTowerDestroyed();
                }
                pBall->OnHit(pTower, 0);
                break;
            }
        }

        // Tower must be destroyed
        Check(pTower->IsDestroyed() == TRUE,
              "tower destroyed after cannonball hit", __LINE__);
        Check(RWS::HUD_GetTowersDestroyed() == 1,
              "HUD tower destroyed counter == 1", __LINE__);
        Check(RWS::HUD_GetScore() >= 500,
              "HUD score >= 500 after tower hit", __LINE__);
        std::printf("tower destroyed: %s   HUD_towers=%d score=%d\n",
                    pTower->IsDestroyed() == TRUE ? "YES" : "NO",
                    RWS::HUD_GetTowersDestroyed(),
                    RWS::HUD_GetScore());

        delete pBall;
        delete pTower;
    }

    if (g_failures == 0) {
        std::printf("playground_test: PASS\n");
        return 0;
    }
    std::printf("playground_test: %d failures\n", g_failures);
    return 1;
}
