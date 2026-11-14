// ============================================================================
// CannonCruise - Bullet Physics Backend (PhysicsBackend.cpp)
// Original path: D:\Projects\CannonCruisePC\code\PhysicsBackend.cpp
// Reconstructed via reverse engineering with CC_ida.asm analysis.
//
// Body counterparts — see the header block (sub_55E6D0 parameters).
// btTriangleMesh ownership: for mesh bodies, shape + mesh are kept
// together (MeshBundle) — the shape does not copy the mesh.
// ============================================================================

#include "PhysicsBackend.h"

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <LinearMath/btDefaultMotionState.h>
#include <LinearMath/btTransform.h>

#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>

// The opaque type from the header — must be defined at global scope (not in an anon ns)
struct PhysicsWorldHandle {
    btDefaultCollisionConfiguration *pConfig;
    btCollisionDispatcher *pDispatcher;
    btDbvtBroadphase *pBroadphase;
    btSequentialImpulseConstraintSolver *pSolver;
    btDiscreteDynamicsWorld *pWorld;
    // needed for the ghost (trigger) overlap list — otherwise
    // getNumOverlappingObjects returns 0 (Bullet contract)
    btGhostPairCallback *pGhostCallback;
};

namespace
{
    // sub_55E6D0 defaults
    const btScalar kGravityY = -9.8;    // [+0x04]
    const btScalar kFriction = 0.6;     // [+0x44]
    const btScalar kRestitution = 0.3;  // [+0x58]
    const int kSolverIterations = 20;   // [+0x80]
    // AABB ±500 ([+0x20..38]) — Bullet uses a large broadphase; in-game
    // clamp not yet applied (documented deviation)

    bool IsFinite3(const RwReal *pValues) {
        return pValues && std::isfinite(pValues[0]) &&
               std::isfinite(pValues[1]) && std::isfinite(pValues[2]);
    }

    bool IsValidHalfExtents(const RwReal *pHalfExtents) {
        return IsFinite3(pHalfExtents) && pHalfExtents[0] > 0.0f &&
               pHalfExtents[1] > 0.0f && pHalfExtents[2] > 0.0f;
    }

    bool IsValidRotation(const RwReal *pRotation) {
        if (!pRotation) {
            return true;
        }
        for (int i = 0; i < 9; ++i) {
            if (!std::isfinite(pRotation[i])) {
                return false;
            }
        }
        return true;
    }

