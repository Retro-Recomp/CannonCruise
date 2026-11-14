// ============================================================================
// CannonCruise - Faz 4.6s entegrasyon stres testi (worldcycle_test.cpp)
//
// Tam oyun döngüsünün dünya-yükleme çekirdeğini sürer: dünya yüklenir,
// clump instantiate edilir, doğrulanır, UNLOAD edilir — iki farklı dünya
// üzerinden art arda turlar. Amaç: load/unload zincirinde crash ve
// doğrulama hatası olmaması (play loop → seviye geçişi → unload).
//
//   usage: worldcycle_test <VIKING01.RWS> <IDUNA03.RWS> <BEAVER01.RWS>
//
// Her turda: RwsStudio open → WorldGeometry asset payload → Inspect →
// CreateClump (grup1+grup2) → atomic sayısı → DestroyClump → free.
// ============================================================================

#include "WorldGeometry.h"
#include "RwStudioStream.h"

#include <rw.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int g_checks = 0;
static int g_failures = 0;

static void Check(bool cond, const char *pExpr, int line) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, pExpr);
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

static unsigned char *ReadWorldGeometryPayload(const char *pPath,
                                               RwUInt32 *pLen) {
    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    if (pFile == nullptr) {
        return nullptr;
    }
    RwStudioAsset asset;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        if (std::strcmp(asset.typeName, "WorldGeometry") == 0 &&
            asset.resourceLength > 0) {
            unsigned char *pPayload = static_cast<unsigned char *>(
                std::malloc(asset.resourceLength));
            if (pPayload != nullptr) {
                const RwUInt32 got = RwStudioAssetReadResource(
                    pFile, &asset, pPayload, asset.resourceLength);
                if (got != asset.resourceLength) {
                    std::free(pPayload);
                    pPayload = nullptr;
                } else {
                    *pLen = asset.resourceLength;
                }
            }
            RwStudioFileClose(pFile);
            return pPayload;
        }
    }
    RwStudioFileClose(pFile);
    return nullptr;
}

struct WorldCase {
    const char *pPath;
    int expectedAtomics;
};

static void Cycle(const WorldCase &wc, int iteration) {
    std::printf("--- cycle %d: %s ---\n", iteration, wc.pPath);
    RwUInt32 len = 0;
    unsigned char *pPayload = ReadWorldGeometryPayload(wc.pPath, &len);
    CHECK(pPayload != nullptr);
    if (pPayload == nullptr) {
        return;
    }
    WorldGeometryInfo info;
    std::memset(&info, 0, sizeof(info));
    const RwBool okInspect = WorldGeometry_Inspect(pPayload, len, &info);
    CHECK(okInspect == TRUE);
    void *pClump = WorldGeometry_CreateClump(pPayload, len);
    CHECK(pClump != nullptr);
    if (pClump != nullptr) {
        rw::Clump *pC = static_cast<rw::Clump *>(pClump);
        const int n = (int)pC->countAtomics();
        std::printf("  atomics=%d (beklenen %d)\n", n, wc.expectedAtomics);
        CHECK(n == wc.expectedAtomics);
        WorldGeometry_DestroyClump(pClump);
    }
    std::free(pPayload);
}

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 4) {
        std::fprintf(stderr,
                     "usage: %s <VIKING01.RWS> <IDUNA03.RWS> <BEAVER01.RWS>\n",
                     argv[0]);
        return 2;
    }
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    const WorldCase worlds[3] = {
        {argv[1], 130}, // VIKING01: 96 + 34 (grup2)
        {argv[2], 61},  // IDUNA03:  31 + 30
        {argv[3], 176}, // BEAVER01: 119 + 57
    };
    // Oyun döngüsü: art arda iki seviye geçişi (A→B→C→A→B) — unload
    // zinciri bellek bozulumu olmadan çalışmalı.
    const int order[6] = {0, 1, 2, 0, 1, 2};
    for (int i = 0; i < 6; ++i) {
        Cycle(worlds[order[i]], i + 1);
    }
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    std::printf("worldcycle_test: %d checks, %d failures\n", g_checks,
                g_failures);
    return (g_failures == 0) ? 0 : 1;
}
