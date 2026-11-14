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
// librw im3d immediate-mode API (same path as the ocean tiles)
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
#if defined(RW_GL3)
using ShipIm3DVertex = rw::gl3::Im3DVertex;
#else
using ShipIm3DVertex = rw::d3d::Im3DVertex;
#endif

// Placeholder boat hull + mast, drawn immediate-mode (same path as the
// ocean tiles â€” known-good in D3D9). The RpWorld clump path renders
// nothing here, so we draw the 13-vertex low-poly boat each frame.
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
    {0, 2, 1},             // bow deck
    {0, 3, 1}, {0, 2, 4},  // mid deck (filler)
    {0, 5, 3}, {0, 4, 5},  // stern deck
    {0, 1, 6}, {1, 7, 6},  // port hull
    {1, 3, 7},
    {0, 6, 2}, {2, 6, 8},  // starboard hull
    {2, 8, 4},
    {3, 8, 7}, {3, 4, 8},  // under the stern transom
};
const RwUInt32 kShipMastTris[][3] = {
    {9, 12, 11}, {9, 10, 12},
};

void PlayerShip_RenderHull() {
    if (std::getenv("CANNONCRUISE_QUAD_TEST") != nullptr) {
        // ocean-like 2x2 subdivided quad at the ship position
        const RwReal cx = s_ship.pos.x, cz = s_ship.pos.z;
        ShipIm3DVertex im[9];
        const RwReal uvs[9][2] = {{0,0},{0.5f,0},{1,0},{0,0.5f},{0.5f,0.5f},{1,0.5f},{0,1},{0.5f,1},{1,1}};
        for (int i = 0; i < 9; ++i) {
            im[i].setX(cx - 15.0f + uvs[i][0] * 30.0f);
            im[i].setY(0.5f);
            im[i].setZ(cz - 15.0f + uvs[i][1] * 30.0f);
            im[i].setNormalX(0.0f);
            im[i].setNormalY(1.0f);
            im[i].setNormalZ(0.0f);
            im[i].setColor(255, 0, 255, 255);
            im[i].setU(uvs[i][0]);
            im[i].setV(uvs[i][1]);
        }
        RwUInt16 qIdx[24] = {0,1,3, 1,4,3, 1,2,4, 2,5,4,
                             3,4,6, 4,7,6, 4,5,7, 5,8,7};
        rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
        rw::im3d::Transform(im, 9, nullptr, rw::im3d::EVERYTHING);
        rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, (void *)qIdx, 24);
        rw::im3d::End();
        return;
    }
    if (s_active != TRUE) {
        return;
    }
    if (std::getenv("CANNONCRUISE_HULL_TRI") != nullptr) {
        ShipIm3DVertex im[3];
        const RwReal tx[3] = {25.0f, 44.0f, 34.4f};
        const RwReal ty[3] = {2.0f, 2.0f, 14.0f};
        const RwReal tz[3] = {390.0f, 390.0f, 390.0f};
        for (int i = 0; i < 3; ++i) {
            im[i].setX(tx[i]);
            im[i].setY(ty[i]);
            im[i].setZ(tz[i]);
            im[i].setNormalX(0.0f);
            im[i].setNormalY(1.0f);
            im[i].setNormalZ(0.0f);
            im[i].setColor(255, 0, 255, 255);
            im[i].setU(0.0f);
            im[i].setV(0.0f);
        }
        const char *mode = std::getenv("CANNONCRUISE_TRI_MODE");
        RwUInt32 flags =
            (mode != nullptr && mode[0] == '2')
                ? (rw::im3d::VERTEXXYZ | rw::im3d::VERTEXRGBA | rw::im3d::ALLOPAQUE)
                : rw::im3d::EVERYTHING;
        rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
        rw::im3d::Transform(im, 3, nullptr, flags);
        rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);
        rw::im3d::End();
        return;
    }
    // world = pos + right*lx + at*lz + up*ly  (same basis as the former
    // clump path: right=(cy,0,-sy), at=(sy,0,cy), up=(0,1,0))
    const RwReal cy = std::cos(s_ship.yaw);
    const RwReal sy = std::sin(s_ship.yaw);
    ShipIm3DVertex im[13];
    for (RwUInt32 i = 0; i < 13; ++i) {
        const RwReal lx = kShipLocal[i].x * kShipScale;
        const RwReal ly = kShipLocal[i].y * kShipScale;
        const RwReal lz = kShipLocal[i].z * kShipScale;
        im[i].setX(s_ship.pos.x + cy * lx + sy * lz);
        im[i].setY(s_ship.pos.y + ly);
        im[i].setZ(s_ship.pos.z - sy * lx + cy * lz);
        im[i].setNormalX(0.0f);
        im[i].setNormalY(1.0f);
        im[i].setNormalZ(0.0f);
        // wood for the hull, sail for the mast
        const rw::RGBA col = (i >= 9) ? rw::RGBA{245, 240, 225, 255}
                                      : rw::RGBA{150, 105, 60, 255};
        im[i].setColor(col.red, col.green, col.blue, col.alpha);
        im[i].setU(0.0f);
        im[i].setV(0.0f);
    }
    // indices: hull then mast (one indexed primitive)
    constexpr RwUInt32 kHullTri =
        (RwUInt32)(sizeof(kShipHullTris) / sizeof(kShipHullTris[0]));
    constexpr RwUInt32 kMastTri =
        (RwUInt32)(sizeof(kShipMastTris) / sizeof(kShipMastTris[0]));
    RwUInt16 indices[(kHullTri + kMastTri) * 3];
    RwUInt32 k = 0;
    for (RwUInt32 i = 0; i < sizeof(kShipHullTris) / sizeof(kShipHullTris[0]);
         ++i) {
        indices[k++] = (RwUInt16)kShipHullTris[i][0];
        indices[k++] = (RwUInt16)kShipHullTris[i][1];
        indices[k++] = (RwUInt16)kShipHullTris[i][2];
    }
    for (RwUInt32 i = 0; i < kMastTri; ++i) {
        indices[k++] = (RwUInt16)kShipMastTris[i][0];
        indices[k++] = (RwUInt16)kShipMastTris[i][1];
        indices[k++] = (RwUInt16)kShipMastTris[i][2];
    }
    // no texture (sampler off); same verified pattern as the ocean tiles
    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
    rw::im3d::Transform(im, 13, nullptr, rw::im3d::EVERYTHING);
    rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, indices, (int)k);
    rw::im3d::End();
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
    // Build the hull as a convex hull shape ÔÇö mass=0 kinematic body,
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
    PlayerShip_RenderHull();
#endif
}
