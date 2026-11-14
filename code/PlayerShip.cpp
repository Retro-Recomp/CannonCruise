// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.cpp)
// ============================================================================

#include "PlayerShip.h"
#include "RenderPlatform.h"
#include "PhysicsWorld.h"
#include "PhysicsBackend.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#if defined(RW_GL3) || defined(RW_D3D9)
// librw retain-mode API (Clump/Geometry/Atomic) — since the Im3D path returned empty
// in D3D9, the boat is now drawn via the RpWorld pipeline.
#include <rw.h>
#endif

namespace {

PlayerShipState s_ship = {};
RwBool s_active = FALSE;
void *s_pShipBody = nullptr;
void *s_pShipShape = nullptr;
const RwReal kShipScale = 4.0f;    ///< placeholder scale (game world meters)

const RwReal kMaxSpeed = 18.0f;    ///< full throttle target speed (units/s)
const RwReal kReverseSpeed = -9.0f;///< full reverse throttle target speed
const RwReal kSpeedLerp = 0.9f;    ///< speed convergence coefficient (1/s)
const RwReal kTurnRate = 0.55f;    ///< rad/s (full rudder + full speed)
const RwReal kWaterY = 0.0f;       ///< water level

#if defined(RW_GL3) || defined(RW_D3D9)
// Single-use boat clump (lazy create, added to the world once).
rw::Clump *s_pShipClump = nullptr;
rw::Frame *s_pShipFrame = nullptr;
RwBool s_shipClumpAdded = FALSE;

const RwV3d kShipLocal[13] = {
    {0.0f, 1.1f, 5.2f},   // 0  bow
    {-1.35f, 1.0f, 2.1f}, // 1
    {1.35f, 1.0f, 2.1f},  // 2
    {-1.7f, 1.0f, -3.6f}, // 3
    {1.7f, 1.0f, -3.6f},  // 4
    {0.0f, 1.15f, -3.7f}, // 5
    {0.0f, -0.75f, 3.6f}, // 6
    {-0.95f, -0.75f, -3.1f}, // 7
    {0.95f, -0.75f, -3.1f},  // 8
    {-0.16f, 1.1f, 0.5f},   // 9  mast lower-left
    {0.16f, 1.1f, 0.5f},    // 10 mast lower-right
    {-0.16f, 4.8f, 0.5f},   // 11 mast upper-left
    {0.16f, 4.8f, 0.5f},    // 12 mast upper-right
};
const RwUInt32 kShipHullTris[][3] = {
    {0, 1, 2},             // bow deck
    {0, 3, 1}, {0, 2, 4},  // mid deck (filler)
    {0, 5, 3}, {0, 4, 5},  // stern deck
    {0, 6, 1}, {1, 6, 7},  // port hull
    {1, 7, 3},
    {0, 2, 6}, {2, 8, 6},  // starboard hull
    {2, 4, 8},
    {3, 7, 8}, {3, 8, 4},  // under the stern transom
};
const RwUInt32 kShipMastTris[][3] = {
    {9, 11, 12}, {9, 12, 10},
};

void ShipFrame_SetPose(rw::Frame *pFrame, const PlayerShipState &s) {
    rw::Matrix *pMat = &pFrame->matrix;
    const RwReal cy = std::cos(s.yaw), sy = std::sin(s.yaw);
    pMat->right = rw::makeV3d(cy, 0.0f, -sy);
    pMat->up = rw::makeV3d(0.0f, 1.0f, 0.0f);
    pMat->at = rw::makeV3d(sy, 0.0f, cy);
    pMat->pos = rw::makeV3d(s.pos.x, s.pos.y, s.pos.z);
    pFrame->updateObjects();
}

RwBool ShipClump_Create(void) {
    const RwUInt32 hullCount = sizeof(kShipHullTris) / sizeof(kShipHullTris[0]);
    const RwUInt32 mastCount = sizeof(kShipMastTris) / sizeof(kShipMastTris[0]);
    const RwUInt32 numTris = hullCount + mastCount;
    rw::Geometry *pGeo = rw::Geometry::create(13, numTris,
                                              rw::Geometry::POSITIONS |
                                                  rw::Geometry::NORMALS |
                                                  rw::Geometry::LIGHT |
                                                  rw::Geometry::PRELIT |
                                                  rw::Geometry::TEXTURED);
    if (pGeo == nullptr) {
        return FALSE;
    }
    rw::MorphTarget *mt = &pGeo->morphTargets[0];
    rw::V3d *verts = mt->vertices;
    rw::V3d *norms = mt->normals;
    // Placeholder proportions ~4x (game scale; original frigate ~40 units).
    const RwReal kShipScale = 4.0f;
    for (RwUInt32 i = 0; i < 13; ++i) {
        verts[i].set(kShipLocal[i].x * kShipScale, kShipLocal[i].y * kShipScale,
                     kShipLocal[i].z * kShipScale);
        if (norms) {
            norms[i].set(0.0f, 1.0f, 0.0f);
        }
    }
    for (RwUInt32 i = 0; i < numTris; ++i) {
        rw::Triangle &tri = pGeo->triangles[i];
        if (i < hullCount) {
            tri.v[0] = kShipHullTris[i][0];
            tri.v[1] = kShipHullTris[i][1];
            tri.v[2] = kShipHullTris[i][2];
        } else {
            tri.v[0] = kShipMastTris[i - hullCount][0];
            tri.v[1] = kShipMastTris[i - hullCount][1];
            tri.v[2] = kShipMastTris[i - hullCount][2];
        }
        tri.matId = 0;
    }
    // UV — pinned to the center of the Hugo boat texture (all corners the same → flat color).
    if (pGeo->texCoords[0] != nullptr) {
        for (RwUInt32 i = 0; i < 13; ++i) {
            pGeo->texCoords[0][i].u = 0.5f;
            pGeo->texCoords[0][i].v = 0.5f;
        }
    }
    // Hull: dark wood; masts/sails: broken white
    // Hull: light wood; masts/sails: broken white (contrast on the blue ocean)
    rw::RGBA wood = {150, 105, 60, 255};
    rw::RGBA sail = {245, 240, 225, 255};
    for (RwUInt32 i = 0; i < 13; ++i) {
        pGeo->colors[i] = (i >= 9) ? sail : wood;
    }
    rw::Material *pMat = rw::Material::create();
    if (pMat == nullptr) {
        pGeo->destroy();
        return FALSE;
    }
    pMat->color.red = 255;
    pMat->color.green = 255;
    pMat->color.blue = 255;
    pMat->color.alpha = 255;
    // Hugo kayak texture (GLOBAL.RWS -> s_menuTextures; loaded after boot).
    void *tex = RenderPlatform_GetTexture("Load_NT_003");
    if (tex != nullptr) {
        pMat->setTexture(static_cast<rw::Texture *>(tex));
    }
    pGeo->matList.appendMaterial(pMat);
    pMat->destroy();
    pGeo->calculateBoundingSphere();
    pGeo->unlock();

    rw::Atomic *pAtomic = rw::Atomic::create();
    rw::Frame *pFrame = rw::Frame::create();
    if (pAtomic == nullptr || pFrame == nullptr) {
        if (pAtomic) pAtomic->destroy();
        pGeo->destroy();
        return FALSE;
    }
    pAtomic->setGeometry(pGeo, 0);
    pGeo->destroy();
    s_pShipClump = rw::Clump::create();
    if (s_pShipClump == nullptr) {
        pAtomic->destroy();
        if (pFrame) pFrame->destroy();
        return FALSE;
    }
    pAtomic->setFrame(pFrame);
    s_pShipFrame = pFrame;
    ShipFrame_SetPose(s_pShipFrame, s_ship);
    s_pShipClump->addAtomic(pAtomic);
    return TRUE;
}
#endif // RW_GL3 || RW_D3D9

} // namespace

