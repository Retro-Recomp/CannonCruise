// ============================================================================
// CannonCruise - Cannonball Projectile (Cannonball.cpp)
// Original path: D:\Projects\CannonCruisePC\code\Cannonball.cpp
// Reverse-engineered via Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_0046c2a0 — CCannonball constructor & registration
//   FUN_0046d1e0 — CCannonball::Spawn
//   FUN_0046d520 — CCannonball::OnHit
//   FUN_0046d950 — CCannonball destructor & world counter
// ============================================================================

#include "Cannonball.h"
#include "framework/core/attributehandler/cclassfactory.h"
#include "AudioManager.h"
#include "PhysicsBackend.h"
#include "PhysicsWorld.h"
#include "Effects.h"
#include "Ocean.h"
#include <cassert>

// Register CCannonball with RenderWare Studio's reflection class factory
RWS_REGISTER_CLASS(CCannonball, []() -> void* { return new CCannonball(); }, sizeof(CCannonball));

// Global counter tracking all live cannonballs across active scenes/worlds
RwUInt32 CCannonball::sm_nCannonBallsAddedToWorlds = 0;

// ---------------------------------------------------------------------------
// Constructor – mirrors FUN_0046c2a0.
// Initializes state variables, increments the world-wide cannonball counter.
// ---------------------------------------------------------------------------
CCannonball::CCannonball()
    : m_pClump(0),
      m_eType(CANNONBALL_STANDARD),
      m_fLifeTimer(0.0f),
      m_uOwnerID(0),
      m_pShape(0)
{
    sm_nCannonBallsAddedToWorlds++;
}

// ---------------------------------------------------------------------------
// Destructor – mirrors FUN_0046d950.
// Verifies counter consistency and destroys the associated RenderWare 3D clump.
// ---------------------------------------------------------------------------
CCannonball::~CCannonball()
{
    assert(sm_nCannonBallsAddedToWorlds != 0 && "ms_nCannonBallsAddedToWorlds != 0 Failed");
    if (sm_nCannonBallsAddedToWorlds > 0)
    {
        sm_nCannonBallsAddedToWorlds--;
    }

    // Remove from the Bullet world and its memory
    RemoveFromWorld();
    if (m_pShape)
    {
        PhysicsShape_Destroy(m_pShape);
        m_pShape = 0;
    }

    if (m_pClump)
    {
        RpClumpDestroy(m_pClump);
        m_pClump = 0;
    }
}

// ---------------------------------------------------------------------------
// HandleEvents – framework message listener for simulation ticks or triggers.
// ---------------------------------------------------------------------------
void CCannonball::HandleEvents(const RWS::CMsg& msg)
{
    // Framework event dispatch (unused in minimal projectile implementation)
}

// ---------------------------------------------------------------------------
// Spawn – mirrors FUN_0046d1e0.
// Create a spherical Bullet body (mass 10 kg) and, during the world's Bullet phase,
// register it; set the initial velocity, give it a 5 s lifetime, play a sound.
// ---------------------------------------------------------------------------
void CCannonball::Spawn(const SCannonballSpawnData& spawnData)
{
    m_eType = spawnData.m_Type;
    m_uOwnerID = spawnData.m_uOwnerID;
    m_fLifeTimer = 5.0f; // Default 5.0 seconds maximum flight time

    // Bullet sphere shape — roughly cannonball-sized, ~0.8 m radius
    if (m_pShape == 0) {
        m_pShape = PhysicsShape_CreateSphere(0.8f);
    }
    if (m_pShape != 0 && m_pRigidBody == 0) {
        const RwReal pos[3] = {spawnData.m_Pos.x, spawnData.m_Pos.y,
                               spawnData.m_Pos.z};
        m_pRigidBody = PhysicsBody_CreateWithShape(m_pShape, 10.0f, pos,
                                                    nullptr);
        if (m_pRigidBody != 0) {
            PhysicsBody_SetKinematic(m_pRigidBody, FALSE);
            PhysicsBody_SetLinearVelocity(m_pRigidBody, spawnData.m_Vel.x,
                                          spawnData.m_Vel.y,
                                          spawnData.m_Vel.z);
        }
    }

    SetPosition(&spawnData.m_Pos);
    SetLinearVelocity(&spawnData.m_Vel);
    AddToWorld();

    CAudioManager* pAudio = CAudioManager::GetSingletonPtr();
    if (pAudio)
    {
        pAudio->PlaySound("Audio_CannonBall", &spawnData.m_Pos);
    }
}

// ---------------------------------------------------------------------------
// Update – frame tick logic.
// Decrements the remaining life timer and automatically removes the projectile
// if it travels without hitting an obstacle before timing out.
// ---------------------------------------------------------------------------
void CCannonball::Update(RwReal deltaTime)
{
    if (m_fLifeTimer <= 0.0f || m_pRigidBody == 0)
    {
        return;
    }
    m_fLifeTimer -= deltaTime;
    RwV3d pos;
    PhysicsBody_GetPosition(m_pRigidBody, &pos.x, &pos.y, &pos.z);
    RwV3d vel;
    PhysicsBody_GetLinearVelocity(m_pRigidBody, &vel.x, &vel.y, &vel.z);
    // Splash down when the ball reaches the water surface.
    if (pos.y < COcean::GetWaterHeight(pos.x, pos.z))
    {
        OnHit(0, 1);
        return;
    }
    // Stopped on land (rock, beach): burst there.
    const RwReal speed2 = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
    if (m_fLifeTimer < 4.7f && speed2 < 4.0f)
    {
        OnHit(0, 2);
        return;
    }
    if (m_fLifeTimer <= 0.0f)
    {
        RemoveFromWorld();
    }
}

// ---------------------------------------------------------------------------
// OnHit – mirrors FUN_0046d520.
// Handles impact resolution based on collision surface:
//   hitType == 0 : Direct hit on target (enemy/player ship hull or fort)
//   hitType == 1 : Ocean water surface splashdown
//   hitType == * : Solid static world geometry (rock, cliff, building)
// Dispatches audio cue and cleans up projectile from physics simulation.
// ---------------------------------------------------------------------------
void CCannonball::OnHit(void* pTarget, RwUInt32 hitType)
{
    CAudioManager* pAudio = CAudioManager::GetSingletonPtr();
    RwV3d hitPos = {0.0f, 0.0f, 0.0f};
    const RwBool havePos = (m_pRigidBody != 0) ? TRUE : FALSE;
    if (havePos == TRUE)
    {
        PhysicsBody_GetPosition(m_pRigidBody, &hitPos.x, &hitPos.y, &hitPos.z);
    }
    if (pAudio)
    {
        const RwV3d *pAt = havePos == TRUE ? &hitPos : 0;
        if (hitType == 0)
        {
            pAudio->PlaySound("Audio_CannonHit", pAt);
        }
        else if (hitType == 1)
        {
            pAudio->PlaySound("Audio_WaterHit", pAt);
        }
        else
        {
            pAudio->PlaySound("Audio_StaticHit", pAt);
        }
    }
    if (m_pRigidBody != 0)
    {
        RwV3d pos;
        PhysicsBody_GetPosition(m_pRigidBody, &pos.x, &pos.y, &pos.z);
        Effects_Spawn(hitType == 1 ? EFFECT_SPLASH : EFFECT_EXPLOSION, pos.x,
                      hitType == 1 ? COcean::GetWaterHeight(pos.x, pos.z) : pos.y - 1.0f, pos.z);
    }
    m_fLifeTimer = 0.0f;

    RemoveFromWorld();
}
