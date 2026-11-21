// ============================================================================
// CannonCruise - level texture pack test (RwTxdDecodeLevelPack)
//
// In a level's texture pack each texture's IMAGE records (mip levels,
// largest first) precede the TEXTURE record that carries its name. The
// decoder once paired every name with the image *after* it, i.e. the next
// texture's pixels (cliffs got wood, the ship hull got a crate ...).
// Usage: levelpack_test <VIKING01.RWS>
// ============================================================================

#include "TxdDecode.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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

static const RwTxdTexture *Find(const RwTxdImage *pImg, const char *pName) {
    for (RwUInt32 i = 0; i < pImg->count; ++i) {
        if (std::strcmp(pImg->textures[i].name, pName) == 0) {
            return &pImg->textures[i];
        }
    }
    return 0;
}

// Average R, G, B of the texture's pixels (stored R,G,B,A).
static void Average(const RwTxdTexture *t, int rgb[3]) {
    long long sum[3] = {0, 0, 0};
    for (RwUInt32 y = 0; y < t->height; ++y) {
        const RwUInt8 *row = t->pPixels + (size_t)y * t->pitch;
        for (RwUInt32 x = 0; x < t->width; ++x) {
            for (int k = 0; k < 3; ++k) {
                sum[k] += row[x * 4 + k];
            }
        }
    }
    const long long n = (long long)t->width * t->height;
    for (int k = 0; k < 3; ++k) {
        rgb[k] = (int)(sum[k] / n);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <VIKING01.RWS>\n", argv[0]);
        return 2;
    }
    FILE *f = std::fopen(argv[1], "rb");
    CHECK(f != 0);
    if (!f) {
        return 1;
    }
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<RwUInt8> buf((size_t)size);
    CHECK(std::fread(&buf[0], 1, buf.size(), f) == buf.size());
    std::fclose(f);

    // Same scan window as RenderPlatform_LoadWorldDictionary: chunk 0.
    RwUInt32 c0len = 0;
    std::memcpy(&c0len, &buf[4], 4);
    const RwUInt32 scanLen = 12 + c0len;
    RwTxdImage *pImg = static_cast<RwTxdImage *>(std::malloc(sizeof(RwTxdImage)));
    CHECK(RwTxdDecodeLevelPack(&buf[0], scanLen, pImg) == TRUE);
    std::printf("VIKING01 level pack: %u textures\n", pImg->count);
    CHECK(pImg->count == 55); // every name record gets its image

    const RwTxdTexture *hull = Find(pImg, "hull");          // Hugo_Ship atlas
    const RwTxdTexture *crate = Find(pImg, "Crate_Weapons");
    const RwTxdTexture *pine = Find(pImg, "Tree_Pine");
    const RwTxdTexture *cliff = Find(pImg, "nordicliff");
    CHECK(hull && hull->width == 256 && hull->height == 256);
    CHECK(crate && crate->width == 128 && crate->height == 128);
    CHECK(pine && pine->width == 64 && pine->height == 64);
    CHECK(cliff && cliff->width == 128 && cliff->height == 128);
    if (pine) {
        int c[3];
        Average(pine, c);
        std::printf("Tree_Pine avg rgb=(%d,%d,%d)\n", c[0], c[1], c[2]);
        CHECK(c[1] > c[0] && c[1] > c[2]); // green
    }
    if (cliff) {
        int c[3];
        Average(cliff, c);
        std::printf("nordicliff avg rgb=(%d,%d,%d)\n", c[0], c[1], c[2]);
        // grey rock: channels within a small range of each other
        CHECK(std::abs(c[0] - c[1]) < 25 && std::abs(c[1] - c[2]) < 25);
    }
    std::free(pImg);
    std::printf("levelpack_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return s_failures == 0 ? 0 : 1;
}