void PlayerShip_Spawn(const RwV3d *pPos, RwReal yaw) {
    std::memset(&s_ship, 0, sizeof(s_ship));
    if (pPos != nullptr) {
        s_ship.pos = *pPos;
    }
    s_ship.pos.y = kWaterY;
    s_ship.yaw = yaw;
    s_ship.speed = 0.0f;
    s_ship.throttle = 0.0f;
    s_ship.rudder = 0.0f;
    s_active = TRUE;
    // Build the hull as a convex hull shape — mass=0 kinematic body,
    // collisions trigger (required for Cannonball & EnemyShips).
    if (s_pShipShape == nullptr) {
        const RwReal hull[9][3] = {
            {0.0f, 1.1f, 5.2f}, {-1.35f, 1.0f, 2.1f}, {1.35f, 1.0f, 2.1f},
            {-1.7f, 1.0f, -3.6f}, {1.7f, 1.0f, -3.6f}, {0.0f, 1.15f, -3.7f},
            {0.0f, -0.75f, 3.6f}, {-0.95f, -0.75f, -3.1f}, {0.95f, -0.75f, -3.1f},
        };
        RwReal scaled[9][3];
        for (int i = 0; i < 9; ++i) {
            scaled[i][0] = hull[i][0] * kShipScale;
            scaled[i][1] = hull[i][1] * kShipScale;
            scaled[i][2] = hull[i][2] * kShipScale;
        }
        s_pShipShape = PhysicsShape_CreateConvexHull(&scaled[0][0], 9,
                                                      3 * sizeof(RwReal));
    }
    if (s_pShipBody == nullptr && s_pShipShape != nullptr) {
        const RwReal pos[3] = {s_ship.pos.x, s_ship.pos.y, s_ship.pos.z};
        s_pShipBody = PhysicsBody_CreateWithShape(s_pShipShape, 0.0f, pos,
                                                   nullptr);
        if (s_pShipBody != nullptr) {
            PhysicsBody_SetKinematic(s_pShipBody, TRUE);
            CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
            if (pWorld != nullptr) {
                pWorld->AddRigidBody(s_pShipBody);
            }
        }
    }
}

