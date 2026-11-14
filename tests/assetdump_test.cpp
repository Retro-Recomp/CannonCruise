// ============================================================================
// CannonCruise - .RWS asset payload dökücü (assetdump_test.cpp)
//
// Keşif aracı: Streams/*.RWS içindeki rwID_CLUMP / wbID_WORLDGEOMETRY vs.
// payload'unu <outfile>'a yazar (kolayca librw clump reader ile çözülebilir).
//
//   usage: assetdump_test <file.RWS> <assetIndex> <outfile>
// ============================================================================

#include "RwStudioStream.h"

#include "rwcore.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 4) {
        std::fprintf(stderr, "usage: assetdump_test <file.RWS> <assetIndex> <outfile>\n");
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
    const RwUInt32 want = (RwUInt32)std::atoi(argv[2]);
    RwStudioAsset asset;
    RwUInt32 idx = 0;
    RwBool found = FALSE;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        if (idx++ == want) {
            found = TRUE;
            break;
        }
    }
    if (found == FALSE) {
        std::fprintf(stderr, "asset [%u] bulunamadi (toplam %u)\n", want, idx);
        RwStudioFileClose(pFile);
        return 2;
    }
    const RwUInt32 len = (asset.embeddedType != 0) ? asset.embeddedLength
                                                   : asset.resourceLength;
    if (len == 0) {
        std::fprintf(stderr, "asset [%u] payload yok\n", want);
        RwStudioFileClose(pFile);
        return 2;
    }
    RwUInt8 *pBuf = static_cast<RwUInt8 *>(std::malloc(len));
    if (pBuf == nullptr) {
        RwStudioFileClose(pFile);
        return 2;
    }
    const RwUInt32 got = (asset.embeddedType != 0)
        ? RwStudioAssetReadEmbedded(pFile, &asset, pBuf, len)
        : RwStudioAssetReadResource(pFile, &asset, pBuf, len);
    if (got != len) {
        std::fprintf(stderr, "okuma basarisiz: %u/%u bayt\n", got, len);
        std::free(pBuf);
        RwStudioFileClose(pFile);
        return 2;
    }
    FILE *f = std::fopen(argv[3], "wb");
    if (f == nullptr) {
        std::fprintf(stderr, "output acilamadi\n");
        std::free(pBuf);
        RwStudioFileClose(pFile);
        return 2;
    }
    std::fwrite(pBuf, 1, got, f);
    std::fclose(f);
    std::printf("asset [%u] '%s' rwID='%s' src='%s' -> %s (%u bytes)\n",
                want, asset.typeName, asset.rwIdName, asset.sourcePath,
                argv[3], got);
    std::free(pBuf);
    RwStudioFileClose(pFile);
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    return 0;
}
