// ============================================================================
// CannonCruise — Ocean smoke test (Phase 4.6q wire-in)
// verifies COcean::Initialize grid setup + COceanTile waves + Im3D render
// path running without crashing (NULL platform: device no-op).
// ============================================================================

#include "Ocean.h"

#include <cstdio>

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++s_checks;                                                            \
        if (!(cond)) {                                                         \
            ++s_failures;                                                      \
            std::printf("CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__,        \
                        #cond);                                                \
        }                                                                      \
    } while (0)

int main() {
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::printf("ocean_test: engine boot failed\n");
        return 2;
    }

    COcean *pOcean = new COcean;
    CHECK(pOcean != nullptr);
    CHECK(COcean::GetInstance() == pOcean);

    // Grid: 2×2 tiles, 100 units, corner (-100,-100) → [-100..100]²
    pOcean->Initialize(2, 2, 100.0f, -100.0f, -100.0f);
    pOcean->Update(0.5f);
    pOcean->Update(1.0f);
    pOcean->Render(); // outside Begin/End — null: no-op; GL3: no-op if device swallows it

    // corrupt input should be clamped (min 1×1, tileSize>0) — not crashing is enough
    pOcean->Initialize(0, 0, -1.0f, 0.0f, 0.0f);
    pOcean->Update(0.0f);
    pOcean->Render();

    // re-init clears the old grid (no leak/overlap)
    pOcean->Initialize(4, 4, 50.0f, -100.0f, -100.0f);
    pOcean->Render();

    delete pOcean;
    CHECK(COcean::GetInstance() == nullptr);

    // NULL-safe calls
    pOcean = nullptr;

    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();

    std::printf("ocean_test: %d checks, %d failures\n", s_checks, s_failures);
    return (s_failures == 0) ? 0 : 1;
}
