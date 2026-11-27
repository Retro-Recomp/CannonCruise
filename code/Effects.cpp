// ============================================================================
// CannonCruise - Hit effects (Effects.cpp) -- see Effects.h
// ============================================================================

#include "Effects.h"

#include "RenderPlatform.h"

#include <cmath>
#include <cstdlib>
#include <vector>

#if defined(RW_GL3) || defined(RW_D3D9)
#include <rw.h>
#if defined(RW_GL3)
typedef rw::gl3::Im3DVertex FxVertex;
#else
typedef rw::d3d::Im3DVertex FxVertex;
#endif
#endif

namespace {

struct Sprite {
    RwReal x, y, z;       // centre
    RwReal vx, vy, vz;    // drift (m/s)
    RwReal age, life;     // seconds
    RwReal size0, size1;  // start / end size (m)
    RwUInt8 r, g, b;      // tint
    RwReal alpha0;        // start alpha (0..1), fades to 0
    bool fire;            // fireball texture (else the soft round puff)
};

std::vector<Sprite> s_sprites;

RwReal Rand01() { return (RwReal)std::rand() / (RwReal)RAND_MAX; }

void Add(RwReal x, RwReal y, RwReal z, RwReal vx, RwReal vy, RwReal vz, RwReal life,
         RwReal s0, RwReal s1, RwUInt8 r, RwUInt8 g, RwUInt8 b, RwReal a0) {
    if (s_sprites.size() > 600) return;
    // Orange/yellow tints are fire; grey/white ones smoke and spray.
    Sprite s = {x, y, z, vx, vy, vz, 0.0f, life, s0, s1, r, g, b, a0, (r > 200 && b < 150)};
    s_sprites.push_back(s);
}

} // namespace

void Effects_Spawn(EffectKind kind, RwReal x, RwReal y, RwReal z) {
    if (kind == EFFECT_EXPLOSION) {
        Add(x, y + 1.0f, z, 0, 0.5f, 0, 0.45f, 2.0f, 7.0f, 255, 200, 90, 1.0f); // flash
        for (int i = 0; i < 6; ++i) {                                              // fire
            Add(x + (Rand01() - 0.5f) * 2.0f, y + 0.8f + Rand01(), z + (Rand01() - 0.5f) * 2.0f,
                (Rand01() - 0.5f) * 3.0f, 2.0f + Rand01() * 2.0f, (Rand01() - 0.5f) * 3.0f,
                0.6f + Rand01() * 0.3f, 1.5f, 3.5f, 255, 140, 40, 0.95f);
        }
        for (int i = 0; i < 8; ++i) {                                              // smoke
            Add(x + (Rand01() - 0.5f) * 2.5f, y + 1.0f + Rand01(), z + (Rand01() - 0.5f) * 2.5f,
                (Rand01() - 0.5f) * 1.5f, 1.5f + Rand01() * 1.5f, (Rand01() - 0.5f) * 1.5f,
                1.6f + Rand01() * 0.8f, 2.0f, 6.0f, 90, 85, 80, 0.35f);
        }
    } else if (kind == EFFECT_SPLASH) {
        for (int i = 0; i < 10; ++i) {
            const RwReal a = Rand01() * 6.2831853f;
            Add(x, y + 0.3f, z, std::cos(a) * (1.0f + Rand01() * 2.0f), 4.0f + Rand01() * 4.0f,
                std::sin(a) * (1.0f + Rand01() * 2.0f), 0.7f + Rand01() * 0.3f, 0.8f, 2.2f, 235,
                245, 255, 0.85f);
        }
        Add(x, y + 0.2f, z, 0, 0, 0, 0.8f, 1.5f, 5.0f, 220, 235, 250, 0.6f); // foam ring
    } else if (kind == EFFECT_WAKE) {
        Add(x + (Rand01() - 0.5f) * 0.6f, y + 0.15f, z + (Rand01() - 0.5f) * 0.6f, 0, 0, 0,
            2.2f, 1.2f, 3.6f, 220, 235, 240, 0.30f);
    } else {
        Add(x, y, z, 0, 0.3f, 0, 0.15f, 0.8f, 2.0f, 255, 220, 120, 1.0f);
        for (int i = 0; i < 3; ++i) {
            Add(x, y, z, (Rand01() - 0.5f), 0.8f + Rand01(), (Rand01() - 0.5f), 0.9f, 0.8f, 2.5f,
                170, 165, 160, 0.6f);
        }
    }
}

void Effects_Update(RwReal dt) {
    for (size_t i = 0; i < s_sprites.size();) {
        Sprite &s = s_sprites[i];
        s.age += dt;
        if (s.age >= s.life) {
            s_sprites[i] = s_sprites.back();
            s_sprites.pop_back();
            continue;
        }
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.z += s.vz * dt;
        if (s.r > 200 && s.b > 200) s.vy -= 9.8f * dt; // splash droplets fall back
        ++i;
    }
}

