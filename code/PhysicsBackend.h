// ============================================================================
// CannonCruise - Bullet Physics Backend (PhysicsBackend.h)
// Original path: D:\Projects\CannonCruisePC\code\PhysicsBackend.h
// Reconstructed via reverse engineering with CC_ida.asm analysis.
//
// Portable physics engine (Bullet) instead of Havok. Keeps CPhysicsWorld's void*
// wrapper API; this module provides the backend.
//
// Binary functions (parameter sources):
//   sub_55E6D0 — game physics param defaults:
//     [+0x04] gravity = -9.8      [+0x20..28] AABB min = -500
//     [+0x30..38] AABB max = 500  [+0x44] friction = 0.6
//     [+0x48] = 1.0               [+0x4C] = 4
//     [+0x58] restitution = 0.3   [+0x80] solver iter = 20
//   sub_4B8710 — fixed physics step 1/60 (push 3C888889h — CSoldier tick)
//   FUN_0043a040 — CPhysicsWorld::Update → hkpWorld::stepDeltaTime (1/60)
//
// DEVIATIONS (documented): Bullet sequential-impulse solver ≠ Havok 2004 solver —
// an identical feel is not guaranteed; critical values are reverse engineered from
// the binary and fed into the backend parameters. The AABB ±500 broadphase limit
// mimics the Havok cinfo (Bullet uses a wide aabb; in-game clamp).
// ============================================================================
#pragma once

#include "rwcore.h"

/**
 * @brief Physics world — opaque btDiscreteDynamicsWorld package
 * (collision config + dispatcher + dbvt broadphase + impulse solver).
 */
struct PhysicsWorldHandle;

/**
 * @brief Dynamic body creation definition — box half extents,
 * origin: top-left-back (RwMatrix column-major pos).
 */
struct PhysicsBodyBoxDesc {
    RwReal halfExtents[3]; ///< half-box half extents
    RwReal mass;           ///< 0 → static
    RwReal friction;       ///< default 0.6 (sub_55E6D0[+0x44])
    RwReal restitution;    ///< default 0.3 (sub_55E6D0[+0x58])
    RwReal pos[3];         ///< world position
    RwReal rot3x3[9];      ///< column-major 3x3 rotation (empty → identity)
};

/**
 * @brief Static triangle mesh definition — generated from a Clump (CollisionGeometry).
 * vertexStride in bytes; typical 12 (3×RwReal) or the vertex struct size.
 */
struct PhysicsTriMeshDesc {
    const RwReal *pVertices; ///< vertex stream (with stride)
    RwInt32 vertexCount;
    RwInt32 vertexStride; ///< bytes
    const RwInt32 *pIndices; ///< list of 3-element indices
    RwInt32 indexCount;
    RwReal pos[3]; ///< mesh body position
};

// PhysicsWorld_Create — btDiscreteDynamicsWorld kurulumu. Gravity (0,-9.8,0)
// (sub_55E6D0[+0x04]); solver iterasyon 20 (sub_55E6D0[+0x80]).
PhysicsWorldHandle *PhysicsWorld_Create(void);
void PhysicsWorld_Destroy(PhysicsWorldHandle *pWorld);

// PhysicsWorld_Step — fixed-step simulation (binary: fixed 1/60; the caller
// provides it). Bullet stepSimulation(dt, maxSubSteps=1, dt) — a single fixed step.
void PhysicsWorld_Step(PhysicsWorldHandle *pWorld, RwReal deltaTime);

void PhysicsWorld_SetGravity(PhysicsWorldHandle *pWorld, RwReal x, RwReal y,
                             RwReal z);
void PhysicsWorld_GetGravity(const PhysicsWorldHandle *pWorld, RwReal *pX,
                             RwReal *pY, RwReal *pZ);

// Body life cycle — the returned void* is directly compatible with CPhysicsWorld
// Add/RemoveRigidBody (btRigidBody*).
void *PhysicsBody_CreateBox(const PhysicsBodyBoxDesc *pDesc);
// Static triangle mesh — btBvhTriangleMeshShape + a non-kinematic body
// (mass 0). btTriangleMesh holds the copy together with the body.
void *PhysicsBody_CreateTriMesh(const PhysicsTriMeshDesc *pDesc);
void PhysicsBody_Destroy(void *pBody);

