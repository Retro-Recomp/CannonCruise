// ============================================================================
// CannonCruise - Embedded TXD (d3d8 IMAGE) decode test
// The real MAINMENU.RWS embedded txd (16 textures, Menu_137..Menu_074)
// is decoded and the pixel/name/format fields are verified.
// Usage: txddecode_test <MAINMENU.RWS>
// ============================================================================

#include "TxdDecode.h"

#include "RwStudioStream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
        std::fprintf(stderr, "usage: %s <MAINMENU.RWS>\n", argv[0]);
        return 2;
    }

    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }

    {
        RwStudioFile *pFile = RwStudioFileOpen(argv[1]);
        CHECK(pFile != 0);
        if (pFile) {
            RwStudioAsset asset;
            while (RwStudioFileNextAsset(pFile, &asset)) {
                if (asset.embeddedType != 0x23) {
                    continue;
                }
                CHECK(asset.embeddedLibId == 0x1803FFFF);
                // payload buffer (1.68MB)
                RwUInt8 *pBuf =
                    static_cast<RwUInt8 *>(std::malloc(asset.embeddedLength));
                CHECK(pBuf != 0);
                if (!pBuf) {
                    break;
                }
                RwUInt32 got = RwStudioAssetReadEmbedded(
                    pFile, &asset, pBuf, asset.embeddedLength);
                CHECK(got == asset.embeddedLength);

                RwTxdImage *pImage =
                    static_cast<RwTxdImage *>(std::malloc(sizeof(RwTxdImage)));
                CHECK(pImage != 0);
                if (pImage) {
                    CHECK(RwTxdDecodeImage(pBuf, got, pImage) == TRUE);
                    CHECK(pImage->textureCount == 16);
                    CHECK(pImage->deviceId == 1);
                    CHECK(pImage->count == 16);

                    // first texture: Menu_137, 128×32×32bpp, pitch 512
                    const RwTxdTexture *pT0 = &pImage->textures[0];
                    CHECK(std::strcmp(pT0->name, "Menu_137") == 0);
                    CHECK(pT0->width == 128);
                    CHECK(pT0->height == 32);
                    CHECK(pT0->depth == 32);
                    CHECK(pT0->pitch == 512);
                    CHECK(pT0->pixelBytes == 16384);
                    CHECK(pT0->filterFlags == 0x00011101);
                    CHECK(pT0->pPixels != 0);
                    // first pixel = 0x00FFFFFF (white)
                    CHECK(pT0->pPixels[0] == 0xFF && pT0->pPixels[1] == 0xFF &&
                          pT0->pPixels[2] == 0xFF && pT0->pPixels[3] == 0x00);

                    // last texture: Menu_074 (640×480 background)
                    const RwTxdTexture *pTL =
                        &pImage->textures[pImage->count - 1];
                    CHECK(std::strcmp(pTL->name, "Menu_074") == 0);
                    CHECK(pTL->width == 640);
                    CHECK(pTL->height == 480);
                    CHECK(pTL->pitch == 2560);
                    CHECK(pTL->pixelBytes == 1228800);
                    CHECK(pTL->pPixels != 0);

                    // size variety: tex14 = 512×128 strip
                    CHECK(pImage->textures[14].width == 512);
                    CHECK(pImage->textures[14].height == 128);
                    CHECK(pImage->textures[14].pixelBytes == 262144);

                    // all names prefixed with "Menu_", pixels are inside the payload
                    RwBool allMenu = TRUE;
                    RwBool pixelsInRange = TRUE;
                    RwBool sizeConsistent = TRUE;
                    for (RwUInt32 i = 0; i < pImage->count; ++i) {
                        if (std::strncmp(pImage->textures[i].name, "Menu_", 5) !=
                            0) {
                            allMenu = FALSE;
                        }
                        const RwUInt8 *pPix = pImage->textures[i].pPixels;
                        if (!pPix || pPix < pBuf || pPix >= pBuf + got) {
                            pixelsInRange = FALSE;
                        }
                        if (pImage->textures[i].pixelBytes !=
                            pImage->textures[i].pitch *
                                pImage->textures[i].height) {
                            sizeConsistent = FALSE;
                        }
                    }
                    CHECK(allMenu == TRUE);
                    CHECK(pixelsInRange == TRUE);
                    CHECK(sizeConsistent == TRUE);
                    std::free(pImage);
                }
                std::free(pBuf);
                break;
            }
            RwStudioFileClose(pFile);
        }
    }

    // invalid input → FALSE
    CHECK(RwTxdDecodeImage(0, 0, 0) == FALSE);
    const RwUInt8 junk[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    RwTxdImage tmpImage;
    CHECK(RwTxdDecodeImage(junk, sizeof(junk), &tmpImage) == FALSE);

    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();

    std::printf("txddecode_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return s_failures == 0 ? 0 : 1;
}
