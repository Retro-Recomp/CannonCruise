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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

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

static RwInt32 AddNonNegativeSaturated(RwInt32 value, RwInt32 amount) {
    const RwInt32 maxValue = (std::numeric_limits<RwInt32>::max)();
    if (amount <= 0) {
        return value;
    }
    return value > maxValue - amount ? maxValue : value + amount;
}

static RwInt32 AddScoreDelta(RwInt32 score, RwInt32 amount) {
    const long long result = (long long)score + (long long)amount;
    if (result < 0) {
        return 0;
    }
    const RwInt32 maxValue = (std::numeric_limits<RwInt32>::max)();
    return result > maxValue ? maxValue : (RwInt32)result;
}

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
    if (amount <= 0) {
        return;
    }
    s_ammo = amount >= s_ammo ? 0 : s_ammo - amount;
}

RwInt32 HUD_GetAmmo(void) { return s_ammo; }

void HUD_AddFish(RwInt32 amount) {
    s_fish = AddNonNegativeSaturated(s_fish, amount);
}

RwInt32 HUD_GetFish(void) { return s_fish; }

void HUD_SetHealth(RwReal ratio) {
    if (!std::isfinite(ratio)) {
        return;
    }
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    s_health = ratio;
}

RwReal HUD_GetHealth(void) { return s_health; }

void HUD_RegisterTowerDestroyed(void) {
    if (s_towersDestroyed >= kTowersTotal) {
        return;
    }
    ++s_towersDestroyed;
    s_score = AddScoreDelta(s_score, 500);
}

void HUD_AddScore(RwInt32 amount) {
    s_score = AddScoreDelta(s_score, amount);
}

RwInt32 HUD_GetScore(void) { return s_score; }

RwInt32 HUD_GetTowersDestroyed(void) { return s_towersDestroyed; }

void HUD_SetShipHeading(RwReal yaw) {
    if (std::isfinite(yaw)) {
        s_shipYaw = std::remainder(yaw, 6.2831853071795864769f);
    }
}

void HUD_SetTowerBlip(RwInt32 index, RwReal bearingX, RwReal bearingZ,
                      RwBool alive) {
    if (index < 0 || index >= 2 || !std::isfinite(bearingX) ||
        !std::isfinite(bearingZ)) {
        return;
    }
    s_blips[index].x = bearingX;
    s_blips[index].z = bearingZ;
    s_blips[index].alive = alive == TRUE ? TRUE : FALSE;
}

void HUD_FlashNoAmmo(void) { s_noAmmoFlash = 45; } // ~0.75 sec @60fps

