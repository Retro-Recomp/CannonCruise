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

// SCR.mov yükleme ekranı: siyah üst, ufukta dalgalı deniz çizgisi, ortada
// parchment şerit, üzerinde "LOADING..." (canlı noktalar), Hugo kayığı
// denizde soldan sağa ilerler (ilerleme = yüklenme oranı). 30 fps sabit.
void DrawLoadingScene(int frame, int frameBudget, int W, int H) {
    const RwReal progress =
        (frameBudget > 0) ? (RwReal)frame / (RwReal)frameBudget : 1.0f;
    const RwReal horizonY = (RwReal)H * 0.56f; // ufuk çizgisi (deniz üstü)

    RenderPlatform_BeginOverlay2D();

    // Zemin — üst gece siyahı (#040914), ufuk altı koyu lacivert deniz
    // (#0C1E3A). DrawOverlayQuad BGRA bayt sırası: 0xAABBGGRR değil, R=b0 G=b1 B=b2.
    RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, (RwReal)W, horizonY,
                                   0xFF140904);
    RenderPlatform_DrawOverlayQuad(0.0f, horizonY, (RwReal)W,
                                   (RwReal)H - horizonY, 0xFF3A1E0C);

    // Parchment şerit — deniz üzerinde yüzer tabela.
    const RwReal stripW = W * 0.72f;
    const RwReal stripH = stripW * (128.0f / 512.0f); // orijinal en/boy oranı
    const RwReal stripX = ((RwReal)W - stripW) * 0.5f;
    const RwReal stripY = H * 0.545f;                 // deniz üstünde asılı
    if (RenderPlatform_HasTexture("Load_NT_001") == TRUE) {
        RenderPlatform_DrawTexturedQuad(stripX, stripY, stripW, stripH,
                                        "Load_NT_001");
    } else {
        RenderPlatform_DrawOverlayQuad(stripX, stripY, stripW, stripH,
                                       0xFF5C94B8); // parchment fallback (BGRA)
    }

    // Hugo — parchment önünde denizde (dalgaların arkasında) soldan sağa
    // süzülür (ilerleme = yüklenme oranı), hafif yalpalama. Gövdesi dalga
    // bandının altında kalacak şekilde oturur.
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

    // Dalga bordürü — parchment'un üzerine, tekneyle parchment birleşimine
    // oturur (ön planda su); 512x32 doku sağa sürüklenir.
    if (RenderPlatform_HasTexture("Load_NT_005") == TRUE) {
        const RwReal waveH = H * 0.085f;
        const RwReal waveW = 512.0f;
        const RwReal offset = (RwReal)(frame * 1);        // yavaş sürüklenme
        const RwReal waveY = stripY - waveH * 0.42f;      // parchment üst çizgisi
        for (int i = -1; i < 3; ++i) {
            RenderPlatform_DrawTexturedQuad(
                (RwReal)i * waveW - offset, waveY, waveW, waveH,
                "Load_NT_005");
        }
    }

    // "LOADING" + canlı noktalar — parchment'un alt yarısında, koyu mürekkep.
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

    // İpucu — sol alt.
    ShowText_DrawBitmapText("ESC", (RwReal)(W * 0.04f), (RwReal)(H * 0.95f),
                            2, 190, 185, 170, 255);

    RenderPlatform_EndOverlay2D();
}

} // namespace

RwBool LoadingScreen_Play(int frames, int screenW, int screenH) {
    if (frames <= 0) {
        return TRUE;
    }
    // Ses — orijinal: yükleme döngüsü (Loading.wav).
    CAudioManager *pAudio = CAudioManager::GetSingletonPtr();
    if (pAudio != nullptr) {
        pAudio->PlayStream("Loading", TRUE);
    }

    // 30 fps hedefi — kare atlatmadan canlı ritim (uykuda bloklamadan).
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

        // Düğmeye basıldıysa çıkış — yükleme atlandı (oyuna geç).
        if (RwPlatformKeyDown(RW_KEY_ESCAPE) || RwPlatformKeyDown(' ') ||
            RwPlatformKeyDown(RW_KEY_ENTER)) {
            break;
        }

        // 30 fps beklemesi (yoğun kare içinde yumuşak nefes).
        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();
        if (now < nextFrame) {
            std::this_thread::sleep_for(nextFrame - now);
        } else {
            nextFrame = now; // geride kaldıysak bir sonraki kareyi geciktirme
        }
    }

    if (pAudio != nullptr) {
        pAudio->StopAllVoices();
    }
    return quit ? FALSE : TRUE;
}
