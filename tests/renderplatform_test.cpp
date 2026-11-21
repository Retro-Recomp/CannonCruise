// ============================================================================
// CannonCruise - Render Platform Smoke Test (renderplatform_test.cpp)
// Phase 4.6n: engine boot → setup (idempotent) → 3 frames (clear/Begin/
// Im2D quad/End) → terminate → re-setup → teardown. NULL platform
// headless (device no-op — verifies the frame sequence API hookup).
// ============================================================================

#include "RenderPlatform.h"
#include "rwcore.h"

#include <cstdio>

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++s_checks;                                                        \
        if (!(cond)) {                                                     \
            ++s_failures;                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                  \
    } while (0)

int main() {
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }

    // Setup
    CHECK(RenderPlatform_Initialize(640, 480) == TRUE);
    CHECK(RenderPlatform_IsInitialized() == TRUE);
    CHECK(RenderPlatform_GetWorld() != nullptr);
    CHECK(RenderPlatform_GetCamera() != nullptr);

    // Idempotent
    CHECK(RenderPlatform_Initialize(640, 480) == TRUE);

    // 3 frames: Begin → TestQuad → End
    for (int frame = 0; frame < 3; ++frame) {
        CHECK(RenderPlatform_BeginFrame(nullptr) == TRUE);
        CHECK(RenderPlatform_RenderTestQuad(0.0f, 0.0f, 0.25f, 0.25f,
                                            0xFF40A0FF) == TRUE);
        RenderPlatform_EndFrame();
    }

    // BeginFrame with clear color
    static RwRGBA kClear = {16, 32, 48, 255};
    CHECK(RenderPlatform_BeginFrame(&kClear) == TRUE);
    RenderPlatform_EndFrame();

    // Terminate → clean state
    RenderPlatform_Terminate();
    CHECK(RenderPlatform_IsInitialized() == FALSE);
    CHECK(RenderPlatform_GetWorld() == nullptr);
    CHECK(RenderPlatform_GetCamera() == nullptr);

    // When not set up, frame/quad → FALSE
    CHECK(RenderPlatform_BeginFrame(nullptr) == FALSE);
    CHECK(RenderPlatform_RenderTestQuad(0.0f, 0.0f, 0.1f, 0.1f,
                                        0xFFFFFFFF) == FALSE);

    // Terminate again (idempotent) + re-setup
    RenderPlatform_Terminate();
    CHECK(RenderPlatform_Initialize(800, 600) == TRUE);
    CHECK(RenderPlatform_BeginFrame(nullptr) == TRUE);
    CHECK(RenderPlatform_RenderTestQuad(0.1f, -0.1f, 0.2f, 0.2f,
                                        0xFFFF8020) == TRUE);
    RenderPlatform_EndFrame();
    RenderPlatform_Terminate();
    CHECK(RenderPlatform_IsInitialized() == FALSE);

    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();

    std::printf("renderplatform_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return (s_failures == 0) ? 0 : 1;
}
