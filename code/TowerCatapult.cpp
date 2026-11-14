// ============================================================================
// CannonCruise - Tower Catapult Defense Entity (TowerCatapult.cpp)
// Original path: D:\Projects\CannonCruisePC\code\TowerCatapult.cpp
// Reverse-engineered via Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_004bc3f0 — CTowerCatapult constructor & class registration ("CTowerCatapult")
//   FUN_004bcf30 — CTowerCatapult::Initialize ("Standard graphics (undamaged) not available...")
//   FUN_004be0b0 — CTowerCatapult::TakeDamage
//   FUN_004bf120 — CTowerCatapult destructor & world counter ("ms_nTowersAddedToWorlds != 0 Failed")
// ============================================================================

#include "TowerCatapult.h"
#include "AudioManager.h"
#include "PhysicsBackend.h"
#include "PhysicsWorld.h"
#include "framework/core/attributehandler/cclassfactory.h"
#include <cassert>

// Register CTowerCatapult with RenderWare Studio reflection system
RWS_REGISTER_CLASS(
    CTowerCatapult, []() -> void * { return new CTowerCatapult(); },
    sizeof(CTowerCatapult));

// Global counter tracking live defensive towers across scenes
RwUInt32 CTowerCatapult::sm_nTowersAddedToWorlds = 0;

// ---------------------------------------------------------------------------
// Constructor – mirrors FUN_004bc3f0.
// Initializes durability to 150 HP and increments global tower counter.
// ---------------------------------------------------------------------------
CTowerCatapult::CTowerCatapult()
    : m_pUndamagedClump(0), m_pDamagedClump(0), m_fHealth(150.0f),
      m_bDestroyed(FALSE), m_pShape(0) {
  m_vWorldPos.x = 0.0f;
  m_vWorldPos.y = 0.0f;
  m_vWorldPos.z = 0.0f;
  sm_nTowersAddedToWorlds++;
}

// ---------------------------------------------------------------------------
// Destructor – mirrors FUN_004bf120.
// Decrements global counter with assert check and destroys owned clumps.
// ---------------------------------------------------------------------------
CTowerCatapult::~CTowerCatapult() {
  assert(sm_nTowersAddedToWorlds != 0 && "ms_nTowersAddedToWorlds != 0 Failed");
  if (sm_nTowersAddedToWorlds > 0) {
    sm_nTowersAddedToWorlds--;
  }

  // Remove from the Bullet world and free from memory
  RemoveFromWorld();
  if (m_pShape) {
    PhysicsShape_Destroy(m_pShape);
    m_pShape = 0;
  }

  if (m_pUndamagedClump) {
    RpClumpDestroy(m_pUndamagedClump);
    m_pUndamagedClump = 0;
  }
  if (m_pDamagedClump) {
    RpClumpDestroy(m_pDamagedClump);
    m_pDamagedClump = 0;
  }
}

// ---------------------------------------------------------------------------
// HandleEvents – framework message listener for simulation events.
// ---------------------------------------------------------------------------
void CTowerCatapult::HandleEvents(const RWS::CMsg &msg) {}

// ---------------------------------------------------------------------------
// SetWorldPosition – creates a Bullet kinematic/static box instead of an RWS clump
// Tower the collision body so cannnonball's flight + bullet step can hit it.
// ---------------------------------------------------------------------------
void CTowerCatapult::SetWorldPosition(const RwV3d &pos) {
  // Tower dimensions — 16×36×16 m (flag watchtower)
  const RwReal halfExtents[3] = {8.0f, 18.0f, 8.0f};
  if (m_pShape == nullptr) {
    m_pShape = PhysicsShape_CreateBox(halfExtents);
  }
  if (m_pRigidBody == nullptr && m_pShape != nullptr) {
    const RwReal p[3] = {pos.x, pos.y + 18.0f, pos.z}; // place at the base-height center
    m_pRigidBody = PhysicsBody_CreateWithShape(m_pShape, 0.0f, p, nullptr);
    if (m_pRigidBody != nullptr) {
      PhysicsBody_SetKinematic(m_pRigidBody, TRUE);
    }
  }
  CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
  if (pWorld != nullptr && m_pRigidBody != nullptr) {
    pWorld->AddRigidBody(m_pRigidBody);
  }
  SetPosition(&pos); // set the entity's logical position
  m_vWorldPos = pos;
}

// ---------------------------------------------------------------------------
// Initialize – mirrors FUN_004bcf30.
// Binds intact and optional damaged clump models, validating undamaged clump.
// ---------------------------------------------------------------------------
void CTowerCatapult::Initialize(RpClump *pUndamagedClump,
                                RpClump *pDamagedClump) {
  assert(pUndamagedClump != 0 &&
         "Standard graphics (undamaged) not available for TowerCatapult !");
  assert(m_pUndamagedClump == 0 &&
         "Undamaged clump already loaded for TowerCatapult");

  m_pUndamagedClump = pUndamagedClump;
  m_pDamagedClump = pDamagedClump;
}

// ---------------------------------------------------------------------------
// TakeDamage – mirrors FUN_004be0b0.
// Subtracts damage from health pool and triggers DestroyTower() at or below 0.
// ---------------------------------------------------------------------------
void CTowerCatapult::TakeDamage(RwReal damage) {
  if (m_bDestroyed)
    return;

  m_fHealth -= damage;
  if (m_fHealth <= 0.0f) {
    DestroyTower();
  }
}

// ---------------------------------------------------------------------------
// DestroyTower – switches state to destroyed, plays "Audio_TowerDestroy".
// ---------------------------------------------------------------------------
void CTowerCatapult::DestroyTower() {
  m_bDestroyed = TRUE;

  CAudioManager *pAudio = CAudioManager::GetSingletonPtr();
  if (pAudio) {
    pAudio->PlaySound("Audio_TowerDestroy");
  }

  // Switch to damaged clump representation
}