    bool IsZeroRotation(const RwReal *pRotation) {
        if (!pRotation) {
            return true;
        }
        for (int i = 0; i < 9; ++i) {
            if (pRotation[i] != 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool IsValidVertexBuffer(const RwReal *pVertices, RwInt32 vertexCount,
                             RwInt32 vertexStride) {
        if (!pVertices || vertexCount < 3 ||
            vertexStride < 3 * static_cast<RwInt32>(sizeof(RwReal)) ||
            vertexStride % static_cast<RwInt32>(sizeof(RwReal)) != 0) {
            return false;
        }

        const std::size_t stride = static_cast<std::size_t>(vertexStride);
        const std::size_t vertexOffset =
            static_cast<std::size_t>(vertexCount - 1);
        const std::size_t maxOffset =
            static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) -
            3 * sizeof(RwReal);
        if (vertexOffset > maxOffset / stride) {
            return false;
        }

        const char *pBase = reinterpret_cast<const char *>(pVertices);
        for (RwInt32 i = 0; i < vertexCount; ++i) {
            const std::size_t offset = static_cast<std::size_t>(i) * stride;
            const RwReal *pVertex =
                reinterpret_cast<const RwReal *>(pBase + offset);
            if (!std::isfinite(pVertex[0]) || !std::isfinite(pVertex[1]) ||
                !std::isfinite(pVertex[2])) {
                return false;
            }
        }
        return true;
    }

    bool IsValidTriMeshDesc(const PhysicsTriMeshDesc *pDesc) {
        if (!pDesc || !pDesc->pIndices || pDesc->indexCount < 3 ||
            pDesc->indexCount % 3 != 0 ||
            !IsValidVertexBuffer(pDesc->pVertices, pDesc->vertexCount,
                                 pDesc->vertexStride) ||
            !IsFinite3(pDesc->pos)) {
            return false;
        }
        for (RwInt32 i = 0; i < pDesc->indexCount; ++i) {
            if (pDesc->pIndices[i] < 0 ||
                pDesc->pIndices[i] >= pDesc->vertexCount) {
                return false;
            }
        }
        return true;
    }

    const RwReal *RotationOrIdentity(const RwReal *pRotation) {
        return IsZeroRotation(pRotation) ? 0 : pRotation;
    }

    // static mesh bodies: shape + triangle mesh live together
    struct MeshBundle {
        btTriangleMesh *pMesh;
        btBvhTriangleMeshShape *pShape;
        btRigidBody *pBody;
    };

    // standalone mesh shapes (CCollisionGeometry CreateShapeFromClump):
    // the mesh carries the shape's lifetime
    struct ShapeBundle {
        btTriangleMesh *pMesh; // null for non-trimesh shapes
        btCollisionShape *pShape;
    };
    ShapeBundle **g_ppShapeReg = 0;
    int g_shapeRegCount = 0;
    int g_shapeRegCap = 0;

    void RegisterShape(void *pOpaque, ShapeBundle *pBundle) {
        if (g_shapeRegCount == g_shapeRegCap) {
            const int newCap = g_shapeRegCap ? g_shapeRegCap * 2 : 64;
            ShapeBundle **ppNew = new ShapeBundle *[newCap];
            for (int i = 0; i < g_shapeRegCount; ++i) {
                ppNew[i] = g_ppShapeReg[i];
            }
            delete[] g_ppShapeReg;
            g_ppShapeReg = ppNew;
            g_shapeRegCap = newCap;
        }
        (void)pOpaque;
        g_ppShapeReg[g_shapeRegCount++] = pBundle;
    }

    ShapeBundle *FindShapeBundle(void *pOpaque) {
        for (int i = 0; i < g_shapeRegCount; ++i) {
            // the returned pointer is a ShapeBundle* (void* cast)
            if (static_cast<void *>(g_ppShapeReg[i]) == pOpaque) {
                return g_ppShapeReg[i];
            }
        }
        return 0;
    }

    void UnregisterShape(void *pOpaque) {
        for (int i = 0; i < g_shapeRegCount; ++i) {
            if (static_cast<void *>(g_ppShapeReg[i]) == pOpaque) {
                g_ppShapeReg[i] = g_ppShapeReg[g_shapeRegCount - 1];
                --g_shapeRegCount;
                return;
            }
        }
    }

    // fill a btTriangleMesh from the given desc (shared path for CreateTriMesh/Shape)
    bool FillTriangleMesh(btTriangleMesh *pMesh,
                          const PhysicsTriMeshDesc *pDesc) {
        const char *pBase = reinterpret_cast<const char *>(pDesc->pVertices);
        for (RwInt32 i = 0; i + 2 < pDesc->indexCount; i += 3) {
            const RwInt32 i0 = pDesc->pIndices[i];
            const RwInt32 i1 = pDesc->pIndices[i + 1];
            const RwInt32 i2 = pDesc->pIndices[i + 2];
            const std::size_t stride =
                static_cast<std::size_t>(pDesc->vertexStride);
            const RwReal *v0 = reinterpret_cast<const RwReal *>(
                pBase + static_cast<std::size_t>(i0) * stride);
            const RwReal *v1 = reinterpret_cast<const RwReal *>(
                pBase + static_cast<std::size_t>(i1) * stride);
            const RwReal *v2 = reinterpret_cast<const RwReal *>(
                pBase + static_cast<std::size_t>(i2) * stride);
            pMesh->addTriangle(btVector3(v0[0], v0[1], v0[2]),
                               btVector3(v1[0], v1[1], v1[2]),
                               btVector3(v2[0], v2[1], v2[2]));
        }
        return pMesh->getNumTriangles() > 0;
    }

    btTransform MakeTransform(const RwReal *pPos, const RwReal *pRot3x3) {
        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(pPos[0], pPos[1], pPos[2]));
        if (pRot3x3) {
            // column-major 3x3 → btMatrix3x3 (row-major fill)
            btMatrix3x3 rot;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    rot[row][col] = pRot3x3[col * 3 + row];
                }
            }
            t.setBasis(rot);
        }
        return t;
    }

    btRigidBody *MakeBody(btScalar mass, const btTransform &worldTrans,
                          btCollisionShape *pShape, btScalar friction,
                          btScalar restitution) {
        btVector3 localInertia(0, 0, 0);
        if (mass != 0.0f) {
            pShape->calculateLocalInertia(mass, localInertia);
        }
        btDefaultMotionState *pMotion =
            new btDefaultMotionState(worldTrans);
        btRigidBody::btRigidBodyConstructionInfo info(mass, pMotion, pShape,
                                                      localInertia);
        info.m_friction = friction;      // sub_55E6D0[+0x44]
        info.m_restitution = restitution; // sub_55E6D0[+0x58]
        btRigidBody *pBody = new btRigidBody(info);
        pBody->setActivationState(DISABLE_DEACTIVATION);
        return pBody;
    }

    // void* body type discrimination — a MeshBundle pointer starts at the same
    // address as a btRigidBody*; a registry table is used to tell them apart.
    // Contract: the pointer returned by PhysicsBody_CreateTriMesh is a MeshBundle*,
    // the others are a direct btRigidBody*.
    struct BodyRegEntry {
        void *pOpaque;
        MeshBundle *pBundle; // non-null if static mesh
    };
    BodyRegEntry *g_pReg = 0;
    int g_regCount = 0;
    int g_regCap = 0;

    void RegisterBody(void *pOpaque, MeshBundle *pBundle) {
        if (g_regCount == g_regCap) {
            const int newCap = g_regCap ? g_regCap * 2 : 64;
            BodyRegEntry *pNew = new BodyRegEntry[newCap];
            for (int i = 0; i < g_regCount; ++i) {
                pNew[i] = g_pReg[i];
            }
            delete[] g_pReg;
            g_pReg = pNew;
            g_regCap = newCap;
        }
        g_pReg[g_regCount].pOpaque = pOpaque;
        g_pReg[g_regCount].pBundle = pBundle;
        ++g_regCount;
    }

    MeshBundle *FindBundle(void *pOpaque) {
        for (int i = 0; i < g_regCount; ++i) {
            if (g_pReg[i].pOpaque == pOpaque) {
                return g_pReg[i].pBundle;
            }
        }
        return 0;
    }

    void UnregisterBody(void *pOpaque) {
        for (int i = 0; i < g_regCount; ++i) {
            if (g_pReg[i].pOpaque == pOpaque) {
                g_pReg[i] = g_pReg[g_regCount - 1];
                --g_regCount;
                return;
            }
        }
    }

    btRigidBody *AsBody(void *pOpaque) {
        MeshBundle *pBundle = FindBundle(pOpaque);
        if (pBundle) {
            return pBundle->pBody;
        }
        return static_cast<btRigidBody *>(pOpaque);
    }

    const btRigidBody *AsBodyConst(const void *pOpaque) {
        return AsBody(const_cast<void *>(pOpaque));
    }
} // namespace

