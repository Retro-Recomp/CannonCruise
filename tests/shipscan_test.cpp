// ============================================================================
// CannonCruise - Ship/asset envanteri tarayıcısı (shipscan_test.cpp)
//
// Keşif testi: Streams/*.RWS içindeki 0x071C manifest sınıf listesini ve
// 0x0704 objectRecord behaviour'larını (className + isim) stdout'a yazar.
// İlgili sınıflar (örn. CShip/CShipPlayer/CShipEnemy) ve GUID'ler aranır.
//
//   usage: shipscan_test <file.RWS> [<file2.RWS>] ...
// ============================================================================

#include "RwStudioStream.h"

#include "rwcore.h"

#include <cstdio>
#include <cstring>

static void DumpFile(const char *pPath) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("\n=== %s ===\n", pPath);
    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    if (pFile == nullptr) {
        std::printf("  (açılamadı)\n");
        return;
    }
    // manifest sınıf listesi
    const RwUInt32 classCount = RwStudioFileGetClassCount(pFile);
    if (classCount > 0) {
        std::printf("manifest (%u sınıf):\n", (unsigned)classCount);
        for (RwUInt32 i = 0; i < classCount; ++i) {
            std::printf("  [%u] %s\n", (unsigned)i,
                        RwStudioFileGetClassName(pFile, i));
        }
    }
    RwStudioAsset asset;
    RwUInt32 idx = 0;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        if (asset.chunkType == rwstudioID_OBJECTRECORD) {
            const RwStudioObject *obj = RwStudioFileGetObject(pFile);
            if (obj == nullptr) {
                std::printf("[%u] OBJECTRECORD (sekil okunamadi)\n",
                            (unsigned)idx);
                continue;
            }
            std::printf("[%u] objectRecords behaviours=%u\n", (unsigned)idx,
                        (unsigned)obj->behaviourCount);
            for (RwUInt32 b = 0; b < obj->behaviourCount &&
                                 b < rwstudioMAX_BEHAVIOURS; ++b) {
                const RwStudioBehaviour &bh = obj->behaviours[b];
                std::printf("    behaviour[%u] class='%s' name='%s' props=%u\n",
                            (unsigned)b, bh.className, bh.name,
                            (unsigned)bh.propCount);
            }
        } else if (asset.chunkType == rwstudioID_ASSETRECORD) {
            const bool hasEmb = asset.embeddedType != 0;
            const bool hasRes = !hasEmb && asset.resourceLength > 0;
            if (hasEmb) {
                std::printf("[%u] ASSETRECORD '%s' rwID='%s' src='%s' emb=0x%x/%u\n",
                            (unsigned)idx, asset.typeName, asset.rwIdName,
                            asset.sourcePath, (unsigned)asset.embeddedType,
                            (unsigned)asset.embeddedLength);
            } else if (hasRes) {
                std::printf("[%u] ASSETRECORD '%s' rwID='%s' src='%s' resource=%u\n",
                            (unsigned)idx, asset.typeName, asset.rwIdName,
                            asset.sourcePath,
                            (unsigned)asset.resourceLength);
            }
        }
        ++idx;
    }
    RwStudioFileClose(pFile);
}

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 2) {
        std::fprintf(stderr, "usage: shipscan_test <file.RWS> [more .RWS]\n");
        return 2;
    }
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        DumpFile(argv[i]);
    }
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    return 0;
}
