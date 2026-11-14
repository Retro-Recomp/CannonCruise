// ============================================================================
// CannonCruise - HUD overlay implementation (HUD.cpp)
// Reference layout (from the original game screenshot): bottom-left ammo dial, bottom health bar,
// fish counter, bottom-right compass (north-up, red tower blips).
// ============================================================================

#include "HUD.h"
#include "RenderPlatform.h"
#include "ShowText.h"
#include "rwcore.h"

#include <rw.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace RWS {

struct TowerBlip {
    RwReal x;     ///< world-space bearing X (tower - ship)
    RwReal z;     ///< world-space bearing Z (tower - ship)
    RwBool alive;
};

static RwReal s_health = 1.0f;         // 0..1 (player ship HP ratio)
static RwInt32 s_ammo = 175;           // remaining cannonballs (original game feel)
static RwInt32 s_fish = 0;             // collected fish
static RwInt32 s_score = 0;            // cumulative score
static RwInt32 s_towersDestroyed = 0;  // towers shot down
static const RwInt32 kTowersTotal = 2; // Viking-1 has 2 towers
static RwReal s_shipYaw = 0.0f;        // for the compass needle
static TowerBlip s_blips[2] = {{0.0f, 1.0f, TRUE}, {0.0f, 1.0f, TRUE}};
static RwInt32 s_noAmmoFlash = 0;      // "NO AMMO" flash counter (frames)

void HUD_Initialize(void) {
    s_health = 1.0f;
    s_ammo = 175;
    s_fish = 0;
    s_score = 0;
    s_towersDestroyed = 0;
    s_shipYaw = 0.0f;
    s_blips[0] = {0.0f, 1.0f, TRUE};
    s_blips[1] = {0.0f, 1.0f, TRUE};
    s_noAmmoFlash = 0;
}

void HUD_UseAmmo(RwInt32 amount) {
    s_ammo -= amount;
    if (s_ammo < 0) s_ammo = 0;
}

RwInt32 HUD_GetAmmo(void) { return s_ammo; }

void HUD_AddFish(RwInt32 amount) { s_fish += amount; }

RwInt32 HUD_GetFish(void) { return s_fish; }

void HUD_SetHealth(RwReal ratio) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    s_health = ratio;
}

void HUD_RegisterTowerDestroyed(void) {
    ++s_towersDestroyed;
    s_score += 500;
}

void HUD_AddScore(RwInt32 amount) { s_score += amount; }

RwInt32 HUD_GetScore(void) { return s_score; }

RwInt32 HUD_GetTowersDestroyed(void) { return s_towersDestroyed; }

void HUD_SetShipHeading(RwReal yaw) { s_shipYaw = yaw; }

void HUD_SetTowerBlip(RwInt32 index, RwReal bearingX, RwReal bearingZ,
                      RwBool alive) {
    if (index < 0 || index >= 2) return;
    s_blips[index].x = bearingX;
    s_blips[index].z = bearingZ;
    s_blips[index].alive = alive;
}

void HUD_FlashNoAmmo(void) { s_noAmmoFlash = 45; } // ~0.75 sec @60fps

// Needle: white, rotates relative to the ship's heading on the north-up compass (im2d triangle).
static void DrawNeedle(RwReal cx, RwReal cy, RwReal relAngleRad,
                       RwReal radius, RwUInt32 argb) {
    const RwReal fx = std::sin(relAngleRad);
    const RwReal fz = -std::cos(relAngleRad); // north = -Z = screen y up
    const RwReal px = std::cos(relAngleRad);  // perpendicular (to the right)
    const RwReal pz = std::sin(relAngleRad);
    const RwReal tipX = cx + fx * radius;
    const RwReal tipY = cy + fz * radius;
    const RwReal baseLX = cx - fx * radius * 0.45f + px * radius * 0.30f;
    const RwReal baseLY = cy - fz * radius * 0.45f + pz * radius * 0.30f;
    const RwReal baseRX = cx - fx * radius * 0.45f - px * radius * 0.30f;
    const RwReal baseRY = cy - fz * radius * 0.45f - pz * radius * 0.30f;
    const RwReal z = rw::im2d::GetNearZ();
    RwIm2DVertex v[3];
    const RwReal xs[3] = {tipX, baseLX, baseRX};
    const RwReal ys[3] = {tipY, baseLY, baseRY};
    const RwUInt8 r0 = (RwUInt8)((argb >> 16) & 0xFF);
    const RwUInt8 g0 = (RwUInt8)((argb >> 8) & 0xFF);
    const RwUInt8 b0 = (RwUInt8)(argb & 0xFF);
    const RwUInt8 a = (RwUInt8)((argb >> 24) & 0xFF);
#if defined(RW_D3D9)
    const RwUInt8 r = b0, g = g0, b = r0;
#else
    const RwUInt8 r = r0, g = g0, b = b0;
#endif
    for (int i = 0; i < 3; ++i) {
        v[i].x = xs[i];
        v[i].y = ys[i];
        v[i].z = z;
        v[i].rhw = 1.0f;
        v[i].color.red = r;
        v[i].color.green = g;
        v[i].color.blue = b;
        v[i].color.alpha = a;
        v[i].u = 0.0f;
        v[i].v = 0.0f;
    }
    rw::im2d::RenderPrimitive(rw::PRIMTYPETRILIST, v, 3);
}