// ---------------------------------------------------------------------------
// world life cycle
// ---------------------------------------------------------------------------
PhysicsWorldHandle *PhysicsWorld_Create(void) {
    PhysicsWorldHandle *p = new PhysicsWorldHandle();
    p->pConfig = new btDefaultCollisionConfiguration();
    p->pDispatcher = new btCollisionDispatcher(p->pConfig);
    p->pBroadphase = new btDbvtBroadphase();
    p->pSolver = new btSequentialImpulseConstraintSolver();
    p->pWorld = new btDiscreteDynamicsWorld(
        p->pDispatcher, p->pBroadphase, p->pSolver, p->pConfig);
    p->pWorld->setGravity(btVector3(0, kGravityY, 0)); // sub_55E6D0[+0x04]
    p->pWorld->getSolverInfo().m_numIterations = kSolverIterations;
    // trigger (hkpPhantom → btGhostObject) overlap registration
    p->pGhostCallback = new btGhostPairCallback();
    p->pBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(
        p->pGhostCallback);
    return p;
}

void PhysicsWorld_Destroy(PhysicsWorldHandle *pWorld) {
    if (!pWorld) {
        return;
    }
    // body cleanup is the caller's responsibility (CPhysicsWorld dtor contract)
    delete pWorld->pWorld;
    delete pWorld->pGhostCallback;
    delete pWorld->pSolver;
    delete pWorld->pBroadphase;
    delete pWorld->pDispatcher;
    delete pWorld->pConfig;
    delete pWorld;
}

