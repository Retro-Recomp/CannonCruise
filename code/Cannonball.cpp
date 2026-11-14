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

    // Bullet dünyasından ve belleğinden çıkar
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
// Bullet küresel gövde (mass 10 kg) yaratıp dünyanın Bullet fazında
// kaydedip; ilk velocity'yi verir, 5 sn lifetime yapar, ses çalar.
// ---------------------------------------------------------------------------
void CCannonball::Spawn(const SCannonballSpawnData& spawnData)
{
    m_eType = spawnData.m_Type;
    m_uOwnerID = spawnData.m_uOwnerID;
    m_fLifeTimer = 5.0f; // Default 5.0 seconds maximum flight time

    // Bullet küre şekli — top büyüklüğünde ~0.8 m yarıçap
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
    if (m_fLifeTimer > 0.0f)
    {
        m_fLifeTimer -= deltaTime;
        if (m_fLifeTimer <= 0.0f)
        {
            RemoveFromWorld();
        }
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
    if (pAudio)
    {
        if (hitType == 0)
        {
            pAudio->PlaySound("Audio_CannonHit");
        }
        else if (hitType == 1)
        {
            pAudio->PlaySound("Audio_WaterHit");
        }
        else
        {
            pAudio->PlaySound("Audio_StaticHit");
        }
    }

    RemoveFromWorld();
}
