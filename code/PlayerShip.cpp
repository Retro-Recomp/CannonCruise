// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.cpp)
// ============================================================================

#include "PlayerShip.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#if defined(RW_GL3) || defined(RW_D3D9)
// rw::im3d / Im3DVertex — GL3 ve D3D9 render yolları.
#include <rw.h>
#if defined(RW_GL3)
using ShipIm3DVertex = rw::gl3::Im3DVertex;
#else
using ShipIm3DVertex = rw::d3d::Im3DVertex;
#endif
#endif

namespace {

PlayerShipState s_ship = {};
RwBool s_active = FALSE;

const RwReal kMaxSpeed = 18.0f;    ///< tam gaza hedef hız (birim/sn)
const RwReal kReverseSpeed = -9.0f;///< tam geri gaza hedef hız
const RwReal kSpeedLerp = 0.9f;    ///< hız yakınsama katsayısı (1/sn)
const RwReal kTurnRate = 0.55f;    ///< rad/sn (tam rudder + tam hız)
const RwReal kWaterY = 0.0f;       ///< su seviyesi

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
    // Yerel uzay gövde: burun +z, kıç -z, omurga aşağı.
    // 0 burun ucu  1 iskele burun  2 sancak burun
    // 3 iskele kıç  4 sancak kıç  5 ayna (kıç orta)
    // 6 omurga burun  7 omurga iskele  8 omurga sancak
    // 9-12 direk (dikdörtgen, 2 üçgen)
    const RwV3d local[13] = {
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
    const RwUInt32 hullTris[][3] = {
        {0, 1, 2},             // güverte burun
        {0, 3, 1}, {0, 2, 4},  // güverte orta (dolgu)
        {0, 5, 3}, {0, 4, 5},  // güverte kıç
        {0, 6, 1}, {1, 6, 7},  // iskele gövde
        {1, 7, 3},
        {0, 2, 6}, {2, 8, 6},  // sancak gövde
        {2, 4, 8},
        {3, 7, 8}, {3, 8, 4},  // kıç ayna altı
    };
    const RwUInt32 mastTris[][3] = {
        {9, 11, 12}, {9, 12, 10},
    };

    const RwReal sy = std::sin(s_ship.yaw);
    const RwReal cy = std::cos(s_ship.yaw);
    RwV3d v[13];
    for (RwUInt32 i = 0; i < 13; ++i) {
        const RwReal lx = local[i].x;
        const RwReal lz = local[i].z;
        v[i].x = s_ship.pos.x + lx * cy + lz * sy;
        v[i].y = s_ship.pos.y + local[i].y;
        v[i].z = s_ship.pos.z - lx * sy + lz * cy;
    }

#if defined(RW_GL3) || defined(RW_D3D9)
    // Im3D: Transform tepeleri device state'e yükler; çizim indekslerle
    // yapılır (Ocean_W32 deseni). GL3/D3D9: Im3DVertex.
    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
    ShipIm3DVertex verts[64]; // 13 hull + 2 direk üçgeni = 45 tepe (pay bırak)
    RwUInt16 indices[64];
    const RwUInt32 hullCount = sizeof(hullTris) / sizeof(hullTris[0]);
    const RwUInt32 mastCount = sizeof(mastTris) / sizeof(mastTris[0]);
    RwUInt32 k = 0;
    for (RwUInt32 t = 0; t < hullCount; ++t) {
        for (RwUInt32 j = 0; j < 3; ++j) {
            ShipIm3DVertex &vx = verts[k];
            vx.setX(v[hullTris[t][j]].x);
            vx.setY(v[hullTris[t][j]].y);
            vx.setZ(v[hullTris[t][j]].z);
            vx.setNormalX(0.0f);
            vx.setNormalY(1.0f);
            vx.setNormalZ(0.0f);
            vx.setColor(96, 64, 32, 255);  // gövde: koyu ahşap
            vx.setU(0.0f);
            vx.setV(0.0f);
            indices[k] = (RwUInt16)k;
            ++k;
        }
    }
    for (RwUInt32 t = 0; t < mastCount; ++t) {
        for (RwUInt32 j = 0; j < 3; ++j) {
            ShipIm3DVertex &vx = verts[k];
            vx.setX(v[mastTris[t][j]].x);
            vx.setY(v[mastTris[t][j]].y);
            vx.setZ(v[mastTris[t][j]].z);
            vx.setNormalX(0.0f);
            vx.setNormalY(0.0f);
            vx.setNormalZ(1.0f);
            vx.setColor(233, 228, 214, 255);  // direk/yelken: kırık beyaz
            vx.setU(0.0f);
            vx.setV(0.0f);
            indices[k] = (RwUInt16)k;
            ++k;
        }
    }
    rw::im3d::Transform(verts, (int)k, nullptr, rw::im3d::EVERYTHING);
    rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, indices, (int)k);
    rw::im3d::End();
#else
    // NULL platform: indeks/köşe üretimi doğrulandı, cihaz draw no-op.
    (void)v;
    (void)hullTris;
    (void)mastTris;
    (void)sy;
    (void)cy;
#endif
}