void PhysicsWorld_Step(PhysicsWorldHandle *pWorld, RwReal deltaTime) {
    if (!pWorld || !pWorld->pWorld) {
        return;
    }
    // Advance one Bullet step for the supplied dt. This is the intended
    // hkpWorld::stepDeltaTime mapping; original cadence equivalence is unverified.
    pWorld->pWorld->stepSimulation(deltaTime, 1, deltaTime);
}

void PhysicsWorld_SetGravity(PhysicsWorldHandle *pWorld, RwReal x, RwReal y,
                             RwReal z) {
    if (pWorld && pWorld->pWorld) {
        pWorld->pWorld->setGravity(btVector3(x, y, z));
    }
}

void PhysicsWorld_GetGravity(const PhysicsWorldHandle *pWorld, RwReal *pX,
                             RwReal *pY, RwReal *pZ) {
    if (!pWorld || !pWorld->pWorld) {
        return;
    }
    const btVector3 g = pWorld->pWorld->getGravity();
    if (pX) *pX = (RwReal)g.x();
    if (pY) *pY = (RwReal)g.y();
    if (pZ) *pZ = (RwReal)g.z();
}

// ---------------------------------------------------------------------------
// bodies
// ---------------------------------------------------------------------------
void *PhysicsBody_CreateBox(const PhysicsBodyBoxDesc *pDesc) {
    if (!pDesc || !IsValidHalfExtents(pDesc->halfExtents) ||
        !std::isfinite(pDesc->mass) || pDesc->mass < 0.0f ||
        !IsFinite3(pDesc->pos) || !IsValidRotation(pDesc->rot3x3) ||
        !std::isfinite(pDesc->friction) ||
        !std::isfinite(pDesc->restitution)) {
        return 0;
    }
    btBoxShape *pShape = new btBoxShape(btVector3(
        pDesc->halfExtents[0], pDesc->halfExtents[1], pDesc->halfExtents[2]));
    btTransform t =
        MakeTransform(pDesc->pos, RotationOrIdentity(pDesc->rot3x3));
    const btScalar friction =
        (pDesc->friction >= 0.0f) ? pDesc->friction : kFriction;
    const btScalar restitution =
        (pDesc->restitution >= 0.0f) ? pDesc->restitution : kRestitution;
    return MakeBody(pDesc->mass, t, pShape, friction, restitution);
}

void *PhysicsBody_CreateTriMesh(const PhysicsTriMeshDesc *pDesc) {
    if (!IsValidTriMeshDesc(pDesc)) {
        return 0;
    }
    MeshBundle *pBundle = new MeshBundle();
    pBundle->pMesh = new btTriangleMesh();
    if (!FillTriangleMesh(pBundle->pMesh, pDesc)) {
        delete pBundle->pMesh;
        delete pBundle;
        return 0;
    }
    pBundle->pShape =
        new btBvhTriangleMeshShape(pBundle->pMesh, /*useQuantized=*/true);
    btTransform t = MakeTransform(pDesc->pos, 0);
    pBundle->pBody = MakeBody(0.0f, t, pBundle->pShape, kFriction,
                              kRestitution); // static mesh
    RegisterBody(pBundle, pBundle); // void* → MeshBundle registration
    return pBundle;
}

