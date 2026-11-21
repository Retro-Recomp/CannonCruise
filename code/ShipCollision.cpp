// ============================================================================
// CannonCruise - Ship collision (ShipCollision.cpp) -- see ShipCollision.h
// ============================================================================

#include "ShipCollision.h"

#include "rpworld.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {

const RwReal kCell = 1.5f;          // grid resolution (m)
const RwReal kLandAboveWater = 0.3f; // lowest surface higher than this = land
const RwReal kNoSurface = 1.0e9f;

struct LandMap {
    RwReal minX, minZ;
    RwInt32 w, h;
    std::vector<RwReal> lowest; // lowest surface height per cell
};

LandMap s_map = {0.0f, 0.0f, 0, 0, std::vector<RwReal>()};
RwReal s_waterY = 0.0f;
std::vector<RwReal> s_obstacles;

struct BoundsCtx {
    RwReal minX, maxX, minZ, maxZ;
    RwBool any;
};

void BoundsCB(const RwReal a[3], const RwReal b[3], const RwReal c[3], void *pCtx) {
    BoundsCtx *p = static_cast<BoundsCtx *>(pCtx);
    const RwReal *v[3] = {a, b, c};
    for (int k = 0; k < 3; ++k) {
        if (p->any != TRUE) {
            p->minX = p->maxX = v[k][0];
            p->minZ = p->maxZ = v[k][2];
            p->any = TRUE;
        }
        if (v[k][0] < p->minX) p->minX = v[k][0];
        if (v[k][0] > p->maxX) p->maxX = v[k][0];
        if (v[k][2] < p->minZ) p->minZ = v[k][2];
        if (v[k][2] > p->maxZ) p->maxZ = v[k][2];
    }
}

// Writes the triangle's height at every cell centre it covers (keeping the
// lowest surface per cell). Near-vertical triangles cover no centres.
void RasterCB(const RwReal a[3], const RwReal b[3], const RwReal c[3], void *) {
    const RwReal x0 = a[0], z0 = a[2], x1 = b[0], z1 = b[2], x2 = c[0], z2 = c[2];
    const RwReal det = (z1 - z2) * (x0 - x2) + (x2 - x1) * (z0 - z2);
    if (std::fabs(det) < 1.0e-6f) {
        return;
    }
    RwReal tminX = x0, tmaxX = x0, tminZ = z0, tmaxZ = z0;
    if (x1 < tminX) tminX = x1; if (x1 > tmaxX) tmaxX = x1;
    if (x2 < tminX) tminX = x2; if (x2 > tmaxX) tmaxX = x2;
    if (z1 < tminZ) tminZ = z1; if (z1 > tmaxZ) tmaxZ = z1;
    if (z2 < tminZ) tminZ = z2; if (z2 > tmaxZ) tmaxZ = z2;
    RwInt32 cx0 = (RwInt32)std::floor((tminX - s_map.minX) / kCell);
    RwInt32 cx1 = (RwInt32)std::floor((tmaxX - s_map.minX) / kCell);
    RwInt32 cz0 = (RwInt32)std::floor((tminZ - s_map.minZ) / kCell);
    RwInt32 cz1 = (RwInt32)std::floor((tmaxZ - s_map.minZ) / kCell);
    if (cx0 < 0) cx0 = 0;
    if (cz0 < 0) cz0 = 0;
    if (cx1 >= s_map.w) cx1 = s_map.w - 1;
    if (cz1 >= s_map.h) cz1 = s_map.h - 1;
    for (RwInt32 cz = cz0; cz <= cz1; ++cz) {
        const RwReal pz = s_map.minZ + ((RwReal)cz + 0.5f) * kCell;
        for (RwInt32 cx = cx0; cx <= cx1; ++cx) {
            const RwReal px = s_map.minX + ((RwReal)cx + 0.5f) * kCell;
            const RwReal l0 = ((z1 - z2) * (px - x2) + (x2 - x1) * (pz - z2)) / det;
            const RwReal l1 = ((z2 - z0) * (px - x2) + (x0 - x2) * (pz - z2)) / det;
            const RwReal l2 = 1.0f - l0 - l1;
            if (l0 < 0.0f || l1 < 0.0f || l2 < 0.0f) {
                continue;
            }
            const RwReal y = l0 * a[1] + l1 * b[1] + l2 * c[1];
            RwReal &cell = s_map.lowest[(size_t)cz * (size_t)s_map.w + (size_t)cx];
            if (y < cell) {
                cell = y;
            }
        }
    }
}

