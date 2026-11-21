// ============================================================================
// CannonCruise - Loading Screen (LoadingScreen.cpp)
// ============================================================================

#include "LoadingScreen.h"

#include "RenderPlatform.h"
#include "ShowText.h"
#include "AudioManager.h"
#include "rwcore.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace {

// SCR.mov loading screen: black top, wavy sea line at the horizon, a parchment
// strip in the middle with "LOADING..." (animated dots) on it, Hugo's boat
// drifts from left to right across the sea (progress = load ratio). Fixed 30 fps.
void DrawLoadingScene(int frame, int frameBudget, int W, int H) {
    const RwReal progress =
        (frameBudget > 0) ? (RwReal)frame / (RwReal)frameBudget : 1.0f;
    const RwReal horizonY = (RwReal)H * 0.56f; // horizon line (above the sea)

    RenderPlatform_BeginOverlay2D();

    // Background — night black on top (#040914), dark navy sea below the
    // horizon (#0C1E3A). DrawOverlayQuad takes 0xAARRGGBB.
    RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, (RwReal)W, horizonY,
                                   0xFF040914);
    RenderPlatform_DrawOverlayQuad(0.0f, horizonY, (RwReal)W,
                                   (RwReal)H - horizonY, 0xFF0C1E3A);

    // Parchment strip — floating sign above the sea.
    const RwReal stripW = W * 0.72f;
    const RwReal stripH = stripW * (128.0f / 512.0f); // original aspect ratio
    const RwReal stripX = ((RwReal)W - stripW) * 0.5f;
    const RwReal stripY = H * 0.545f;                 // hanging above the sea
    if (RenderPlatform_HasTexture("Load_NT_001") == TRUE) {
        RenderPlatform_DrawTexturedQuad(stripX, stripY, stripW, stripH,
                                        "Load_NT_001");
    } else {
        RenderPlatform_DrawOverlayQuad(stripX, stripY, stripW, stripH,
                                       0xFFB8945C); // parchment fallback
    }

    // Hugo — on the sea in front of the parchment (behind the waves), drifts
    // from left to right (progress = load ratio), gentle rocking. Sits so its
    // hull stays below the wave band.
    const RwReal boatW = H * 0.26f;
    const RwReal boatY = stripY - boatW * 0.02f +
                         std::sin((RwReal)frame * 0.11f) * 2.0f;
    if (RenderPlatform_HasTexture("Load_NT_003") == TRUE) {
        const RwReal startX = -boatW * 0.35f;
        const RwReal endX = (RwReal)W + boatW * 0.1f;
        const RwReal boatX = startX + progress * (endX - startX);
        RenderPlatform_DrawTexturedQuad(boatX, boatY, boatW, boatW,
                                        "Load_NT_003");
    }

    // Wave border — over the parchment, sitting at the junction of the boat and
    // the parchment (water in the foreground); the 512x32 texture scrolls right.
    if (RenderPlatform_HasTexture("Load_NT_005") == TRUE) {
        const RwReal waveH = H * 0.085f;
        const RwReal waveW = 512.0f;
        const RwReal offset = (RwReal)(frame * 1);        // slow drift
        const RwReal waveY = stripY - waveH * 0.42f;      // parchment top edge
        for (int i = -1; i < 3; ++i) {
            RenderPlatform_DrawTexturedQuad(
                (RwReal)i * waveW - offset, waveY, waveW, waveH,
                "Load_NT_005");
        }
    }

    // "LOADING" + live dots — in the lower half of the parchment, dark ink.
    const char *dots = (frame / 18) % 4 == 0   ? ""
                       : (frame / 18) % 4 == 1 ? "."
                       : (frame / 18) % 4 == 2 ? ".."
                                               : "...";
    char label[32];
    std::snprintf(label, sizeof(label), "LOADING%s", dots);
    const RwInt32 textScale = 4;
    const RwInt32 glyphAdv = 6 * textScale;
    const RwInt32 textW = (RwInt32)std::strlen(label) * glyphAdv;
    ShowText_DrawBitmapText(label,
                            (RwReal)(W / 2 - textW / 2),
                            stripY + stripH * 0.62f, textScale, 72, 48, 26,
                            255);

    // Hint — bottom left.
    ShowText_DrawBitmapText("ESC", (RwReal)(W * 0.04f), (RwReal)(H * 0.95f),
                            2, 190, 185, 170, 255);

    RenderPlatform_EndOverlay2D();
}

} // namespace

RwBool LoadingScreen_Play(int frames, int screenW, int screenH) {
    if (frames <= 0) {
        return TRUE;
    }
    // Draw in the real render-target size (the caller's 640x480 only covered
    // the top-left part of the 1280x720 frame).
    RwInt32 rtW = 0, rtH = 0;
    if (RenderPlatform_GetScreenSize(&rtW, &rtH) == TRUE) {
        screenW = rtW;
        screenH = rtH;
    }
    // Sound — original: loading loop (Loading.wav).
    CAudioManager *pAudio = CAudioManager::GetSingletonPtr();
    if (pAudio != nullptr) {
        pAudio->PlayStream("Loading", TRUE);
    }

    // 30 fps target — smooth live rhythm without frame skipping (no blocking sleep).
    const auto kFrameDuration = std::chrono::milliseconds(33); // ~30 fps
    std::chrono::steady_clock::time_point nextFrame =
        std::chrono::steady_clock::now();

    RwBool quit = FALSE;
    for (int frame = 0; frame < frames; ++frame) {
        nextFrame += kFrameDuration;

        RwPlatformPollEvents();
        if (RwPlatformShouldClose() == TRUE) {
            quit = TRUE;
            break;
        }
        if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
            break;
        }
        DrawLoadingScene(frame, frames, screenW, screenH);
        if (std::getenv("CANNONCRUISE_SCREENSHOT_LOADING") != nullptr &&
            frame == 2) {
            RenderPlatform_RequestScreenshot(
                std::getenv("CANNONCRUISE_SCREENSHOT_LOADING"));
        }
        RenderPlatform_EndFrame();
        if (std::getenv("CANNONCRUISE_SCREENSHOT_LOADING") != nullptr &&
            frame == 89 && frames > 90) {
            RenderPlatform_RequestScreenshot(
                std::getenv("CANNONCRUISE_SCREENSHOT_LOADING"));
        }

        // Quit if a button was pressed — loading skipped (go to the game).
        if (RwPlatformKeyDown(RW_KEY_ESCAPE) || RwPlatformKeyDown(' ') ||
            RwPlatformKeyDown(RW_KEY_ENTER)) {
            break;
        }

        // 30 fps wait (gentle pause within the busy frame).
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        if (now < nextFrame) {
            std::this_thread::sleep_for(nextFrame - now);
        } else {
            nextFrame = now; // if we're behind, don't delay the next frame
        }
    }

    if (pAudio != nullptr) {
        pAudio->StopAllVoices();
    }
    return quit ? FALSE : TRUE;
}