// ---------------------------------------------------------------------------
// standalone collision shapes — the CCollisionGeometry counterpart
// ---------------------------------------------------------------------------
void *PhysicsShape_CreateBox(const RwReal halfExtents[3]) {
    if (!IsValidHalfExtents(halfExtents)) {
        return 0;
    }
    ShapeBundle *pBundle = new ShapeBundle();
    pBundle->pMesh = 0;
    pBundle->pShape = new btBoxShape(btVector3(
        halfExtents[0], halfExtents[1], halfExtents[2]));
    RegisterShape(pBundle, pBundle);
    return pBundle;
}

void *PhysicsShape_CreateSphere(RwReal radius) {
    if (!std::isfinite(radius) || radius <= 0.0f) {
        return 0;
    }
    ShapeBundle *pBundle = new ShapeBundle();
    pBundle->pMesh = 0;
    pBundle->pShape = new btSphereShape(radius);
    RegisterShape(pBundle, pBundle);
    return pBundle;
}

void *PhysicsShape_CreateConvexHull(const RwReal *pVertices,
                                    RwInt32 vertexCount, RwInt32 vertexStride) {
    if (!IsValidVertexBuffer(pVertices, vertexCount, vertexStride)) {
        return 0;
    }
    ShapeBundle *pBundle = new ShapeBundle();
    pBundle->pMesh = 0;
    btConvexHullShape *pShape = new btConvexHullShape();
    const char *pBase = reinterpret_cast<const char *>(pVertices);
    for (RwInt32 i = 0; i < vertexCount; ++i) {
        const std::size_t offset =
            static_cast<std::size_t>(i) *
            static_cast<std::size_t>(vertexStride);
        const RwReal *v = reinterpret_cast<const RwReal *>(pBase + offset);
        pShape->addPoint(btVector3(v[0], v[1], v[2]), /*recalcLocalAabb=*/false);
    }
    pShape->recalcLocalAabb();
    pBundle->pShape = pShape;
    RegisterShape(pBundle, pBundle);
    return pBundle;
}

void *PhysicsShape_CreateTriMesh(const PhysicsTriMeshDesc *pDesc) {
    if (!IsValidTriMeshDesc(pDesc)) {
        return 0;
    }
    ShapeBundle *pBundle = new ShapeBundle();
    pBundle->pMesh = new btTriangleMesh();
    if (!FillTriangleMesh(pBundle->pMesh, pDesc)) {
        delete pBundle->pMesh;
        delete pBundle;
        return 0;
    }
    pBundle->pShape =
        new btBvhTriangleMeshShape(pBundle->pMesh, /*useQuantized=*/true);
    RegisterShape(pBundle, pBundle);
    return pBundle;
}

void PhysicsShape_Destroy(void *pShape) {
    if (!pShape) {
        return;
    }
    ShapeBundle *pBundle = FindShapeBundle(pShape);
    if (pBundle) {
        delete pBundle->pShape;
        delete pBundle->pMesh;
        delete pBundle;
        UnregisterShape(pShape);
        return;
    }
    // box/sphere may be unregistered too (registration happens in every Create;
    // defensive) — delete as a direct shape: unknown pointer → ignore
}

void *PhysicsBody_CreateWithShape(void *pShape, RwReal mass,
                                  const RwReal pos[3],
                                  const RwReal *pRot3x3ColMajor) {
    if (!pShape || !IsFinite3(pos) || !std::isfinite(mass) || mass < 0.0f ||
        !IsValidRotation(pRot3x3ColMajor)) {
        return 0;
    }
    btCollisionShape *pBtShape;
    ShapeBundle *pBundle = FindShapeBundle(pShape);
    if (pBundle) {
        pBtShape = pBundle->pShape;
    } else {
        pBtShape = static_cast<btCollisionShape *>(pShape);
    }
    btTransform t = MakeTransform(pos, RotationOrIdentity(pRot3x3ColMajor));
    btRigidBody *pBody =
        MakeBody(mass, t, pBtShape, kFriction, kRestitution);
    // userIndex=1 → shape came from outside; Destroy does not take ownership
    pBody->setUserIndex(1);
    return pBody;
}

