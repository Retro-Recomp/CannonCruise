// ============================================================================
// CannonCruise - Placeholder Player Boat (PlayerShip.cpp)
// ============================================================================

#include "PlayerShip.h"
#include "RenderPlatform.h"
#include "PhysicsWorld.h"
#include "PhysicsBackend.h"
#include <cmath>
#include <cstdio>
#include <cstring>

#if defined(RW_GL3) || defined(RW_D3D9)
// librw im3d immediate-mode API (same path as the ocean tiles)
#include <rw.h>
#endif

namespace {

PlayerShipState s_ship = {};
RwBool s_active = FALSE;
void *s_pShipBody = nullptr;
void *s_pShipShape = nullptr;
RwBool s_mapBoundsSet = FALSE;
RwReal s_mapMinX = 0.0f;
RwReal s_mapMaxX = 0.0f;
RwReal s_mapMinZ = 0.0f;
RwReal s_mapMaxZ = 0.0f;
const RwReal kShipScale = 4.0f;    ///< longboat model scale (game world meters)
const RwReal kMapClearance = 22.0f;

const RwReal kMaxSpeed = 18.0f;    ///< full throttle target speed (units/s)
const RwReal kReverseSpeed = -9.0f;///< full reverse throttle target speed
const RwReal kSpeedLerp = 0.9f;    ///< speed convergence coefficient (1/s)
const RwReal kTurnRate = 0.55f;    ///< rad/s (full rudder + full speed)
const RwReal kWaterY = 0.0f;       ///< water level

void ClampShipToMap() {
    if (s_mapBoundsSet != TRUE) {
        return;
    }
    const RwReal minX = s_mapMinX + kMapClearance;
    const RwReal maxX = s_mapMaxX - kMapClearance;
    const RwReal minZ = s_mapMinZ + kMapClearance;
    const RwReal maxZ = s_mapMaxZ - kMapClearance;
    if (s_ship.pos.x < minX) s_ship.pos.x = minX;
    if (s_ship.pos.x > maxX) s_ship.pos.x = maxX;
    if (s_ship.pos.z < minZ) s_ship.pos.z = minZ;
    if (s_ship.pos.z > maxZ) s_ship.pos.z = maxZ;
}

#if defined(RW_GL3) || defined(RW_D3D9)
#if defined(RW_GL3)
using ShipIm3DVertex = rw::gl3::Im3DVertex;
#else
using ShipIm3DVertex = rw::d3d::Im3DVertex;
#endif

const char *kShipHullTextureName = "CC_ShipHull";
RwBool s_shipTextureErrorReported = FALSE;

RwUInt8 ClampTextureChannel(int value) {
    return (RwUInt8)((value < 0) ? 0 : (value > 255) ? 255 : value);
}

void EnsureShipTextures() {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    if (RenderPlatform_GetTexture(kShipHullTextureName) != nullptr) {
        return;
    }

    const RwInt32 width = 64;
    const RwInt32 height = 64;
    RwUInt8 hull[width * height * 4];
    for (RwInt32 y = 0; y < height; ++y) {
        for (RwInt32 x = 0; x < width; ++x) {
            const RwInt32 offset = (y * width + x) * 4;
            const RwInt32 plank = x / 16;
            const RwInt32 grain = ((y * 13 + x * 7 + plank * 11) % 19) - 9;
            const RwBool seam =
                (x % 16 < 2 || (y + plank * 9) % 32 < 2) ? TRUE : FALSE;
            const RwInt32 wood = seam ? 48 : 120 + plank * 7 + grain;
            hull[offset + 0] = ClampTextureChannel(wood + 22);
            hull[offset + 1] = ClampTextureChannel(wood * 3 / 5 + 10);
            hull[offset + 2] = ClampTextureChannel(wood * 7 / 20 + 4);
            hull[offset + 3] = 255;
        }
    }

    if (RenderPlatform_UploadTexture(kShipHullTextureName, width, height,
                                    hull, width * 4) != TRUE &&
        s_shipTextureErrorReported != TRUE) {
        std::fprintf(stderr, "PlayerShip: failed to upload generated hull texture\n");
        s_shipTextureErrorReported = TRUE;
    }
}

struct ShipLocalVertex {
    RwV3d position;
    RwReal u;
    RwReal v;
    RwUInt8 red;
    RwUInt8 green;
    RwUInt8 blue;
    RwUInt8 alpha;
};

struct ShipHullStation {
    RwReal z;
    RwReal width;
    RwReal gunwaleY;
    RwReal chineY;
    RwReal keelY;
};

const ShipHullStation kHullStations[] = {
    {-4.6f, 0.08f, 1.20f, 0.62f, 0.06f},
    {-3.7f, 0.72f, 0.76f, 0.10f, -0.55f},
    {-2.0f, 1.30f, 0.62f, -0.20f, -0.80f},
    {0.0f, 1.65f, 0.60f, -0.24f, -0.86f},
    {2.0f, 1.30f, 0.62f, -0.20f, -0.80f},
    {3.7f, 0.72f, 0.76f, 0.10f, -0.55f},
    {4.6f, 0.08f, 1.20f, 0.62f, 0.06f},
};

ShipLocalVertex MakeShipVertex(const RwV3d &position, RwReal u, RwReal v,
                               RwUInt8 red, RwUInt8 green, RwUInt8 blue) {
    ShipLocalVertex vertex = {};
    vertex.position = position;
    vertex.u = u;
    vertex.v = v;
    vertex.red = red;
    vertex.green = green;
    vertex.blue = blue;
    vertex.alpha = 255;
    return vertex;
}

void AddTriangle(RwUInt16 *indices, int &indexCount, RwUInt16 a, RwUInt16 b,
                 RwUInt16 c) {
    indices[indexCount++] = a;
    indices[indexCount++] = b;
    indices[indexCount++] = c;
}

void AddQuad(RwUInt16 *indices, int &indexCount, RwUInt16 a, RwUInt16 b,
             RwUInt16 c, RwUInt16 d) {
    AddTriangle(indices, indexCount, a, b, c);
    AddTriangle(indices, indexCount, a, c, d);
}

RwV3d HullSidePoint(const ShipHullStation &station, RwReal depth,
                    RwReal side) {
    const RwV3d upper = {side * station.width, station.gunwaleY, station.z};
    const RwV3d chine = {
        side * station.width * 0.65f, station.chineY, station.z};
    const RwV3d keel = {0.0f, station.keelY, station.z};
    const RwV3d &start = depth <= 0.5f ? upper : chine;
    const RwV3d &end = depth <= 0.5f ? chine : keel;
    const RwReal t = depth <= 0.5f ? depth * 2.0f : (depth - 0.5f) * 2.0f;
    return {start.x + (end.x - start.x) * t,
            start.y + (end.y - start.y) * t,
            station.z};
}

void AppendBox(ShipLocalVertex *vertices, int &vertexCount,
               RwUInt16 *indices, int &indexCount, RwReal minX, RwReal minY,
               RwReal minZ, RwReal maxX, RwReal maxY, RwReal maxZ,
               RwUInt8 red, RwUInt8 green, RwUInt8 blue) {
    const RwUInt16 base = (RwUInt16)vertexCount;
    const RwV3d corners[8] = {
        {minX, minY, minZ}, {maxX, minY, minZ},
        {maxX, maxY, minZ}, {minX, maxY, minZ},
        {minX, minY, maxZ}, {maxX, minY, maxZ},
        {maxX, maxY, maxZ}, {minX, maxY, maxZ},
    };
    for (int i = 0; i < 8; ++i) {
        vertices[vertexCount++] =
            MakeShipVertex(corners[i], 0.0f, 0.0f, red, green, blue);
    }
    const RwUInt16 boxIndices[36] = {
        0, 3, 2, 0, 2, 1, 4, 5, 6, 4, 6, 7,
        0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
        0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2,
    };
    for (int i = 0; i < 36; ++i) {
        indices[indexCount++] = (RwUInt16)(base + boxIndices[i]);
    }
}

void AppendBeam(ShipLocalVertex *vertices, int &vertexCount,
                RwUInt16 *indices, int &indexCount, const RwV3d &start,
                const RwV3d &end, RwReal halfWidth, RwReal halfHeight,
                RwUInt8 red, RwUInt8 green, RwUInt8 blue) {
    RwV3d direction = {end.x - start.x, end.y - start.y, end.z - start.z};
    const RwReal length = std::sqrt(direction.x * direction.x +
                                    direction.y * direction.y +
                                    direction.z * direction.z);
    if (length <= 0.0001f) {
        return;
    }
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;
    RwV3d side = {-direction.z, 0.0f, direction.x};
    RwReal sideLength =
        std::sqrt(side.x * side.x + side.z * side.z);
    if (sideLength <= 0.0001f) {
        side.x = 1.0f;
        side.z = 0.0f;
        sideLength = 1.0f;
    }
    side.x *= halfWidth / sideLength;
    side.z *= halfWidth / sideLength;
    RwV3d up = {
        side.y * direction.z - side.z * direction.y,
        side.z * direction.x - side.x * direction.z,
        side.x * direction.y - side.y * direction.x,
    };
    const RwReal upLength =
        std::sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
    if (upLength <= 0.0001f) {
        return;
    }
    up.x *= halfHeight / upLength;
    up.y *= halfHeight / upLength;
    up.z *= halfHeight / upLength;

    const RwUInt16 base = (RwUInt16)vertexCount;
    const RwReal signs[4][2] = {
        {-1.0f, -1.0f}, {1.0f, -1.0f},
        {1.0f, 1.0f}, {-1.0f, 1.0f},
    };
    for (int endIndex = 0; endIndex < 2; ++endIndex) {
        const RwV3d &center = endIndex == 0 ? start : end;
        for (int i = 0; i < 4; ++i) {
            const RwV3d position = {
                center.x + side.x * signs[i][0] + up.x * signs[i][1],
                center.y + side.y * signs[i][0] + up.y * signs[i][1],
                center.z + side.z * signs[i][0] + up.z * signs[i][1],
            };
            vertices[vertexCount++] =
                MakeShipVertex(position, 0.0f, 0.0f, red, green, blue);
        }
    }
    const RwUInt16 beamIndices[36] = {
        0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
        0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5,
        2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7,
    };
    for (int i = 0; i < 36; ++i) {
        indices[indexCount++] = (RwUInt16)(base + beamIndices[i]);
    }
}

void AppendEllipsoid(ShipLocalVertex *vertices, int &vertexCount,
                     RwUInt16 *indices, int &indexCount, const RwV3d &center,
                     const RwV3d &radii, RwUInt8 red, RwUInt8 green,
                     RwUInt8 blue) {
    const int segments = 8;
    const RwUInt16 base = (RwUInt16)vertexCount;
    vertices[vertexCount++] = MakeShipVertex(
        {center.x, center.y + radii.y, center.z}, 0.5f, 0.0f,
        red, green, blue);
    for (int ring = 0; ring < 2; ++ring) {
        const RwReal angle = ring == 0 ? 0.78539816f : 2.35619449f;
        const RwReal ringScale = std::fabs(std::cos(angle));
        const RwReal y = center.y + radii.y * std::sin(angle);
        for (int segment = 0; segment < segments; ++segment) {
            const RwReal around =
                6.28318530718f * (RwReal)segment / segments;
            vertices[vertexCount++] = MakeShipVertex(
                {center.x + radii.x * ringScale * std::cos(around), y,
                 center.z + radii.z * ringScale * std::sin(around)},
                (RwReal)segment / segments, (RwReal)(ring + 1) / 3.0f,
                red, green, blue);
        }
    }
    const RwUInt16 bottom = (RwUInt16)vertexCount;
    vertices[vertexCount++] = MakeShipVertex(
        {center.x, center.y - radii.y, center.z}, 0.5f, 1.0f,
        red, green, blue);
    for (int segment = 0; segment < segments; ++segment) {
        const RwUInt16 current = (RwUInt16)(base + 1 + segment);
        const RwUInt16 next = (RwUInt16)(base + 1 +
                                         (segment + 1) % segments);
        AddTriangle(indices, indexCount, base, current, next);
    }
    for (int segment = 0; segment < segments; ++segment) {
        const RwUInt16 upper = (RwUInt16)(base + 1 + segment);
        const RwUInt16 upperNext = (RwUInt16)(base + 1 +
                                              (segment + 1) % segments);
        const RwUInt16 lower = (RwUInt16)(base + 1 + segments + segment);
        const RwUInt16 lowerNext = (RwUInt16)(base + 1 + segments +
                                              (segment + 1) % segments);
        AddQuad(indices, indexCount, upper, upperNext, lowerNext, lower);
        AddTriangle(indices, indexCount, bottom, lowerNext, lower);
    }
}

void DrawShipMesh(const ShipLocalVertex *vertices, int vertexCount,
                  const RwUInt16 *indices, int indexCount,
                  rw::Raster *textureRaster) {
    ShipIm3DVertex transformed[256];
    if (vertexCount <= 0 || vertexCount > 256 || indexCount <= 0) {
        return;
    }
    const RwReal cy = std::cos(s_ship.yaw);
    const RwReal sy = std::sin(s_ship.yaw);
    for (int i = 0; i < vertexCount; ++i) {
        const RwReal lx = vertices[i].position.x * kShipScale;
        const RwReal ly = vertices[i].position.y * kShipScale;
        const RwReal lz = vertices[i].position.z * kShipScale;
        transformed[i].setX(s_ship.pos.x + cy * lx + sy * lz);
        transformed[i].setY(s_ship.pos.y + ly);
        transformed[i].setZ(s_ship.pos.z - sy * lx + cy * lz);
        transformed[i].setNormalX(0.0f);
        transformed[i].setNormalY(1.0f);
        transformed[i].setNormalZ(0.0f);
        transformed[i].setColor(vertices[i].red, vertices[i].green,
                                vertices[i].blue, vertices[i].alpha);
        transformed[i].setU(vertices[i].u);
        transformed[i].setV(vertices[i].v);
    }
    rw::SetRenderStatePtr(rw::TEXTURERASTER, textureRaster);
    rw::im3d::Transform(transformed, vertexCount, nullptr, rw::im3d::EVERYTHING);
    rw::im3d::RenderIndexedPrimitive(
        rw::PRIMTYPETRILIST, const_cast<RwUInt16 *>(indices), indexCount);
    rw::im3d::End();
}

void PlayerShip_RenderHull() {
    if (std::getenv("CANNONCRUISE_QUAD_TEST") != nullptr) {
        // ocean-like 2x2 subdivided quad at the ship position
        const RwReal cx = s_ship.pos.x, cz = s_ship.pos.z;
        ShipIm3DVertex im[9];
        const RwReal uvs[9][2] = {{0,0},{0.5f,0},{1,0},{0,0.5f},{0.5f,0.5f},{1,0.5f},{0,1},{0.5f,1},{1,1}};
        for (int i = 0; i < 9; ++i) {
            im[i].setX(cx - 15.0f + uvs[i][0] * 30.0f);
            im[i].setY(0.5f);
            im[i].setZ(cz - 15.0f + uvs[i][1] * 30.0f);
            im[i].setNormalX(0.0f);
            im[i].setNormalY(1.0f);
            im[i].setNormalZ(0.0f);
            im[i].setColor(255, 0, 255, 255);
            im[i].setU(uvs[i][0]);
            im[i].setV(uvs[i][1]);
        }
        RwUInt16 qIdx[24] = {0,1,3, 1,4,3, 1,2,4, 2,5,4,
                             3,4,6, 4,7,6, 4,5,7, 5,8,7};
        rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
        rw::im3d::Transform(im, 9, nullptr, rw::im3d::EVERYTHING);
        rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, (void *)qIdx, 24);
        rw::im3d::End();
        return;
    }
    if (s_active != TRUE) {
        return;
    }
    if (std::getenv("CANNONCRUISE_HULL_TRI") != nullptr) {
        ShipIm3DVertex im[3];
        const RwReal tx[3] = {25.0f, 44.0f, 34.4f};
        const RwReal ty[3] = {2.0f, 2.0f, 14.0f};
        const RwReal tz[3] = {390.0f, 390.0f, 390.0f};
        for (int i = 0; i < 3; ++i) {
            im[i].setX(tx[i]);
            im[i].setY(ty[i]);
            im[i].setZ(tz[i]);
            im[i].setNormalX(0.0f);
            im[i].setNormalY(1.0f);
            im[i].setNormalZ(0.0f);
            im[i].setColor(255, 0, 255, 255);
            im[i].setU(0.0f);
            im[i].setV(0.0f);
        }
        const char *mode = std::getenv("CANNONCRUISE_TRI_MODE");
        RwUInt32 flags =
            (mode != nullptr && mode[0] == '2')
                ? (rw::im3d::VERTEXXYZ | rw::im3d::VERTEXRGBA | rw::im3d::ALLOPAQUE)
                : rw::im3d::EVERYTHING;
        rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
        rw::im3d::Transform(im, 3, nullptr, flags);
        rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);
        rw::im3d::End();
        return;
    }
    EnsureShipTextures();
    rw::Texture *hullTexture =
        static_cast<rw::Texture *>(RenderPlatform_GetTexture(kShipHullTextureName));
    const RwUInt32 oldCullMode = rw::GetRenderState(rw::CULLMODE);
    rw::SetRenderState(rw::CULLMODE, rw::CULLNONE);

    const int stationCount =
        (int)(sizeof(kHullStations) / sizeof(kHullStations[0]));
    ShipLocalVertex hullVertices[7 * 5];
    RwUInt16 hullIndices[6 * 4 * 6];
    int hullIndexCount = 0;
    for (int stationIndex = 0; stationIndex < stationCount; ++stationIndex) {
        const ShipHullStation &station = kHullStations[stationIndex];
        const RwReal v = (RwReal)stationIndex / (stationCount - 1);
        const RwV3d points[5] = {
            {0.0f, station.keelY, station.z},
            {-station.width * 0.65f, station.chineY, station.z},
            {-station.width, station.gunwaleY, station.z},
            {station.width, station.gunwaleY, station.z},
            {station.width * 0.65f, station.chineY, station.z},
        };
        const RwReal u[5] = {0.5f, 0.25f, 0.0f, 1.0f, 0.75f};
        for (int pointIndex = 0; pointIndex < 5; ++pointIndex) {
            hullVertices[stationIndex * 5 + pointIndex] =
                MakeShipVertex(points[pointIndex], u[pointIndex], v,
                               255, 255, 255);
        }
    }
    const RwUInt16 hullBands[4][2] = {{0, 1}, {1, 2}, {3, 4}, {4, 0}};
    for (int stationIndex = 0; stationIndex < stationCount - 1; ++stationIndex) {
        for (int bandIndex = 0; bandIndex < 4; ++bandIndex) {
            const RwUInt16 a =
                (RwUInt16)(stationIndex * 5 + hullBands[bandIndex][0]);
            const RwUInt16 b =
                (RwUInt16)(stationIndex * 5 + hullBands[bandIndex][1]);
            const RwUInt16 c =
                (RwUInt16)((stationIndex + 1) * 5 + hullBands[bandIndex][1]);
            const RwUInt16 d =
                (RwUInt16)((stationIndex + 1) * 5 + hullBands[bandIndex][0]);
            AddQuad(hullIndices, hullIndexCount, a, b, c, d);
        }
    }
    DrawShipMesh(hullVertices, stationCount * 5, hullIndices, hullIndexCount,
                 hullTexture != nullptr ? hullTexture->raster : nullptr);

    const RwUInt8 plankColors[4][3] = {
        {126, 74, 39}, {103, 58, 31}, {139, 83, 45}, {116, 66, 35},
    };
    ShipLocalVertex plankVertices[2 * 4 * 7 * 2];
    RwUInt16 plankIndices[2 * 4 * 6 * 6];
    int plankVertexCount = 0;
    int plankIndexCount = 0;
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
        const RwReal side = sideIndex == 0 ? -1.0f : 1.0f;
        for (int plank = 0; plank < 4; ++plank) {
            const RwUInt16 stripBase = (RwUInt16)plankVertexCount;
            const RwUInt8 *color = plankColors[plank];
            for (int stationIndex = 0; stationIndex < stationCount;
                 ++stationIndex) {
                const ShipHullStation &station = kHullStations[stationIndex];
                RwV3d upper =
                    HullSidePoint(station, (RwReal)plank / 4.0f, side);
                RwV3d lower =
                    HullSidePoint(station, (RwReal)(plank + 1) / 4.0f, side);
                upper.x += side * 0.035f;
                lower.x += side * 0.035f;
                upper.y += 0.012f;
                lower.y += 0.012f;
                plankVertices[plankVertexCount++] =
                    MakeShipVertex(upper, 0.0f, 0.0f,
                                   color[0], color[1], color[2]);
                plankVertices[plankVertexCount++] =
                    MakeShipVertex(lower, 0.0f, 0.0f,
                                   color[0], color[1], color[2]);
            }
            for (int stationIndex = 0; stationIndex < stationCount - 1;
                 ++stationIndex) {
                const RwUInt16 current =
                    (RwUInt16)(stripBase + stationIndex * 2);
                AddQuad(plankIndices, plankIndexCount, current, current + 1,
                        current + 3, current + 2);
            }
        }
    }
    DrawShipMesh(plankVertices, plankVertexCount, plankIndices,
                 plankIndexCount, nullptr);

    ShipLocalVertex deckVertices[7 * 3];
    RwUInt16 deckIndices[6 * 2 * 6];
    int deckIndexCount = 0;
    for (int stationIndex = 0; stationIndex < stationCount; ++stationIndex) {
        const ShipHullStation &station = kHullStations[stationIndex];
        const RwReal y = station.gunwaleY - 0.055f;
        const RwReal v = (RwReal)stationIndex / (stationCount - 1);
        deckVertices[stationIndex * 3] =
            MakeShipVertex({0.0f, y, station.z}, 0.5f, v, 255, 255, 255);
        deckVertices[stationIndex * 3 + 1] =
            MakeShipVertex({-station.width * 0.78f, y, station.z},
                           0.0f, v, 255, 255, 255);
        deckVertices[stationIndex * 3 + 2] =
            MakeShipVertex({station.width * 0.78f, y, station.z},
                           1.0f, v, 255, 255, 255);
    }
    for (int stationIndex = 0; stationIndex < stationCount - 1; ++stationIndex) {
        const RwUInt16 current = (RwUInt16)(stationIndex * 3);
        const RwUInt16 next = (RwUInt16)((stationIndex + 1) * 3);
        AddQuad(deckIndices, deckIndexCount, current, current + 1,
                next + 1, next);
        AddQuad(deckIndices, deckIndexCount, current, next,
                next + 2, current + 2);
    }
    DrawShipMesh(deckVertices, stationCount * 3, deckIndices, deckIndexCount,
                 hullTexture != nullptr ? hullTexture->raster : nullptr);

    ShipLocalVertex gunwaleVertices[2 * 6 * 8];
    RwUInt16 gunwaleIndices[2 * 6 * 36];
    int gunwaleVertexCount = 0;
    int gunwaleIndexCount = 0;
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
        const RwReal side = sideIndex == 0 ? -1.0f : 1.0f;
        for (int stationIndex = 0; stationIndex < stationCount - 1;
             ++stationIndex) {
            const ShipHullStation &a = kHullStations[stationIndex];
            const ShipHullStation &b = kHullStations[stationIndex + 1];
            const RwV3d start = {
                side * a.width * 0.96f, a.gunwaleY + 0.045f, a.z};
            const RwV3d end = {
                side * b.width * 0.96f, b.gunwaleY + 0.045f, b.z};
            AppendBeam(gunwaleVertices, gunwaleVertexCount, gunwaleIndices,
                       gunwaleIndexCount, start, end, 0.045f, 0.045f,
                       144, 90, 47);
        }
    }
    DrawShipMesh(gunwaleVertices, gunwaleVertexCount, gunwaleIndices,
                 gunwaleIndexCount, nullptr);

    ShipLocalVertex detailVertices[192];
    RwUInt16 detailIndices[1024];
    int detailVertexCount = 0;
    int detailIndexCount = 0;
    const RwUInt8 wood[3] = {134, 83, 43};
    const RwUInt8 darkWood[3] = {91, 53, 29};
    const RwUInt8 rope[3] = {56, 44, 34};
    const RwUInt8 iron[3] = {49, 52, 54};

    const RwReal benchZ[3] = {-2.0f, -0.15f, 1.65f};
    for (int i = 0; i < 3; ++i) {
        AppendBox(detailVertices, detailVertexCount, detailIndices,
                  detailIndexCount, -0.73f, 0.61f, benchZ[i] - 0.12f,
                  0.73f, 0.76f, benchZ[i] + 0.12f,
                  darkWood[0], darkWood[1], darkWood[2]);
    }

    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              -0.48f, 0.62f, 0.95f, 0.48f, 0.88f, 2.48f,
              darkWood[0], darkWood[1], darkWood[2]);
    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              -0.56f, 0.72f, 1.05f, -0.36f, 1.18f, 2.24f,
              wood[0], wood[1], wood[2]);
    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              0.36f, 0.72f, 1.05f, 0.56f, 1.18f, 2.24f,
              wood[0], wood[1], wood[2]);
    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              -0.34f, 0.79f, 1.30f, -0.20f, 1.02f, 2.80f,
              wood[0], wood[1], wood[2]);
    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              0.20f, 0.79f, 1.30f, 0.34f, 1.02f, 2.80f,
              wood[0], wood[1], wood[2]);

    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.0f, 0.62f, 0.0f}, {0.0f, 5.15f, 0.0f},
               0.060f, 0.060f, wood[0], wood[1], wood[2]);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {-1.20f, 4.70f, 0.0f}, {1.20f, 4.70f, 0.0f},
               0.050f, 0.050f, wood[0], wood[1], wood[2]);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.0f, 5.02f, 0.0f}, {-1.30f, 0.78f, -3.30f},
               0.018f, 0.018f, rope[0], rope[1], rope[2]);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.0f, 5.02f, 0.0f}, {1.30f, 0.78f, -3.30f},
               0.018f, 0.018f, rope[0], rope[1], rope[2]);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.0f, 5.02f, 0.0f}, {-1.30f, 0.78f, 3.30f},
               0.018f, 0.018f, rope[0], rope[1], rope[2]);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.0f, 5.02f, 0.0f}, {1.30f, 0.78f, 3.30f},
               0.018f, 0.018f, rope[0], rope[1], rope[2]);

    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              -0.23f, 0.76f, -3.15f, 0.23f, 1.20f, -2.45f,
              darkWood[0], darkWood[1], darkWood[2]);
    AppendBox(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
              -0.10f, 5.15f, -0.04f, 0.12f, 5.28f, 0.10f,
              30, 31, 29);
    AppendBeam(detailVertices, detailVertexCount, detailIndices, detailIndexCount,
               {0.03f, 5.20f, 0.0f}, {0.32f, 5.20f, 0.0f},
               0.018f, 0.018f, 30, 31, 29);

    const int barrelSegments = 12;
    const RwReal barrelZ[3] = {1.10f, 1.95f, 2.35f};
    const RwReal barrelRadius[3] = {0.32f, 0.40f, 0.36f};
    const RwReal barrelCenterY[3] = {1.08f, 1.20f, 1.28f};
    const RwUInt16 barrelBase = (RwUInt16)detailVertexCount;
    for (int ring = 0; ring < 3; ++ring) {
        for (int segment = 0; segment < barrelSegments; ++segment) {
            const RwReal angle =
                6.28318530718f * (RwReal)segment / barrelSegments;
            const RwV3d position = {
                barrelRadius[ring] * std::cos(angle),
                barrelCenterY[ring] + barrelRadius[ring] * std::sin(angle),
                barrelZ[ring],
            };
            detailVertices[detailVertexCount++] =
                MakeShipVertex(position, 0.0f, 0.0f,
                               (RwUInt8)(ring == 1 ? 41 : iron[0]),
                               (RwUInt8)(ring == 1 ? 43 : iron[1]),
                               (RwUInt8)(ring == 1 ? 45 : iron[2]));
        }
    }
    for (int ring = 0; ring < 2; ++ring) {
        for (int segment = 0; segment < barrelSegments; ++segment) {
            const RwUInt16 a = (RwUInt16)(barrelBase +
                                           ring * barrelSegments + segment);
            const RwUInt16 b = (RwUInt16)(barrelBase +
                                           ring * barrelSegments +
                                           (segment + 1) % barrelSegments);
            const RwUInt16 c = (RwUInt16)(barrelBase +
                                           (ring + 1) * barrelSegments +
                                           (segment + 1) % barrelSegments);
            const RwUInt16 d = (RwUInt16)(barrelBase +
                                           (ring + 1) * barrelSegments + segment);
            AddQuad(detailIndices, detailIndexCount, a, b, c, d);
        }
    }
    const RwUInt16 muzzleCenter = (RwUInt16)detailVertexCount;
    detailVertices[detailVertexCount++] =
        MakeShipVertex({0.0f, barrelCenterY[2], barrelZ[2] + 0.005f},
                       0.0f, 0.0f, 16, 17, 18);
    for (int segment = 0; segment < barrelSegments; ++segment) {
        const RwReal angle =
            6.28318530718f * (RwReal)segment / barrelSegments;
        detailVertices[detailVertexCount++] =
            MakeShipVertex(
                {barrelRadius[2] * 0.62f * std::cos(angle),
                 barrelCenterY[2] + barrelRadius[2] * 0.62f * std::sin(angle),
                 barrelZ[2] + 0.005f},
                0.0f, 0.0f, 13, 14, 15);
    }
    for (int segment = 0; segment < barrelSegments; ++segment) {
        AddTriangle(detailIndices, detailIndexCount, muzzleCenter,
                    (RwUInt16)(muzzleCenter + 1 + segment),
                    (RwUInt16)(muzzleCenter + 1 +
                               (segment + 1) % barrelSegments));
    }

    DrawShipMesh(detailVertices, detailVertexCount, detailIndices,
                 detailIndexCount, nullptr);

    ShipLocalVertex crewVertices[160];
    RwUInt16 crewIndices[640];
    int crewVertexCount = 0;
    int crewIndexCount = 0;
    const RwUInt8 tunic[3] = {67, 112, 58};
    const RwUInt8 trousers[3] = {72, 54, 39};
    const RwUInt8 skin[3] = {205, 143, 96};
    const RwUInt8 hair[3] = {43, 35, 29};
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {-0.10f, 0.83f, -1.55f}, {-0.12f, 0.62f, -1.58f},
               0.065f, 0.065f, trousers[0], trousers[1], trousers[2]);
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {0.10f, 0.83f, -1.55f}, {0.12f, 0.62f, -1.58f},
               0.065f, 0.065f, trousers[0], trousers[1], trousers[2]);
    AppendBox(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
             -0.19f, 0.80f, -1.78f, 0.19f, 1.28f, -1.36f,
             tunic[0], tunic[1], tunic[2]);
    AppendBox(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
             -0.19f, 0.82f, -1.82f, 0.19f, 0.91f, -1.34f,
             darkWood[0], darkWood[1], darkWood[2]);
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {-0.16f, 1.20f, -1.58f}, {-0.30f, 1.02f, -1.36f},
               0.075f, 0.075f, tunic[0], tunic[1], tunic[2]);
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {0.16f, 1.20f, -1.58f}, {0.31f, 1.03f, -1.35f},
               0.075f, 0.075f, tunic[0], tunic[1], tunic[2]);
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {-0.30f, 1.02f, -1.36f}, {-0.35f, 0.95f, -1.24f},
               0.040f, 0.040f, skin[0], skin[1], skin[2]);
    AppendBeam(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
               {0.31f, 1.03f, -1.35f}, {0.36f, 0.96f, -1.23f},
               0.040f, 0.040f, skin[0], skin[1], skin[2]);
    AppendEllipsoid(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
                    {0.0f, 1.43f, -1.58f}, {0.17f, 0.20f, 0.15f},
                    skin[0], skin[1], skin[2]);
    AppendBox(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
             -0.18f, 1.53f, -1.75f, 0.18f, 1.64f, -1.40f,
             hair[0], hair[1], hair[2]);
    DrawShipMesh(crewVertices, crewVertexCount, crewIndices, crewIndexCount,
                 nullptr);

    ShipLocalVertex oarVertices[96];
    RwUInt16 oarIndices[432];
    int oarVertexCount = 0;
    int oarIndexCount = 0;
    const RwReal oarZ[3] = {-2.65f, -1.95f, -1.25f};
    for (int i = 0; i < 3; ++i) {
        const RwV3d pivot = {-1.35f, 0.59f, oarZ[i] - 0.16f};
        const RwV3d bladeRoot = {-1.92f, 0.45f, oarZ[i] - 0.33f};
        const RwV3d bladeTip = {-2.62f, 0.31f, oarZ[i] - 0.56f};
        const RwV3d inboard = {-0.77f, 0.73f, oarZ[i] + 0.10f};
        AppendBeam(oarVertices, oarVertexCount, oarIndices, oarIndexCount,
                   inboard, pivot, 0.035f, 0.035f,
                   wood[0], wood[1], wood[2]);
        AppendBeam(oarVertices, oarVertexCount, oarIndices, oarIndexCount,
                   pivot, bladeRoot, 0.040f, 0.040f,
                   wood[0], wood[1], wood[2]);
        AppendBeam(oarVertices, oarVertexCount, oarIndices, oarIndexCount,
                   bladeRoot, bladeTip, 0.13f, 0.035f,
                   218, 210, 187);
    }
    DrawShipMesh(oarVertices, oarVertexCount, oarIndices, oarIndexCount,
                 nullptr);

    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
    rw::SetRenderState(rw::CULLMODE, oldCullMode);
}
#endif // RW_GL3 || RW_D3D9

} // namespace