RwBool IsLand(RwReal x, RwReal z) {
    if (s_map.w <= 0) {
        return FALSE;
    }
    const RwInt32 cx = (RwInt32)std::floor((x - s_map.minX) / kCell);
    const RwInt32 cz = (RwInt32)std::floor((z - s_map.minZ) / kCell);
    if (cx < 0 || cz < 0 || cx >= s_map.w || cz >= s_map.h) {
        return FALSE;
    }
    const RwReal y = s_map.lowest[(size_t)cz * (size_t)s_map.w + (size_t)cx];
    return (y < kNoSurface && y > s_waterY + kLandAboveWater) ? TRUE : FALSE;
}

} // namespace

RwInt32 ShipCollision_BuildLandMap(void *pWorldClump, RwReal waterY) {
    ShipCollision_Clear();
    s_waterY = waterY;
    if (pWorldClump == nullptr) {
        return 0;
    }
    RpClump *pClump = static_cast<RpClump *>(pWorldClump);
    BoundsCtx bounds = {0.0f, 0.0f, 0.0f, 0.0f, FALSE};
    RpClumpPortForAllTriangles(pClump, &BoundsCB, &bounds);
    if (bounds.any != TRUE) {
        return 0;
    }
    s_map.minX = bounds.minX;
    s_map.minZ = bounds.minZ;
    s_map.w = (RwInt32)std::ceil((bounds.maxX - bounds.minX) / kCell) + 1;
    s_map.h = (RwInt32)std::ceil((bounds.maxZ - bounds.minZ) / kCell) + 1;
    if (s_map.w <= 0 || s_map.h <= 0 || (double)s_map.w * s_map.h > 8.0e6) {
        s_map.w = s_map.h = 0;
        return 0;
    }
    s_map.lowest.assign((size_t)s_map.w * (size_t)s_map.h, kNoSurface);
    const RwInt32 tris = RpClumpPortForAllTriangles(pClump, &RasterCB, nullptr);
    RwInt32 land = 0;
    for (size_t i = 0; i < s_map.lowest.size(); ++i) {
        if (s_map.lowest[i] < kNoSurface && s_map.lowest[i] > waterY + kLandAboveWater) {
            ++land;
        }
    }
    std::printf("ShipCollision: land map %dx%d cells (%.1f m) from %d triangles, %d land cells\n",
                (int)s_map.w, (int)s_map.h, kCell, (int)tris, (int)land);
    return land;
}

void ShipCollision_Clear(void) {
    s_map.w = s_map.h = 0;
    s_map.lowest.clear();
    s_obstacles.clear();
}

void ShipCollision_SetObstacles(const RwReal *pXZR, RwInt32 count) {
    s_obstacles.clear();
    if (pXZR != nullptr && count > 0) {
        s_obstacles.assign(pXZR, pXZR + count * 3);
    }
}

RwBool ShipCollision_IsBlocked(RwReal x, RwReal z) {
    if (IsLand(x, z) == TRUE) {
        return TRUE;
    }
    for (size_t i = 0; i + 2 < s_obstacles.size(); i += 3) {
        const RwReal dx = x - s_obstacles[i], dz = z - s_obstacles[i + 1];
        const RwReal r = s_obstacles[i + 2];
        if (dx * dx + dz * dz < r * r) {
            return TRUE;
        }
    }
    return FALSE;
}
