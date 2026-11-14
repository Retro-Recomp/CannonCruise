// ============================================================================
// CannonCruise - gemi .dff librw clump doğrulama (shipprobe_test.cpp)
//
// Keşif sonucu: VIKING01.RWS içindeki rwID_CLUMP payload'ları 16 bayt özel
// başlık + devamında standart RW 3.6 clump chunk'ları (STRUCT/FRAMELIST/
// GEOMETRYLIST + GEOMETRY'ler + ATOMIC'ler). Bu 16 baytlık başlık silinirse
// rw::Clump::streamRead (RpClumpStreamRead) doğrudan çözümleyebilir.
// ============================================================================

#include "rwcore.h"

#include <rw.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char **argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 2) {
        std::fprintf(stderr, "usage: shipprobe_test <ship.dff>\n");
        return 2;
    }
    if (RwEngineInit(0, 0, 0) != TRUE || RwEngineOpen() != TRUE ||
        RwEngineStart() != TRUE) {
        std::fprintf(stderr, "engine init failed\n");
        return 2;
    }
    FILE *f = std::fopen(argv[1], "rb");
    if (!f) {
        std::fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    std::fseek(f, 0, SEEK_END);
    size_t n = (size_t)std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    RwUInt8 *pBuf = (RwUInt8 *)std::malloc(n);
    if (!pBuf || std::fread(pBuf, 1, n, f) != n) {
        std::fprintf(stderr, "read failed\n");
        return 2;
    }
    std::fclose(f);
    rw::StreamMemory mem;
    mem.open(pBuf, (rw::uint32)n);
    rw::Clump *pClump = rw::Clump::streamRead(&mem);
    std::printf("rw::Clump::streamRead('%s') -> %p\n", argv[1], (void *)pClump);
    if (pClump) {
        rw::Frame *pFrame = pClump->getFrame();
        std::printf("clump frame=%p\n", (void *)pFrame);
        FORLIST(lnk, pClump->atomics) {
            rw::Atomic *a = rw::Atomic::fromClump(lnk);
            rw::Geometry *g = a->geometry;
            std::printf("  atomic geo=%p flags=%u verts=%u tris=%u mats=%u\n",
                        (void *)g, g->flags, g->numVertices, g->numTriangles,
                        g->matList.numMaterials);
        }
        pClump->destroy();
    }
    std::free(pBuf);
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();
    return 0;
}