RwBool PlayerShip_SetMapBounds(RwReal minX, RwReal maxX,
                               RwReal minZ, RwReal maxZ) {
    s_mapBoundsSet = FALSE;
    if (!std::isfinite(minX) || !std::isfinite(maxX) ||
        !std::isfinite(minZ) || !std::isfinite(maxZ) ||
        maxX - minX <= 2.0f * kMapClearance ||
        maxZ - minZ <= 2.0f * kMapClearance) {
        return FALSE;
    }
    s_mapMinX = minX;
    s_mapMaxX = maxX;
    s_mapMinZ = minZ;
    s_mapMaxZ = maxZ;
    s_mapBoundsSet = TRUE;
    ClampShipToMap();
    if (s_pShipBody != nullptr) {
        PhysicsBody_SetPosition(s_pShipBody, s_ship.pos.x, s_ship.pos.y,
                                s_ship.pos.z);
    }
    return TRUE;
}

void PlayerShip_Spawn(const RwV3d *pPos, RwReal yaw) {
    std::memset(&s_ship, 0, sizeof(s_ship));
    if (pPos != nullptr) {
        s_ship.pos = *pPos;
    }
    s_ship.pos.y = kWaterY;
    s_ship.yaw = yaw;
    s_ship.speed = 0.0f;
    s_ship.throttle = 0.0f;
    s_ship.rudder = 0.0f;
    ClampShipToMap();
    s_active = TRUE;
#if defined(RW_GL3) || defined(RW_D3D9)
    EnsureShipTextures();
#endif
    // Keep the kinematic collision hull aligned with the rendered longboat.
    if (s_pShipShape == nullptr) {
        const RwReal hull[9][3] = {
            {0.0f, 1.2f, 4.6f}, {0.0f, 1.2f, -4.6f},
            {-1.65f, 0.6f, 0.0f}, {1.65f, 0.6f, 0.0f},
            {-0.72f, 0.76f, 3.7f}, {0.72f, 0.76f, 3.7f},
            {-0.72f, 0.76f, -3.7f}, {0.72f, 0.76f, -3.7f},
            {0.0f, -0.86f, 0.0f},
        };
        RwReal scaled[9][3];
        for (int i = 0; i < 9; ++i) {
            scaled[i][0] = hull[i][0] * kShipScale;
            scaled[i][1] = hull[i][1] * kShipScale;
            scaled[i][2] = hull[i][2] * kShipScale;
        }
        s_pShipShape = PhysicsShape_CreateConvexHull(&scaled[0][0], 9,
                                                      3 * sizeof(RwReal));
    }
    if (s_pShipBody == nullptr && s_pShipShape != nullptr) {
        const RwReal pos[3] = {s_ship.pos.x, s_ship.pos.y, s_ship.pos.z};
        s_pShipBody = PhysicsBody_CreateWithShape(s_pShipShape, 0.0f, pos,
                                                   nullptr);
        if (s_pShipBody != nullptr) {
            PhysicsBody_SetKinematic(s_pShipBody, TRUE);
            CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
            if (pWorld != nullptr) {
                pWorld->AddRigidBody(s_pShipBody);
            }
        }
    }
    if (s_pShipBody != nullptr) {
        PhysicsBody_SetPosition(s_pShipBody, s_ship.pos.x, s_ship.pos.y,
                                s_ship.pos.z);
    }
}

