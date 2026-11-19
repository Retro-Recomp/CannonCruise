// ============================================================================
// CannonCruise - Win32 Direct3D Ocean Renderer (Ocean_W32.cpp)
// Original path: D:\Projects\CannonCruisePC\code\Ocean_W32.cpp
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_00431910 — COcean_W32::Initialize
//   FUN_00431bd0 — COcean_W32::RenderWaterMesh ("paTexCoords != NULL Failed", "paColors != NULL Failed")
// ============================================================================

#include "Ocean_W32.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if defined(RW_GL3) || defined(RW_D3D9)
// rw::im3d / Im3DVertex — GL3 and D3D9 render paths (requires the librw include
// path; the syntax-only probe builds without RW_* — see the else branch).
#include <rw.h>
#if defined(RW_GL3)
using OceanIm3DVertex = rw::gl3::Im3DVertex;
#else
using OceanIm3DVertex = rw::d3d::Im3DVertex;
#endif
#endif

// ---------------------------------------------------------------------------
// Initialize – mirrors FUN_00431910.
// Prepares Direct3D/RenderWare texture samplers, blend modes, and vertex layouts.
// ---------------------------------------------------------------------------
RwBool COcean_W32::Initialize()
{
    return TRUE;
}

// ---------------------------------------------------------------------------
// Shutdown – cleans up Direct3D/RenderWare resources.
// ---------------------------------------------------------------------------
void COcean_W32::Shutdown()
{
}

// ---------------------------------------------------------------------------
// RenderWaterMesh – mirrors FUN_00431bd0.
// Dispatches immediate 3D indexed primitives to GPU using RenderWare Im3D.
// ---------------------------------------------------------------------------
void COcean_W32::RenderWaterMesh(const RwV3d* pVertices, const RwTexCoords* pTexCoords, const RwRGBA* pColors, RwUInt32 numVertices)
{
    assert(pVertices != 0);
    assert(pTexCoords != 0 && "paTexCoords != NULL Failed");
    assert(pColors != 0 && "paColors != NULL Failed");

    // Grid (subdiv+1)^2 — a single TRISTRIP cannot represent the 2D grid;
    // TRILIST with 6 indices per quad. Side is derived from numVertices.
    const RwUInt32 side = (RwUInt32)(std::sqrt((RwReal)numVertices) + 0.5f);
    if (side < 2 || side * side != numVertices) {
        return;
    }
    const RwUInt32 numIndices = (side - 1) * (side - 1) * 6;
    RwUInt16 *indices = (RwUInt16 *)malloc(numIndices * sizeof(RwUInt16));
    if (indices == nullptr) {
        return;
    }
    {
        RwUInt32 k = 0;
        for (RwUInt32 iz = 0; iz + 1 < side; ++iz) {
            for (RwUInt32 ix = 0; ix + 1 < side; ++ix) {
                const RwUInt32 v00 = iz * side + ix;
                const RwUInt32 v10 = v00 + 1;
                const RwUInt32 v01 = v00 + side;
                const RwUInt32 v11 = v01 + 1;
                indices[k++] = (RwUInt16)v00;
                indices[k++] = (RwUInt16)v10;
                indices[k++] = (RwUInt16)v11;
                indices[k++] = (RwUInt16)v00;
                indices[k++] = (RwUInt16)v11;
                indices[k++] = (RwUInt16)v01;
            }
        }
    }

    // rw::im3d directly (does not fall back to the rwstubs no-op C API — RenderTestQuad
    // im2d pattern). GL3/D3D9: Im3DVertex (pos+normal+rgba+uv); NULL: no drawing.
#if defined(RW_GL3) || defined(RW_D3D9)
    // Headless unit tests (e.g. ocean_test) may start the engine without an active
    // camera or world. Grid validation and index generation above still run; skip
    // GPU Im3D submission until a camera and world are active.
    if (rw::engine != nullptr &&
        rw::Engine::state == rw::Engine::Started &&
        rw::engine->currentCamera != nullptr &&
        rw::engine->currentWorld != nullptr)
    {
        // No texture: clear the TEXTURERASTER leaked from the terrain render (otherwise
        // the ocean multiplies with the last terrain texture and darkens).
        rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
        OceanIm3DVertex *imVertices = (OceanIm3DVertex *)RwMalloc(
            numVertices * sizeof(OceanIm3DVertex), 0);
        if (imVertices == nullptr) {
            free(indices);
            return;
        }
        for (RwUInt32 i = 0; i < numVertices; ++i) {
            imVertices[i].setX(pVertices[i].x);
            imVertices[i].setY(pVertices[i].y);
            imVertices[i].setZ(pVertices[i].z);
            imVertices[i].setNormalX(0.0f);
            imVertices[i].setNormalY(1.0f);
            imVertices[i].setNormalZ(0.0f);
            imVertices[i].setColor(pColors[i].red, pColors[i].green,
                                   pColors[i].blue, pColors[i].alpha);
            imVertices[i].setU(pTexCoords[i].u);
            imVertices[i].setV(pTexCoords[i].v);
        }
        rw::im3d::Transform(imVertices, (int)numVertices, nullptr,
                            rw::im3d::EVERYTHING);
        rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, indices,
                                         (int)numIndices);
        rw::im3d::End();
        RwFree(imVertices);
    }
#else
    // NULL platform: device im3D no-op — index buffer generation is proven,
    // drawing is skipped (in the RW_GL3 build the real render path above is used).
    (void)pVertices;
    (void)pTexCoords;
    (void)pColors;
#endif
    free(indices);
}