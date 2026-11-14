// ============================================================================
// CannonCruise - Ship/asset inventory scanner (shipscan_test.cpp)
//
// Discovery test: writes the 0x071C manifest class list inside Streams/*.RWS
// and the 0x0704 objectRecord behaviours (className + name) to stdout.
// Looks up relevant classes (e.g. CShip/CShipPlayer/CShipEnemy) and GUIDs.
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
        std::printf("  (could not be opened)\n");
        return;
    }
    // manifest class list
    const RwUInt32 classCount = RwStudioFileGetClassCount(pFile);
    if (classCount > 0) {
        std::printf("manifest (%u classes):\n", (unsigned)classCount);
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
                std::printf("[%u] OBJECTRECORD (shape could not be read)\n",
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