void PhysicsBody_Destroy(void *pBody) {
    if (!pBody) {
        return;
    }
    MeshBundle *pBundle = FindBundle(pBody);
    if (pBundle) {
        btMotionState *pMS = pBundle->pBody->getMotionState();
        delete pBundle->pBody;
        delete pMS;
        delete pBundle->pShape;
        delete pBundle->pMesh;
        delete pBundle;
        UnregisterBody(pBody);
        return;
    }
    btRigidBody *pRb = static_cast<btRigidBody *>(pBody);
    btMotionState *pMS = pRb->getMotionState();
    btCollisionShape *pShape = pRb->getCollisionShape();
    // userIndex=1 → shape came from outside (CreateWithShape); no ownership
    const bool ownsShape = (pRb->getUserIndex() != 1);
    delete pRb;
    delete pMS;
    if (pShape && ownsShape) {
        delete pShape;
    }
}

void *PhysicsTrigger_CreateBox(const RwReal halfExtents[3],
                               const RwReal pos[3]) {
    if (!IsValidHalfExtents(halfExtents) || !IsFinite3(pos)) {
        return 0;
    }
    btBoxShape *pShape = new btBoxShape(btVector3(
        halfExtents[0], halfExtents[1], halfExtents[2]));
    btGhostObject *pGhost = new btGhostObject();
    pGhost->setCollisionShape(pShape);
    pGhost->setCollisionFlags(pGhost->getCollisionFlags() |
                              btCollisionObject::CF_NO_CONTACT_RESPONSE);
    pGhost->setWorldTransform(
        MakeTransform(pos, 0));
    return pGhost;
}

void PhysicsTrigger_Destroy(void *pTrigger) {
    if (!pTrigger) {
        return;
    }
    btGhostObject *pGhost = static_cast<btGhostObject *>(pTrigger);
    btCollisionShape *pShape = pGhost->getCollisionShape();
    delete pGhost;
    delete pShape;
}

// ---------------------------------------------------------------------------
// world registration
// ---------------------------------------------------------------------------
void PhysicsWorld_AddRigidBody(PhysicsWorldHandle *pWorld, void *pBody) {
    if (pWorld && pWorld->pWorld && pBody) {
        pWorld->pWorld->addRigidBody(AsBody(pBody));
    }
}

void PhysicsWorld_RemoveRigidBody(PhysicsWorldHandle *pWorld, void *pBody) {
    if (pWorld && pWorld->pWorld && pBody) {
        pWorld->pWorld->removeRigidBody(AsBody(pBody));
    }
}

void PhysicsWorld_AddTrigger(PhysicsWorldHandle *pWorld, void *pTrigger) {
    if (pWorld && pWorld->pWorld && pTrigger) {
        pWorld->pWorld->addCollisionObject(
            static_cast<btGhostObject *>(pTrigger),
            btBroadphaseProxy::SensorTrigger,
            btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
    }
}

void PhysicsWorld_RemoveTrigger(PhysicsWorldHandle *pWorld, void *pTrigger) {
    if (pWorld && pWorld->pWorld && pTrigger) {
        pWorld->pWorld->removeCollisionObject(
            static_cast<btCollisionObject *>(pTrigger));
    }
}

// ---------------------------------------------------------------------------
// state access
// ---------------------------------------------------------------------------
void PhysicsBody_GetPosition(const void *pBody, RwReal *pX, RwReal *pY,
                             RwReal *pZ) {
    if (!pBody) {
        return;
    }
    const btRigidBody *pRb = AsBodyConst(pBody);
    const btVector3 o = pRb->getWorldTransform().getOrigin();
    if (pX) *pX = (RwReal)o.x();
    if (pY) *pY = (RwReal)o.y();
    if (pZ) *pZ = (RwReal)o.z();
}

void PhysicsBody_GetRotation(const void *pBody, RwReal *pRot3x3ColMajor) {
    if (!pBody || !pRot3x3ColMajor) {
        return;
    }
    const btRigidBody *pRb = AsBodyConst(pBody);
    const btMatrix3x3 &b = pRb->getWorldTransform().getBasis();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            pRot3x3ColMajor[col * 3 + row] = (RwReal)b[row][col];
        }
    }
}