// ---------------------------------------------------------------------------
// Standalone collision shapes — the CCollisionGeometry API counterpart.
// Ownership contract:
//   - The shape returned by PhysicsShape_Create* belongs to the caller; when given
//     to a body via PhysicsBody_CreateWithShape, the body does NOT take ownership of the SHAPE;
//     after the body is destroyed, PhysicsShape_Destroy must be called.
//   - PhysicsBody_CreateBox/CreateTriMesh bodies take ownership of their own shapes
//     (Destroy cleans up automatically).
// ---------------------------------------------------------------------------
void *PhysicsShape_CreateBox(const RwReal halfExtents[3]);
void *PhysicsShape_CreateSphere(RwReal radius);
// convex hull — for dynamic bodies (bConvex=TRUE path); the vertex stream is
// copied with stride (the shape keeps its own copy)
void *PhysicsShape_CreateConvexHull(const RwReal *pVertices,
                                    RwInt32 vertexCount, RwInt32 vertexStride);
// triangle mesh shape — the mesh data is kept in the shapeBundle together with the shape
void *PhysicsShape_CreateTriMesh(const PhysicsTriMeshDesc *pDesc);
void PhysicsShape_Destroy(void *pShape);
// body with a ready shape (mass 0 → static; shape ownership stays with the caller)
void *PhysicsBody_CreateWithShape(void *pShape, RwReal mass,
                                  const RwReal pos[3], const RwReal *pRot3x3ColMajor);

// Trigger (hkpPhantom counterpart) — ghost object AABB; overlap query via
// PhysicsTrigger_GetOverlaps. Massless, no collision response.
void *PhysicsTrigger_CreateBox(const RwReal halfExtents[3],
                               const RwReal pos[3]);
void PhysicsTrigger_Destroy(void *pTrigger);

// World registration (the actual call under CPhysicsWorld Add/Remove)
void PhysicsWorld_AddRigidBody(PhysicsWorldHandle *pWorld, void *pBody);
void PhysicsWorld_RemoveRigidBody(PhysicsWorldHandle *pWorld, void *pBody);
void PhysicsWorld_AddTrigger(PhysicsWorldHandle *pWorld, void *pTrigger);
void PhysicsWorld_RemoveTrigger(PhysicsWorldHandle *pWorld, void *pTrigger);

// State access — for RW frame sync (body → frame matrix).
void PhysicsBody_GetPosition(const void *pBody, RwReal *pX, RwReal *pY,
                             RwReal *pZ);
void PhysicsBody_GetRotation(const void *pBody, RwReal *pRot3x3ColMajor);
void PhysicsBody_SetPosition(void *pBody, RwReal x, RwReal y, RwReal z);
void PhysicsBody_SetKinematic(void *pBody, RwBool bKinematic);
void PhysicsBody_ApplyImpulse(void *pBody, RwReal x, RwReal y, RwReal z,
                              RwReal relX, RwReal relY, RwReal relZ);
void PhysicsBody_ApplyCentralImpulse(void *pBody, RwReal x, RwReal y,
                                     RwReal z);
void PhysicsBody_GetLinearVelocity(const void *pBody, RwReal *pX, RwReal *pY,
                                   RwReal *pZ);
void PhysicsBody_SetLinearVelocity(void *pBody, RwReal x, RwReal y, RwReal z);

// Trigger overlap — active bodies inside the ghost object (at most maxCount;
// return: number written). Source for the CPhysicsWorldBehaviour trigger flow.
RwInt32 PhysicsTrigger_GetOverlaps(const void *pTrigger, void **ppBodies,
                                   RwInt32 maxCount);

// Test access — backend world (the contents of CPhysicsWorld m_pHavokWorld).
PhysicsWorldHandle *PhysicsWorld_FromOpaque(void *pOpaque);
void *PhysicsWorld_ToOpaque(PhysicsWorldHandle *pWorld);
