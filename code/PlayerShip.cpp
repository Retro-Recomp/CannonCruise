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
// librw retain-mode API (Clump/Geometry/Atomic) — Im3D yolu D3D9'da boş
// döndürdüğü için tekne artık RpWorld pipeline'ı üzerinden çizilir.
#include <rw.h>
#endif

namespace {

PlayerShipState s_ship = {};
RwBool s_active = FALSE;
void *s_pShipBody = nullptr;
void *s_pShipShape = nullptr;
const RwReal kShipScale = 4.0f;    ///< placeholder ölçek (oyun dünyası metre)

const RwReal kMaxSpeed = 18.0f;    ///< tam gaza hedef hız (birim/sn)
const RwReal kReverseSpeed = -9.0f;///< tam geri gaza hedef hız
const RwReal kSpeedLerp = 0.9f;    ///< hız yakınsama katsayısı (1/sn)
const RwReal kTurnRate = 0.55f;    ///< rad/sn (tam rudder + tam hız)
const RwReal kWaterY = 0.0f;       ///< su seviyesi

#if defined(RW_GL3) || defined(RW_D3D9)
// Tek kullanımlık tekne clump'ı (lazy create, tek sefer world'e eklenir).
rw::Clump *s_pShipClump = nullptr;
rw::Frame *s_pShipFrame = nullptr;
RwBool s_shipClumpAdded = FALSE;

const RwV3d kShipLocal[13] = {
    {0.0f, 1.1f, 5.2f},   // 0  burun
    {-1.35f, 1.0f, 2.1f}, // 1
    {1.35f, 1.0f, 2.1f},  // 2
    {-1.7f, 1.0f, -3.6f}, // 3
    {1.7f, 1.0f, -3.6f},  // 4
    {0.0f, 1.15f, -3.7f}, // 5
    {0.0f, -0.75f, 3.6f}, // 6
    {-0.95f, -0.75f, -3.1f}, // 7
    {0.95f, -0.75f, -3.1f},  // 8
    {-0.16f, 1.1f, 0.5f},   // 9  direk sol-alt
    {0.16f, 1.1f, 0.5f},    // 10 direk sağ-alt
    {-0.16f, 4.8f, 0.5f},   // 11 direk sol-üst
    {0.16f, 4.8f, 0.5f},    // 12 direk sağ-üst
};
const RwUInt32 kShipHullTris[][3] = {
    {0, 1, 2},             // güverte burun
    {0, 3, 1}, {0, 2, 4},  // güverte orta (dolgu)
    {0, 5, 3}, {0, 4, 5},  // güverte kıç
    {0, 6, 1}, {1, 6, 7},  // iskele gövde
    {1, 7, 3},
    {0, 2, 6}, {2, 8, 6},  // sancak gövde
    {2, 4, 8},
    {3, 7, 8}, {3, 8, 4},  // kıç ayna altı
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
    // Placeholder oranları ~4x (oyun ölçeği; orijinal frigate ~40 birim).
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
    // UV — Hugo bot dokusunun ortasına sabit (tüm köşe aynı → düz renk).
    if (pGeo->texCoords[0] != nullptr) {
        for (RwUInt32 i = 0; i < 13; ++i) {
            pGeo->texCoords[0][i].u = 0.5f;
            pGeo->texCoords[0][i].v = 0.5f;
        }
    }
    // Gövde: koyu ahşap; direk/yelken: kırık beyaz
    // Gövde: açık ahşap; direk/yelken: kırık beyaz (mavi okyanusta kontrast)
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
    // Hugo kayak dokusu (GLOBAL.RWS → s_menuTextures; boot sonrası yüklü).
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
    // Hull'u convex hull shape olarak kur — mass=0 kinematic gövde,
    // collisions tetiklenir (Cannonball & EnemyShip'leri için gerekli).
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

    // Dümen etkinliği hızla ölçeklenir (yerinde dönüş yok).
    const RwReal speedNorm = std::fabs(s_ship.speed) / kMaxSpeed;
    s_ship.yaw -= rudder * kTurnRate * (0.25f + 0.75f * speedNorm) *
                  ((s_ship.speed >= 0.0f) ? 1.0f : -1.0f) * deltaTime;

    s_ship.pos.x += std::sin(s_ship.yaw) * s_ship.speed * deltaTime;
    s_ship.pos.z += std::cos(s_ship.yaw) * s_ship.speed * deltaTime;
    // Dalga üstü bob — Ocean'ın dalga formuyla birebir aynı faz/genlik
    // (sin(x*0.05+t*2)*1.5 + cos(z*0.08+t*1.5)*1.0)
    const RwReal wave = std::sin(s_ship.pos.x * 0.05f + waveT * 2.0f) * 1.5f +
                        std::cos(s_ship.pos.z * 0.08f + waveT * 1.5f) * 1.0f;
    s_ship.pos.y = kWaterY + 0.8f + wave;
    // Bullet — kinematic gövde pozisyonu el ile entegrasyona uy
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
    // RpWorld pipeline'ı (default MATFx) — Im3D D3D9'da çizilmedi.
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
    // Pose'u her frame güncelle (frame matrix; RpWorld LTM bunu kullanır)
    ShipFrame_SetPose(s_pShipFrame, s_ship);
#endif
}