void PhysicsBody_SetPosition(void *pBody, RwReal x, RwReal y, RwReal z) {
    btRigidBody *pRb = AsBody(pBody);
    if (!pRb) {
        return;
    }
    btTransform t = pRb->getWorldTransform();
    t.setOrigin(btVector3(x, y, z));
    pRb->setWorldTransform(t);
    if (pRb->getMotionState()) {
        pRb->getMotionState()->setWorldTransform(t);
    }
}

void PhysicsBody_SetKinematic(void *pBody, RwBool bKinematic) {
    btRigidBody *pRb = AsBody(pBody);
    if (!pRb) {
        return;
    }
    if (bKinematic) {
        pRb->setCollisionFlags(pRb->getCollisionFlags() |
                               btCollisionObject::CF_KINEMATIC_OBJECT);
        pRb->setActivationState(DISABLE_DEACTIVATION);
    } else {
        pRb->setCollisionFlags(pRb->getCollisionFlags() &
                               ~btCollisionObject::CF_KINEMATIC_OBJECT);
    }
}

void PhysicsBody_ApplyImpulse(void *pBody, RwReal x, RwReal y, RwReal z,
                              RwReal relX, RwReal relY, RwReal relZ) {
    btRigidBody *pRb = AsBody(pBody);
    if (pRb) {
        pRb->applyImpulse(btVector3(x, y, z),
                          btVector3(relX, relY, relZ));
    }
}

void PhysicsBody_ApplyCentralImpulse(void *pBody, RwReal x, RwReal y,
                                     RwReal z) {
    btRigidBody *pRb = AsBody(pBody);
    if (pRb) {
        pRb->applyCentralImpulse(btVector3(x, y, z));
    }
}

void PhysicsBody_GetLinearVelocity(const void *pBody, RwReal *pX, RwReal *pY,
                                   RwReal *pZ) {
    if (!pBody) {
        return;
    }
    const btRigidBody *pRb = AsBodyConst(pBody);
    const btVector3 v = pRb->getLinearVelocity();
    if (pX) *pX = (RwReal)v.x();
    if (pY) *pY = (RwReal)v.y();
    if (pZ) *pZ = (RwReal)v.z();
}

void PhysicsBody_SetLinearVelocity(void *pBody, RwReal x, RwReal y, RwReal z) {
    btRigidBody *pRb = AsBody(pBody);
    if (pRb) {
        pRb->setLinearVelocity(btVector3(x, y, z));
    }
}

RwInt32 PhysicsTrigger_GetOverlaps(const void *pTrigger, void **ppBodies,
                                   RwInt32 maxCount) {
    if (!pTrigger || !ppBodies || maxCount <= 0) {
        return 0;
    }
    const btGhostObject *pGhost =
        static_cast<const btGhostObject *>(pTrigger);
    RwInt32 n = 0;
    for (int i = 0;
         i < pGhost->getNumOverlappingObjects() && n < maxCount; ++i) {
        ppBodies[n++] = const_cast<btCollisionObject *>(
            pGhost->getOverlappingObject(i));
    }
    return n;
}

// ---------------------------------------------------------------------------
// opaque bridge
// ---------------------------------------------------------------------------
PhysicsWorldHandle *PhysicsWorld_FromOpaque(void *pOpaque) {
    return static_cast<PhysicsWorldHandle *>(pOpaque);
}

void *PhysicsWorld_ToOpaque(PhysicsWorldHandle *pWorld) {
    return pWorld;
}