void Effects_Render(void) {
#if defined(RW_GL3) || defined(RW_D3D9)
    if (s_sprites.empty() || RenderPlatform_IsInitialized() != TRUE) return;
    rw::Camera *pCam = static_cast<rw::Camera *>(RenderPlatform_GetCamera());
    if (pCam == nullptr || pCam->getFrame() == nullptr) return;
    const rw::Matrix *pView = pCam->getFrame()->getLTM();
    const rw::V3d right = pView->right, up = pView->up;
    rw::Texture *pTex = static_cast<rw::Texture *>(RenderPlatform_GetTexture("vikingfireball"));
    // Soft round puff for smoke and spray (generated once; the fireball
    // sprite's dark background showed as boxes under additive blending).
    if (RenderPlatform_HasTexture("FX_Soft") != TRUE) {
        static unsigned char s_puff[32 * 32 * 4];
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                const float dx = (x - 15.5f) / 15.5f, dy = (y - 15.5f) / 15.5f;
                float a = 1.0f - std::sqrt(dx * dx + dy * dy);
                if (a < 0.0f) a = 0.0f;
                unsigned char *px = &s_puff[(y * 32 + x) * 4];
                px[0] = px[1] = px[2] = (unsigned char)(255.0f * a);
                px[3] = (unsigned char)(255.0f * a * a);
            }
        }
        RenderPlatform_UploadTexture("FX_Soft", 32, 32, s_puff, 32 * 4);
    }
    rw::Texture *pSoft = static_cast<rw::Texture *>(RenderPlatform_GetTexture("FX_Soft"));
    std::vector<FxVertex> verts(s_sprites.size() * 4);
    std::vector<rw::uint16> idx(s_sprites.size() * 6);
    for (size_t i = 0; i < s_sprites.size(); ++i) {
        const Sprite &s = s_sprites[i];
        const RwReal t = s.age / s.life;
        const RwReal half = 0.5f * (s.size0 + (s.size1 - s.size0) * t);
        const int a = (int)(255.0f * s.alpha0 * (1.0f - t));
        const RwReal cx[4] = {-1, -1, 1, 1}, cy[4] = {-1, 1, -1, 1};
        for (int k = 0; k < 4; ++k) {
            FxVertex &v = verts[i * 4 + k];
            v.setX(s.x + (right.x * cx[k] + up.x * cy[k]) * half);
            v.setY(s.y + (right.y * cx[k] + up.y * cy[k]) * half);
            v.setZ(s.z + (right.z * cx[k] + up.z * cy[k]) * half);
            v.setNormalX(0.0f);
            v.setNormalY(1.0f);
            v.setNormalZ(0.0f);
            v.setColor(s.r, s.g, s.b, (rw::uint8)(a < 0 ? 0 : a));
            v.setU(cx[k] < 0 ? 0.0f : 1.0f);
            v.setV(cy[k] < 0 ? 1.0f : 0.0f);
        }
        const rw::uint16 b = (rw::uint16)(i * 4);
        const rw::uint16 q[6] = {b, (rw::uint16)(b + 1), (rw::uint16)(b + 2),
                                 (rw::uint16)(b + 2), (rw::uint16)(b + 1), (rw::uint16)(b + 3)};
        for (int k = 0; k < 6; ++k) idx[i * 6 + k] = q[k];
    }
    rw::SetRenderState(rw::VERTEXALPHA, 1);
    rw::SetRenderState(rw::SRCBLEND, rw::BLENDSRCALPHA);
    // Additive: the sprite's black background vanishes, fire and spray glow.
    rw::SetRenderState(rw::DESTBLEND, rw::BLENDONE);
    rw::SetRenderState(rw::ZWRITEENABLE, 0);
    rw::SetRenderState(rw::CULLMODE, rw::CULLNONE);
    // Two passes (fire with the fireball sprite, the rest with the puff);
    // each sprite is its own 4-vertex batch slice (im3d caps 10000 indices).
    for (int pass = 0; pass < 2; ++pass) {
        rw::Texture *t = pass == 0 ? pTex : pSoft;
        rw::SetRenderStatePtr(rw::TEXTURERASTER, t != nullptr ? t->raster : nullptr);
        std::vector<FxVertex> pv;
        std::vector<rw::uint16> pi;
        for (size_t i = 0; i < s_sprites.size(); ++i) {
            if (s_sprites[i].fire != (pass == 0)) continue;
            const rw::uint16 b = (rw::uint16)pv.size();
            for (int k = 0; k < 4; ++k) pv.push_back(verts[i * 4 + k]);
            const rw::uint16 q[6] = {b, (rw::uint16)(b + 1), (rw::uint16)(b + 2),
                                     (rw::uint16)(b + 2), (rw::uint16)(b + 1), (rw::uint16)(b + 3)};
            for (int k = 0; k < 6; ++k) pi.push_back(q[k]);
            if (pi.size() >= 9600) break;
        }
        if (pv.empty()) continue;
        rw::im3d::Transform(&pv[0], (int)pv.size(), nullptr, rw::im3d::EVERYTHING);
        rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, &pi[0], (int)pi.size());
        rw::im3d::End();
    }
    (void)idx;
    rw::SetRenderState(rw::ZWRITEENABLE, 1);
    rw::SetRenderState(rw::DESTBLEND, rw::BLENDINVSRCALPHA);
    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
#endif
}

void Effects_Clear(void) { s_sprites.clear(); }
