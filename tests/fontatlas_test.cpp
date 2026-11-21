// ============================================================================
// CannonCruise - font atlas dictionary test
// Localiza\Win\win_<lang>.txd is a platform-independent texture dictionary
// (0x23) holding one raw rwID_IMAGE; RwTexDictionaryReadImageFile must turn it
// into a dictionary with the 512x512 "brady48" texture.
// Usage: fontatlas_test <win_eng.txd>
// ============================================================================

#include "rwcore.h"

#include <rw.h>

#include <cstdio>

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++s_checks;                                                            \
        if (!(cond)) {                                                         \
            ++s_failures;                                                      \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                \
        }                                                                      \
    } while (0)

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <win_eng.txd>\n", argv[0]);
        return 2;
    }
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    CHECK(RwTexDictionaryReadImageFile(0) == 0);
    CHECK(RwTexDictionaryReadImageFile("no_such_file.txd") == 0);

    RwTexDictionary *pDict = RwTexDictionaryReadImageFile(argv[1]);
    CHECK(pDict != 0);
    if (pDict) {
        rw::TexDictionary *pTxd = reinterpret_cast<rw::TexDictionary *>(pDict);
        CHECK(pTxd->count() == 1);
        rw::Texture *pTex = pTxd->find("brady48");
        CHECK(pTex != 0);
        if (pTex && pTex->raster) {
            CHECK(pTex->raster->width == 512 && pTex->raster->height == 512);
        }
    }
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    std::printf("fontatlas_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return s_failures == 0 ? 0 : 1;
}