// Needle: white, rotates relative to the ship's heading on the north-up compass (im2d triangle).
static void DrawNeedle(RwReal cx, RwReal cy, RwReal relAngleRad,
                       RwReal radius, RwUInt32 argb) {
    const RwReal fx = std::sin(relAngleRad);
    const RwReal fy = std::cos(relAngleRad);
    const RwReal px = std::cos(relAngleRad);  // perpendicular (to the right)
    const RwReal py = -std::sin(relAngleRad);
    const RwReal tipX = cx + fx * radius;
    const RwReal tipY = cy + fy * radius;
    const RwReal baseLX = cx - fx * radius * 0.45f + px * radius * 0.30f;
    const RwReal baseLY = cy - fy * radius * 0.45f + py * radius * 0.30f;
    const RwReal baseRX = cx - fx * radius * 0.45f - px * radius * 0.30f;
    const RwReal baseRY = cy - fy * radius * 0.45f - py * radius * 0.30f;
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

struct HudLayout {
    RwReal scale;
    RwReal offsetX;
    RwReal offsetY;

    RwReal MapX(RwReal x) const { return offsetX + x * scale; }
    RwReal MapY(RwReal y) const { return offsetY + y * scale; }
    RwReal MapSize(RwReal size) const { return size * scale; }
    RwInt32 TextScale(RwInt32 baseScale) const {
        return (RwInt32)std::max(1.0f, std::floor(baseScale * scale + 0.5f));
    }
};

static void DrawHudOverlayQuad(const HudLayout &layout, RwReal x, RwReal y,
                               RwReal w, RwReal h, RwUInt32 argb) {
    RenderPlatform_DrawOverlayQuad(layout.MapX(x), layout.MapY(y),
                                   layout.MapSize(w), layout.MapSize(h), argb);
}

static void DrawHudTexturedQuad(const HudLayout &layout, RwReal x, RwReal y,
                                RwReal w, RwReal h, const char *texName) {
    RenderPlatform_DrawTexturedQuad(layout.MapX(x), layout.MapY(y),
                                    layout.MapSize(w), layout.MapSize(h),
                                    texName);
}

static void DrawScaledText(const char *s, RwReal x, RwReal y,
                           RwInt32 baseScale, RwUInt32 argb,
                           const HudLayout &layout) {
    ShowText_DrawBitmapText(s, layout.MapX(x), layout.MapY(y),
                            layout.TextScale(baseScale),
                            (RwUInt8)((argb >> 16) & 0xFF),
                            (RwUInt8)((argb >> 8) & 0xFF),
                            (RwUInt8)(argb & 0xFF),
                            (RwUInt8)((argb >> 24) & 0xFF));
}

static void DrawAnchoredText(const char *s, RwReal cx, RwReal y,
                             RwInt32 baseScale, RwUInt32 argb,
                             const HudLayout &layout) {
    const RwInt32 scale = layout.TextScale(baseScale);
    ShowText_DrawBitmapText(
        s, layout.MapX(cx) - TextWidth(s, scale) * 0.5f, layout.MapY(y), scale,
        (RwUInt8)((argb >> 16) & 0xFF), (RwUInt8)((argb >> 8) & 0xFF),
        (RwUInt8)(argb & 0xFF), (RwUInt8)((argb >> 24) & 0xFF));
}

void HUD_Render(void) {
    RwInt32 screenW = 0;
    RwInt32 screenH = 0;
    if (RenderPlatform_GetScreenSize(&screenW, &screenH) != TRUE ||
        screenW <= 0 || screenH <= 0) {
        return;
    }

    const RwReal kDesignW = 1280.0f;
    const RwReal kDesignH = 720.0f;
    const RwReal layoutScale =
        std::min((RwReal)screenW / kDesignW, (RwReal)screenH / kDesignH);
    const HudLayout layout = {
        layoutScale,
        ((RwReal)screenW - kDesignW * layoutScale) * 0.5f,
        ((RwReal)screenH - kDesignH * layoutScale) * 0.5f};

    RenderPlatform_BeginOverlay2D();

    char scoreText[32];
    std::snprintf(scoreText, sizeof(scoreText), "SCORE %06d", s_score);
    const RwInt32 scoreScale = layout.TextScale(2);
    const RwReal scorePanelW =
        (TextWidth(scoreText, scoreScale) + 16.0f) / layoutScale;
    const RwReal scorePanelH = (7.0f * scoreScale + 8.0f) / layoutScale;
    const RwReal scoreTextH = 7.0f * scoreScale / layoutScale;
    const RwReal scorePanelY = 12.0f;
    DrawHudOverlayQuad(layout, (kDesignW - scorePanelW) * 0.5f,
                       scorePanelY, scorePanelW, scorePanelH, 0xA0101010u);
    DrawAnchoredText(scoreText, kDesignW * 0.5f,
                     scorePanelY + (scorePanelH - scoreTextH) * 0.5f, 2,
                     0xFFFFD66Au, layout);

    // ================= Bottom-left: ammo dial =================
    const RwReal dialX = 20.0f, dialSize = 128.0f;
    const RwReal dialY = kDesignH - 168.0f;
    const RwReal dialCX = dialX + dialSize * 0.5f;

    if (RenderPlatform_HasTexture("GUI_022") == TRUE) {
        DrawHudTexturedQuad(layout, dialX, dialY, dialSize, dialSize,
                            "GUI_022");
    } else {
        DrawHudOverlayQuad(layout, dialX, dialY, dialSize, dialSize,
                           0xFF8B6914u);
    }
    const RwReal globeSize = 74.0f;
    const RwReal globeX = dialX + (dialSize - globeSize) * 0.5f;
    const RwReal globeY = dialY + 16.0f;
    if (RenderPlatform_HasTexture("GUI_012") == TRUE) {
        DrawHudTexturedQuad(layout, globeX, globeY, globeSize, globeSize,
                            "GUI_012");
    } else {
        DrawHudOverlayQuad(layout, dialX + 20.0f, dialY + 20.0f,
                           dialSize - 40.0f, dialSize - 40.0f,
                           0xFF1A1A1Au);
    }

    // Ammo count (above the globe)
    char ammoText[16];
    std::snprintf(ammoText, sizeof(ammoText), "%d", s_ammo);
    DrawAnchoredText(ammoText, dialCX, dialY + 44.0f, 3, 0xFFFFE666u,
                     layout);

    // "NO AMMO" flash
    if (s_noAmmoFlash > 0) {
        if ((s_noAmmoFlash / 6) % 2 == 0) {
            DrawAnchoredText("NO AMMO", dialCX, dialY - 22.0f, 2,
                             0xFFFF4040u, layout);
        }
        --s_noAmmoFlash;
    }

    // ================= Bottom: health bar (wooden end caps + red) =================
    const RwReal barX = 16.0f, barY = kDesignH - 40.0f;
    const RwReal barH = 22.0f, capW = 26.0f, barW = 216.0f;
    const RwReal midX = barX + capW, midW = barW - capW * 2.0f;
    if (RenderPlatform_HasTexture("GUI_024") == TRUE) {
        DrawHudTexturedQuad(layout, midX, barY, midW, barH, "GUI_024");
    } else {
        DrawHudOverlayQuad(layout, barX, barY, barW, barH, 0xFF101010u);
        DrawHudOverlayQuad(layout, barX + 2.0f, barY + 2.0f, barW - 4.0f,
                           barH - 4.0f, 0xFF302018u);
    }
    const RwReal innerX = midX + 6.0f, innerW = midW - 12.0f;
    const RwReal innerY = barY + 8.0f, innerH = 6.0f;
    DrawHudOverlayQuad(layout, innerX, innerY, innerW, innerH, 0xFF302018u);
    if (s_health > 0.0f) {
        const RwUInt8 rr = (RwUInt8)(140 + 100 * s_health);
        const RwUInt8 gg = (RwUInt8)(30 * s_health);
        const RwUInt8 bb = (RwUInt8)(20 * s_health);
        const RwUInt32 fill =
            0xFF000000u | ((RwUInt32)rr << 16) | ((RwUInt32)gg << 8) | bb;
        DrawHudOverlayQuad(layout, innerX, innerY, innerW * s_health,
                           innerH, fill);
    }
    if (RenderPlatform_HasTexture("GUI_026") == TRUE) {
        DrawHudTexturedQuad(layout, barX, barY, capW, barH, "GUI_026");
    } else {
        DrawHudOverlayQuad(layout, barX, barY, capW, barH, 0xFF604020u);
    }
    if (RenderPlatform_HasTexture("GUI_028") == TRUE) {
        DrawHudTexturedQuad(layout, barX + barW - capW, barY, capW, barH,
                            "GUI_028");
    } else {
        DrawHudOverlayQuad(layout, barX + barW - capW, barY, capW, barH,
                           0xFF604020u);
    }

    // ================= Middle-left: fish counter =================
    const RwReal fishX = barX + barW + 14.0f, fishY = kDesignH - 58.0f;
    if (RenderPlatform_HasTexture("GUI_031") == TRUE) {
        DrawHudTexturedQuad(layout, fishX, fishY, 38.0f, 38.0f, "GUI_031");
    } else {
        DrawHudOverlayQuad(layout, fishX, fishY, 38.0f, 38.0f, 0xFF40C060u);
    }
    char fishText[16];
    std::snprintf(fishText, sizeof(fishText), "%d", s_fish);
    DrawScaledText(fishText, fishX + 46.0f, fishY + 10.0f, 3, 0xFFB8FF50u,
                   layout);

    // ================= Bottom-right: compass (north-up) =================
    const RwReal compSize = 136.0f;
    const RwReal compX = kDesignW - compSize - 18.0f;
    const RwReal compY = kDesignH - compSize - 34.0f;
    const RwReal compCX = compX + compSize * 0.5f;
    const RwReal compCY = compY + compSize * 0.5f;
    if (RenderPlatform_HasTexture("GUI_020") == TRUE) {
        DrawHudTexturedQuad(layout, compX, compY, compSize, compSize,
                            "GUI_020");
    } else {
        DrawHudOverlayQuad(layout, compX, compY, compSize, compSize,
                           0xFF202830u);
    }
    // Tower blips (world direction, north-up): red dots
    for (int i = 0; i < 2; ++i) {
        if (!s_blips[i].alive) {
            continue;
        }
        const RwReal len = std::hypot(s_blips[i].x, s_blips[i].z);
        if (!std::isfinite(len) || len < 1.0f) {
            continue;
        }
        const RwReal nx = s_blips[i].x / len;
        const RwReal nz = s_blips[i].z / len;
        const RwReal bRad = compSize * 0.30f;
        const RwReal bx = compCX + nx * bRad;
        const RwReal by = compCY + nz * bRad;
        DrawHudOverlayQuad(layout, bx - 4.0f, by - 4.0f, 8.0f, 8.0f,
                           0xFFFF3030u);
    }
    // Needle: rotates to face the ship's bow
    DrawNeedle(layout.MapX(compCX), layout.MapY(compCY), s_shipYaw,
               layout.MapSize(compSize * 0.30f), 0xFFF0F0F0u);
    // Ship center point
    DrawHudOverlayQuad(layout, compCX - 2.0f, compCY - 2.0f, 4.0f, 4.0f,
                       0xFFFFFFFFu);

    // TOWERS indicator (below the compass, gold)
    char towerText[32];
    std::snprintf(towerText, sizeof(towerText), "TOWERS %d/%d",
                  s_towersDestroyed, kTowersTotal);
    DrawAnchoredText(towerText, compCX, compY + compSize + 4.0f, 2,
                     0xFFFFD040u, layout);

    RenderPlatform_EndOverlay2D();
}

} // namespace RWS
