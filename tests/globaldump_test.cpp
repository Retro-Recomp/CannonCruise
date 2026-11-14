// ============================================================================
// CannonCruise - GLOBAL.RWS gömülü txd → PNG dökümü (globaldump_test.cpp)
//
// Keşif amaçlı: GLOBAL'daki gömülü TexDictionary (yükleme ekranı art'ı)
// çözülür ve her doku <outdir>/tex_<i>.png olarak yazılır. Görsel doğrulama
// için kullanılır; ctest sadece çıktıyı listeler.
//
//   usage: globaldump_test <GLOBAL.RWS> <outdir>
// ============================================================================

#include "TxdDecode.h"
#include "RwStudioStream.h"
#include "rwcore.h"

#include <rw.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <GLOBAL.RWS> <outdir>\n", argv[0]);
        return 2;
    }
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    RwStudioFile *pFile = RwStudioFileOpen(argv[1]);
    if (pFile == nullptr) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    RwStudioAsset asset;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        if (asset.embeddedType != 0x23 || asset.embeddedLibId != 0x1803FFFF) {
            continue;
        }
        RwUInt8 *pBuf = static_cast<RwUInt8 *>(
            std::malloc(asset.embeddedLength));
        if (pBuf == nullptr) {
            continue;
        }
        const RwUInt32 got = RwStudioAssetReadEmbedded(
            pFile, &asset, pBuf, asset.embeddedLength);
        if (got == asset.embeddedLength) {
            RwTxdImage *pImage = static_cast<RwTxdImage *>(
                std::malloc(sizeof(RwTxdImage)));
            if (pImage != nullptr &&
                RwTxdDecodeImage(pBuf, got, pImage) == TRUE) {
                for (RwUInt32 i = 0; i < pImage->count; ++i) {
                    const RwTxdTexture *t = &pImage->textures[i];
                    rw::Image *im = rw::Image::create((int)t->width,
                                                      (int)t->height, 32);
                    if (im == nullptr) {
                        continue;
                    }
                    im->allocate();
                    if (im->pixels == nullptr) {
                        im->destroy();
                        continue;
                    }
                    for (RwUInt32 y = 0; y < t->height; ++y) {
                        const RwUInt8 *src =
                            t->pPixels + (size_t)y * t->pitch;
                        RwUInt8 *dst =
                            im->pixels + (size_t)y * im->stride;
                        std::memcpy(dst, src, (size_t)t->width * 4);
                    }
                    std::string out = std::string(argv[2]) + "/tex_" +
                                      std::to_string(i) + "_" + t->name;
                    const size_t n = out.size();
                    if (out.size() < 4 ||
                        out.compare(n - 4, 4, ".png") != 0) {
                        out += ".png";
                    }
                    rw::writePNG(im, out.c_str());
                    im->destroy();
                    std::printf("dumped %s\n", out.c_str());
                }
            }
            std::free(pImage);
        }
        std::free(pBuf);
    }
    RwStudioFileClose(pFile);
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    return 0;
}
