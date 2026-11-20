// ============================================================================
// CannonCruise - GLOBAL.RWS asset inventory (globalassets_test.cpp)
//
// Phase 4.6r-5 / GLOBAL load: all top-level chunks + embedded payloads
// listed. Embedded TexDictionaries (embeddedType=0x23) are verified by name
// (loading screen art). The inventory list is written to stdout
// (inspectable from the ctest output) — regression/discovery test for asset discovery.
//
//   usage: globalassets_test <GLOBAL.RWS>
// ============================================================================

#include "RwStudioStream.h"

#include "rwcore.h"

#include <cstdio>
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

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <GLOBAL.RWS>\n", argv[0]);
        return 2;
    }
    RwStudioFile *pFile = nullptr;
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    pFile = RwStudioFileOpen(argv[1]);
    CHECK(pFile != nullptr);
    if (pFile == nullptr) {
        std::printf("globalassets_test: %s could not be opened\n", argv[1]);
        return 2;
    }
    RwStudioAsset asset;
    RwUInt32 index = 0;
    RwUInt32 embeddedCount = 0;
    RwUInt32 resourceCount = 0;
    RwUInt32 txdCount = 0;
    RwUInt32 imageLibIdCount = 0;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        const RwBool hasEmbedded = (asset.embeddedType != 0);
        const RwBool hasResource = !hasEmbedded && asset.resourceLength > 0;
        const unsigned long long emb = hasEmbedded ? asset.embeddedLength : 0ull;
        std::printf("asset[%u] chunkType=0x%08x '%s' rwId='%s' src='%s' "
                    "embedded=0x%04x/%llu libId=0x%08x resource=%u\n",
                    (unsigned)index, (unsigned)asset.chunkType,
                    asset.typeName, asset.rwIdName, asset.sourcePath,
                    (unsigned)asset.embeddedType, emb,
                    (unsigned)asset.embeddedLibId,
                    (unsigned)asset.resourceLength);
        if (hasEmbedded) {
            ++embeddedCount;
            if (asset.embeddedType == 0x23 && asset.embeddedLibId == 0x1803FFFF) {
                ++txdCount;
            }
            if (asset.embeddedLibId == 0x1803FFFF) {
                ++imageLibIdCount;
            }
        } else if (hasResource) {
            ++resourceCount;
        }
        ++index;
    }
    RwStudioFileClose(pFile);
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    std::printf("globalassets_test: %u chunk, %u embedded (%u txd), "
                "%u external resource, %u libId=0x1803FFFF\n",
                (unsigned)index, (unsigned)embeddedCount, (unsigned)txdCount,
                (unsigned)resourceCount, (unsigned)imageLibIdCount);
    // GLOBAL.RWS contains at least one embedded txd (loading screen art).
    CHECK(index > 0);
    CHECK(txdCount >= 1);
    std::printf("globalassets_test: %d checks, %d failures\n", g_checks,
                g_failures);
    return (g_failures == 0) ? 0 : 1;
}