void PlayerShip_Shutdown(void) {
    CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
    if (s_pShipBody != nullptr) {
        if (pWorld != nullptr) {
            pWorld->RemoveRigidBody(s_pShipBody);
        }
        PhysicsBody_Destroy(s_pShipBody);
        s_pShipBody = nullptr;
    }
    if (s_pShipShape != nullptr) {
        PhysicsShape_Destroy(s_pShipShape);
        s_pShipShape = nullptr;
    }
    s_active = FALSE;
}

void PlayerShip_Update(RwReal deltaTime, RwReal waveT,
                       RwReal throttle, RwReal rudder) {
    if (s_active != TRUE) {
        return;
    }
    if (throttle > 1.0f) throttle = 1.0f;
    if (throttle < -1.0f) throttle = -1.0f;
    if (rudder > 1.0f) rudder = 1.0f;
    if (rudder < -1.0f) rudder = -1.0f;
    s_ship.throttle = throttle;
    s_ship.rudder = rudder;

    const RwReal target = (throttle >= 0.0f) ? throttle * kMaxSpeed
                                             : throttle * -kReverseSpeed;
    s_ship.speed += (target - s_ship.speed) * kSpeedLerp * deltaTime;

    // Rudder effectiveness scales with speed (no turning in place).
    const RwReal speedNorm = std::fabs(s_ship.speed) / kMaxSpeed;
    s_ship.yaw -= rudder * kTurnRate * (0.25f + 0.75f * speedNorm) *
                  ((s_ship.speed >= 0.0f) ? 1.0f : -1.0f) * deltaTime;

    s_ship.pos.x += std::sin(s_ship.yaw) * s_ship.speed * deltaTime;
    s_ship.pos.z += std::cos(s_ship.yaw) * s_ship.speed * deltaTime;
    // Wave-top bob -- exactly the same phase/amplitude as Ocean's wave form
    // (sin(x*0.05+t*2)*1.5 + cos(z*0.08+t*1.5)*1.0)
    const RwReal wave = std::sin(s_ship.pos.x * 0.05f + waveT * 2.0f) * 1.5f +
                        std::cos(s_ship.pos.z * 0.08f + waveT * 1.5f) * 1.0f;
    s_ship.pos.y = kWaterY + 0.8f + wave;
    // Bullet -- match the kinematic body position with the manual integration
    if (s_pShipBody != nullptr) {
        PhysicsBody_SetPosition(s_pShipBody, s_ship.pos.x, s_ship.pos.y,
                                s_ship.pos.z);
    }
}

RwBool PlayerShip_GetState(PlayerShipState *pOut) {
    if (pOut == nullptr || s_active != TRUE) {
        return FALSE;
    }
    *pOut = s_ship;
    return TRUE;
}

void PlayerShip_Render() {
    if (s_active != TRUE) {
        return;
    }
#if defined(RW_GL3) || defined(RW_D3D9)
    // RpWorld pipeline (default MATFx) -- Im3D is not drawn in D3D9.
    if (s_pShipClump == nullptr) {
        if (ShipClump_Create() != TRUE) {
            return;
        }
    }
    if (s_shipClumpAdded == FALSE) {
        if (RenderPlatform_AddClump(s_pShipClump) == TRUE) {
            s_shipClumpAdded = TRUE;
        }
    }
    // Update the pose every frame (frame matrix; RpWorld LTM uses this)
    ShipFrame_SetPose(s_pShipFrame, s_ship);
#endif
}