void PlayerShip_Shutdown(void) {
    CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
    if (s_pShipBody != nullptr) {
        if (pWorld != nullptr) {
            pWorld->RemoveRigidBody(s_pShipBody);
        }
        PhysicsBody_Destroy(s_pShipBody);
        s_pShipBody = nullptr;
    }
    if (s_pShipShape != nullptr) {
        PhysicsShape_Destroy(s_pShipShape);
        s_pShipShape = nullptr;
    }
    s_active = FALSE;
    s_mapBoundsSet = FALSE;
}

void PlayerShip_Update(RwReal deltaTime, RwReal waveT,
                       RwReal throttle, RwReal rudder) {
    if (s_active != TRUE || !std::isfinite(deltaTime) ||
        deltaTime <= 0.0f || !std::isfinite(waveT)) {
        return;
    }
    if (!std::isfinite(throttle)) throttle = 0.0f;
    if (!std::isfinite(rudder)) rudder = 0.0f;
    if (throttle > 1.0f) throttle = 1.0f;
    if (throttle < -1.0f) throttle = -1.0f;
    if (rudder > 1.0f) rudder = 1.0f;
    if (rudder < -1.0f) rudder = -1.0f;
    s_ship.throttle = throttle;
    s_ship.rudder = rudder;

    const RwReal target = (throttle >= 0.0f) ? throttle * kMaxSpeed
                                             : throttle * -kReverseSpeed;
    s_ship.speed += (target - s_ship.speed) * kSpeedLerp * deltaTime;

    // Rudder has no effect while stopped and gains authority with speed.
    RwReal speedNorm = std::fabs(s_ship.speed) / kMaxSpeed;
    if (speedNorm > 1.0f) speedNorm = 1.0f;
    s_ship.yaw -= rudder * kTurnRate * speedNorm *
                  ((s_ship.speed >= 0.0f) ? 1.0f : -1.0f) * deltaTime;

    s_ship.pos.x += std::sin(s_ship.yaw) * s_ship.speed * deltaTime;
    s_ship.pos.z += std::cos(s_ship.yaw) * s_ship.speed * deltaTime;
    ClampShipToMap();
    // Wave-top bob -- exactly the same phase/amplitude as Ocean's wave form
    // (sin(x*0.05+t*2)*1.5 + cos(z*0.08+t*1.5)*1.0)
    const RwReal wave = std::sin(s_ship.pos.x * 0.05f + waveT * 2.0f) * 1.5f +
                        std::cos(s_ship.pos.z * 0.08f + waveT * 1.5f) * 1.0f;
    s_ship.pos.y = kWaterY + 0.8f + wave;
    // Bullet -- match the kinematic body position with the manual integration
    if (s_pShipBody != nullptr) {
        PhysicsBody_SetPosition(s_pShipBody, s_ship.pos.x, s_ship.pos.y,
                                s_ship.pos.z);
    }
}

RwBool PlayerShip_GetState(PlayerShipState *pOut) {
    if (pOut == nullptr || s_active != TRUE) {
        return FALSE;
    }
    *pOut = s_ship;
    return TRUE;
}

void PlayerShip_Render() {
    if (s_active != TRUE) {
        return;
    }
#if defined(RW_GL3) || defined(RW_D3D9)
    PlayerShip_RenderHull();
#endif
}