// 5x7 bitmap text width (6px per scale)
static RwReal TextWidth(const char *s, RwInt32 scale) {
    return (RwReal)std::strlen(s) * 6.0f * (RwReal)scale;
}

static void DrawScaledText(const char *s, RwReal x, RwReal y, RwInt32 scale,
                           RwUInt32 argb) {
    ShowText_DrawBitmapText(s, x, y, scale,
                            (RwUInt8)((argb >> 16) & 0xFF),
                            (RwUInt8)((argb >> 8) & 0xFF),
                            (RwUInt8)(argb & 0xFF),
                            (RwUInt8)((argb >> 24) & 0xFF));
}

static void DrawAnchoredText(const char *s, RwReal cx, RwReal y,
                             RwInt32 scale, RwUInt32 argb) {
    DrawScaledText(s, cx - TextWidth(s, scale) * 0.5f, y, scale, argb);
}

void HUD_Render(void) {
    const RwReal kScreenW = 1280.0f;
    const RwReal kScreenH = 720.0f;
    const RwBool hasArt = (RenderPlatform_HasTexture("GUI_022") == TRUE);

    RenderPlatform_BeginOverlay2D();

    // ================= Bottom-left: ammo dial =================
    const RwReal dialX = 20.0f, dialSize = 128.0f;
    const RwReal dialY = kScreenH - 168.0f; // 552
    const RwReal dialCX = dialX + dialSize * 0.5f;

    if (hasArt) {
        RenderPlatform_DrawTexturedQuad(dialX, dialY, dialSize, dialSize,
                                        "GUI_022");
        // globe (dark ball) - circular center of the dial
        const RwReal globeSize = 74.0f;
        const RwReal globeX = dialX + (dialSize - globeSize) * 0.5f;
        const RwReal globeY = dialY + 16.0f;
        RenderPlatform_DrawTexturedQuad(globeX, globeY, globeSize,
                                        globeSize, "GUI_012");
    } else {
        RenderPlatform_DrawOverlayQuad(dialX, dialY, dialSize, dialSize,
                                       0xFF8B6914u);
        RenderPlatform_DrawOverlayQuad(dialX + 20, dialY + 20,
                                       dialSize - 40, dialSize - 40,
                                       0xFF1A1A1Au);
    }

    // Ammo count (above the globe)
    char ammoText[16];
    std::snprintf(ammoText, sizeof(ammoText), "%d", s_ammo);
    DrawAnchoredText(ammoText, dialCX, dialY + 44.0f, 3, 0xFFFFE666u);

    // "NO AMMO" flash
    if (s_noAmmoFlash > 0) {
        if ((s_noAmmoFlash / 6) % 2 == 0) {
            DrawAnchoredText("NO AMMO", dialCX, dialY - 22.0f, 2,
                             0xFFFF4040u);
        }
        --s_noAmmoFlash;
    }

    // ================= Bottom: health bar (wooden end caps + red) =================
    const RwReal barX = 16.0f, barY = kScreenH - 40.0f; // 680
    const RwReal barH = 22.0f, capW = 26.0f, barW = 216.0f;
    const RwReal midX = barX + capW, midW = barW - capW * 2.0f;
    if (hasArt) {
        // Middle rail (GUI_024) — stretch to fill the slot
        RenderPlatform_DrawTexturedQuad(midX, barY, midW, barH, "GUI_024");
        // Inner fill (red) - sits in the white socket of the rail
        const RwReal innerX = midX + 6.0f, innerW = midW - 12.0f;
        const RwReal innerY = barY + 8.0f, innerH = 6.0f;
        RenderPlatform_DrawOverlayQuad(innerX, innerY, innerW, innerH,
                                       0xFF302018u);
        if (s_health > 0.0f) {
            // Bright red when full; darkens as it drains
            const RwUInt8 rr = (RwUInt8)(140 + 100 * s_health);
            const RwUInt8 gg = (RwUInt8)(30 * s_health);
            const RwUInt8 bb = (RwUInt8)(20 * s_health);
            const RwUInt32 fill =
                0xFF000000u | ((RwUInt32)rr << 16) | ((RwUInt32)gg << 8) | bb;
            RenderPlatform_DrawOverlayQuad(innerX, innerY,
                                           innerW * s_health, innerH, fill);
        }
        RenderPlatform_DrawTexturedQuad(barX, barY, capW, barH, "GUI_026");
        RenderPlatform_DrawTexturedQuad(barX + barW - capW, barY, capW,
                                        barH, "GUI_028");
    } else {
        RenderPlatform_DrawOverlayQuad(barX, barY, barW, barH, 0xFF101010u);
        RenderPlatform_DrawOverlayQuad(barX + 2, barY + 2, barW - 4,
                                       barH - 4, 0xFF302018u);
        if (s_health > 0.0f) {
            const RwUInt8 rr = (RwUInt8)(140 + 100 * s_health);
            const RwUInt8 gg = (RwUInt8)(30 * s_health);
            const RwUInt8 bb = (RwUInt8)(20 * s_health);
            const RwUInt32 fill =
                0xFF000000u | ((RwUInt32)rr << 16) | ((RwUInt32)gg << 8) | bb;
            RenderPlatform_DrawOverlayQuad(barX + 2, barY + 2,
                                           (barW - 4) * s_health,
                                           barH - 4, fill);
        }
    }

    // ================= Middle-left: fish counter =================
    const RwReal fishX = barX + barW + 14.0f, fishY = kScreenH - 58.0f;
    if (RenderPlatform_HasTexture("GUI_031") == TRUE) {
        RenderPlatform_DrawTexturedQuad(fishX, fishY, 38.0f, 38.0f,
                                        "GUI_031");
    } else {
        RenderPlatform_DrawOverlayQuad(fishX, fishY, 38.0f, 38.0f,
                                       0xFF40C060u);
    }
    char fishText[16];
    std::snprintf(fishText, sizeof(fishText), "%d", s_fish);
    DrawScaledText(fishText, fishX + 46.0f, fishY + 10.0f, 3, 0xFFB8FF50u);

    // ================= Bottom-right: compass (north-up) =================
    const RwReal compSize = 136.0f;
    const RwReal compX = kScreenW - compSize - 18.0f;
    const RwReal compY = kScreenH - compSize - 34.0f; // 550
    const RwReal compCX = compX + compSize * 0.5f;
    const RwReal compCY = compY + compSize * 0.5f;
    if (RenderPlatform_HasTexture("GUI_020") == TRUE) {
        RenderPlatform_DrawTexturedQuad(compX, compY, compSize, compSize,
                                        "GUI_020");
    } else {
        RenderPlatform_DrawOverlayQuad(compX, compY, compSize, compSize,
                                       0xFF202830u);
    }
    // Tower blips (world direction, north-up): red dots
    for (int i = 0; i < 2; ++i) {
        if (!s_blips[i].alive) {
            continue;
        }
        const RwReal len = std::sqrt(s_blips[i].x * s_blips[i].x +
                                     s_blips[i].z * s_blips[i].z);
        if (len < 1.0f) {
            continue;
        }
        const RwReal nx = s_blips[i].x / len;
        const RwReal nz = s_blips[i].z / len;
        const RwReal bRad = compSize * 0.30f;
        const RwReal bx = compCX + nx * bRad;
        const RwReal by = compCY + nz * bRad; // north(-Z) screen y up
        RenderPlatform_DrawOverlayQuad(bx - 4.0f, by - 4.0f, 8.0f, 8.0f,
                                       0xFFFF3030u);
    }
    // Needle: rotates to face the ship's bow
    DrawNeedle(compCX, compCY, s_shipYaw, compSize * 0.30f, 0xFFF0F0F0u);
    // Ship center point
    RenderPlatform_DrawOverlayQuad(compCX - 2.0f, compCY - 2.0f, 4.0f,
                                   4.0f, 0xFFFFFFFFu);

    // TOWERS indicator (below the compass, gold)
    char towerText[32];
    std::snprintf(towerText, sizeof(towerText), "TOWERS %d/%d",
                  s_towersDestroyed, kTowersTotal);
    DrawAnchoredText(towerText, compCX, compY + compSize + 4.0f, 1,
                     0xFFFFD040u);

    RenderPlatform_EndOverlay2D();
}

} // namespace RWS
