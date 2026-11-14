// ============================================================================
// CannonCruise - Render Platform (RenderPlatform.cpp)
// Phase 4.6n: librw scene setup + frame loop + Im2D test quad.
// Binary counterparts (documented): sub_4189E0 MainLoop, FUN_0041C380 Idle,
// sub_41CEB0 Render (Begin/EndUpdate), FUN_004cc1f0 RenderPipeline_Reset,
// FUN_004cc020 SetViewWindowFromCamera.
//
// NULL platform: device no-op -> headless smoke; GL3: real drawing
// (skipped if ShowRaster frameBuffer is nil -- engine.cpp:360 null-safety).
// ============================================================================

#if defined(RW_D3D9) && !defined(WITH_D3D)
// Open the librw d3d headers together with d3d9.h (for the d3ddevice extern).
#define WITH_D3D 1
#endif
#include "RenderPlatform.h"
#include "RenderPipeline.h"
#include "TxdDecode.h"

#include <rw.h>

#ifdef RW_GL3
// Raw GL (for mipmap completion) -- librw glad is not in this TU; framework
// linked (GL3 build). Not compiled in the NULL build.
#include <OpenGL/gl3.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

rw::World *s_pWorld = nullptr;
rw::Camera *s_pCamera = nullptr;
rw::Light *s_pAmbient = nullptr;
rw::Light *s_pDirectional = nullptr;
rw::Frame *s_pCameraFrame = nullptr;
rw::Frame *s_pLightFrame = nullptr;
RwBool s_lightsAdded = FALSE; // whether addLight was done (removeLight guard)
RwInt32 s_width = 0;
RwInt32 s_height = 0;
char s_screenshotPath[512] = {0}; // PNG capture before EndFrame swap

/// RCCCameraView + RenderPipeline_SetViewWindowFromCamera connection.
/// Camera viewWindow (half size) x2 + frame LTM (CCMatrix layout --
/// librw Matrix is exactly the same 64B) + raster size (if no frameBuffer,
/// platform width/height -- informational).
void ApplyCameraToPipeline(void) {
    RCCCameraView view;
    view.projFlag = s_pCamera->projection;
    view.rasterWidth = s_width;
    view.rasterHeight = s_height;
    view.viewWindowX = s_pCamera->viewWindow.x;
    view.viewWindowY = s_pCamera->viewWindow.y;
    view.pFrameMatrix =
        reinterpret_cast<const CCMatrix *>(s_pCameraFrame->getLTM());
    RenderPipeline_SetViewWindowFromCamera(&view);
}

} // namespace

RwBool RenderPlatform_Initialize(RwInt32 width, RwInt32 height) {
    if (width <= 0 || height <= 0) {
        std::fprintf(stderr, "RenderPlatform: invalid raster size %d x %d\n",
                     width, height);
        return FALSE;
    }
    if (RwEngineIsRunning() != TRUE) {
        std::fprintf(stderr, "RenderPlatform: engine not running\n");
        return FALSE;
    }
    if (s_pWorld != nullptr) {
        return TRUE; // idempotent
    }

    // World -- binary physics AABB ±500 (same bound as the
    // gravity/floor plane; Phase 4.1 reverse note).
    rw::BBox bbox;
    bbox.inf = rw::makeV3d(-500.0f, -500.0f, -500.0f);
    bbox.sup = rw::makeV3d(500.0f, 500.0f, 500.0f);
    s_pWorld = rw::World::create(&bbox);
    if (s_pWorld == nullptr) {
        std::fprintf(stderr, "RenderPlatform: World::create failed\n");
        return FALSE;
    }

    // Camera -- perspective; librw default near/far (0.05/10) is too narrow
    // for the game -> game values (near 0.1, far 5000).
    s_pCamera = rw::Camera::create();
    s_pCameraFrame = rw::Frame::create();
    if (s_pCamera == nullptr || s_pCameraFrame == nullptr) {
        std::fprintf(stderr, "RenderPlatform: Camera/Frame create failed\n");
        RenderPlatform_Terminate();
        return FALSE;
    }
    s_pCamera->setFrame(s_pCameraFrame);
    s_pCamera->setNearPlane(0.1f);
    s_pCamera->setFarPlane(5000.0f);
    // viewWindow (half size) 1.0 -- while trying 0.6, GL3 frustum culling
    // culled all atomics (empty frame); 1.0 until culling is resolved.
    const rw::V2d kViewWindow = rw::makeV2d(1.0f, 1.0f);
    s_pCamera->setViewWindow(&kViewWindow);
    s_pCamera->setProjection(rw::Camera::PERSPECTIVE);
#if defined(RW_GL3) || defined(RW_D3D9)
    // GL3/D3D9: real framebuffer (librw skeleton CameraCreate pattern --
    // skeleton/skeleton.cpp:90 Raster::CAMERA/ZBUFFER) -- ShowRaster copies
    // this raster to the window. NULL platform has no raster (no-op).
    // D3D9 NOTE: without these rasters, clear()/beginUpdate() resolve a null
    // frame -> 0xC0000005 (2026-10-15).
#if defined(RW_D3D9)
        // D3D9: CAMERATEXTURE raster (CAMERA texture stays nil -- D3D9's
        // RT-bind edge case; see EndFrame screenshot path). GL3/NULL:
        // stays CAMERA.
        s_pCamera->frameBuffer =
            rw::Raster::create(width, height, 0, rw::Raster::CAMERATEXTURE);
#else
        s_pCamera->frameBuffer =
            rw::Raster::create(width, height, 0, rw::Raster::CAMERA);
#endif
    s_pCamera->zBuffer =
        rw::Raster::create(width, height, 0, rw::Raster::ZBUFFER);
    if (s_pCamera->frameBuffer == nullptr ||
        s_pCamera->zBuffer == nullptr) {
        std::fprintf(stderr, "RenderPlatform: camera raster create failed\n");
        RenderPlatform_Terminate();
        return FALSE;
    }
#endif
    s_pWorld->addCamera(s_pCamera);

    // Lights -- ambient + directional (for shading in GL3; no-op on NULL
    // platform). Direction: identity frame -> -Z (RW directional
    // at vector).
    s_pAmbient = rw::Light::create(rw::Light::AMBIENT);
    s_pDirectional = rw::Light::create(rw::Light::DIRECTIONAL);
    if (s_pAmbient != nullptr && s_pDirectional != nullptr) {
        s_pAmbient->setColor(0.35f, 0.35f, 0.35f);
        s_pDirectional->setColor(0.85f, 0.85f, 0.85f);
        s_pLightFrame = rw::Frame::create();
        if (s_pLightFrame != nullptr) {
            s_pDirectional->setFrame(s_pLightFrame);
        }
        s_pWorld->addLight(s_pAmbient);
        s_pWorld->addLight(s_pDirectional);
        s_lightsAdded = TRUE;
    }

    s_width = width;
    s_height = height;
    return TRUE;
}

static rw::TexDictionary *s_worldDict = nullptr;

RwBool RenderPlatform_IsInitialized(void) {
    return (s_pWorld != nullptr && s_pCamera != nullptr) ? TRUE : FALSE;
}

RwBool RenderPlatform_GetScreenSize(RwInt32 *width, RwInt32 *height) {
    if (width == nullptr || height == nullptr) {
        return FALSE;
    }
    *width = s_width;
    *height = s_height;
    return (s_width > 0 && s_height > 0) ? TRUE : FALSE;
}

void *RenderPlatform_GetWorld(void) { return s_pWorld; }

void *RenderPlatform_GetCamera(void) { return s_pCamera; }

RwBool RenderPlatform_BeginFrame(const RwRGBA *pClearCol) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return FALSE;
    }

    // Clear -- IMAGE|Z (binary render pass; NULL platform no-op).
    static rw::RGBA s_defaultClear = {32, 48, 64, 255}; // light sea blue
    rw::RGBA *pCol = pClearCol
                         ? reinterpret_cast<rw::RGBA *>(const_cast<RwRGBA *>(pClearCol))
                         : &s_defaultClear;
    s_pCamera->clear(pCol, rw::Camera::CLEARIMAGE | rw::Camera::CLEARZ);

    // BeginUpdate — world sync + viewMatrix (cameraSync; NULL no-op).
    s_pCamera->beginUpdate();

    // RenderPipeline context -- binary render pass FUN_004cc1f0 +
    // FUN_004cc020 order.
    RenderPipeline_Reset();
    ApplyCameraToPipeline();
    // Fog off (suspicion that distant terrain darkens -- diagnosis).
    rw::SetRenderState(rw::FOGENABLE, 0);
    return TRUE;
}

void RenderPlatform_EndFrame(void) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    s_pCamera->endUpdate();
#if defined(RW_D3D9)
    // D3D9: scene renders into the camera texture RT (CAMERATEXTURE
    // raster) — copy it into the backbuffer so showRaster's Present()
    // shows it. SWAPEFFECT_DISCARD backbuffer contents are undefined,
    // so without this the window stays black.
    {
        IDirect3DDevice9 *pDev = rw::d3d::d3ddevice;
        if (pDev != nullptr) {
            rw::d3d::D3dRaster *pNat = GETD3DRASTEREXT(s_pCamera->frameBuffer);
            if (pNat != nullptr && pNat->texture != nullptr) {
                IDirect3DTexture9 *pTex = (IDirect3DTexture9 *)pNat->texture;
                IDirect3DSurface9 *pRT = nullptr;
                if (SUCCEEDED(pTex->GetSurfaceLevel(0, &pRT))) {
                    IDirect3DSurface9 *pBack = nullptr;
                    if (SUCCEEDED(pDev->GetBackBuffer(0, 0,
                                                      D3DBACKBUFFER_TYPE_MONO,
                                                      &pBack))) {
                        pDev->StretchRect(pRT, nullptr, pBack, nullptr,
                                          D3DTEXF_LINEAR);
                        pBack->Release();
                    }
                    pRT->Release();
                }
            }
        }
    }
#endif
    // D3D9: backbuffer -> systemmem surface -> RGBA -> PNG.
    if (s_screenshotPath[0] != '\0' && s_pCamera->frameBuffer != nullptr) {
#ifdef RW_GL3
        // Screenshot -- GL3 framebuffer -> PNG.
        rw::Image *pShot = s_pCamera->frameBuffer->toImage();
        if (pShot != nullptr) {
            rw::writePNG(pShot, s_screenshotPath);
            pShot->destroy();
            std::printf("Screenshot: %s\n", s_screenshotPath);
        }
#elif defined(RW_D3D9)
        // The CAMERA raster is a CAMERATEXTURE so it has a surface; copy RT
        // to a systemmem surface and make a PNG (backbuffer DISCARD is unreliable).
        IDirect3DDevice9 *pDev = rw::d3d::d3ddevice;
        rw::d3d::D3dRaster *pNat = pDev ? GETD3DRASTEREXT(s_pCamera->frameBuffer) : nullptr;
        if (pDev == nullptr || pNat == nullptr || pNat->texture == nullptr) {
            std::fprintf(stderr, "Screenshot: d3ddevice/texture null\n");
        } else {
            IDirect3DTexture9 *pTex = (IDirect3DTexture9 *)pNat->texture;
            D3DSURFACE_DESC desc;
            if (FAILED(pTex->GetLevelDesc(0, &desc))) {
                std::fprintf(stderr, "Screenshot: GetLevelDesc failed\n");
            } else if (desc.Format != D3DFMT_X8R8G8B8 &&
                       desc.Format != D3DFMT_A8R8G8B8) {
                std::fprintf(stderr,
                             "Screenshot: unsupported format 0x%x\n",
                             (unsigned)desc.Format);
            } else {
                IDirect3DSurface9 *pRT = nullptr;
                if (SUCCEEDED(pTex->GetSurfaceLevel(0, &pRT))) {
                    IDirect3DSurface9 *pSys = nullptr;
                    if (SUCCEEDED(pDev->CreateOffscreenPlainSurface(
                            desc.Width, desc.Height, desc.Format,
                            D3DPOOL_SYSTEMMEM, &pSys, nullptr))) {
                        if (SUCCEEDED(pDev->GetRenderTargetData(pRT, pSys))) {
                            D3DLOCKED_RECT lr;
                            if (SUCCEEDED(pSys->LockRect(&lr, nullptr,
                                                          D3DLOCK_READONLY))) {
                                const int w = (int)desc.Width;
                                const int h = (int)desc.Height;
                                rw::Image *pImg = rw::Image::create(w, h, 32);
                                if (pImg != nullptr) {
                                    pImg->allocate();
                                    for (int y = 0; y < h; ++y) {
                                        const RwUInt8 *src =
                                            (const RwUInt8 *)lr.pBits +
                                            (size_t)y * (size_t)lr.Pitch;
                                        RwUInt8 *dst =
                                            pImg->pixels +
                                            (size_t)y * (size_t)pImg->stride;
                                        for (int x = 0; x < w; ++x) {
                                            dst[x * 4 + 0] = src[x * 4 + 2];
                                            dst[x * 4 + 1] = src[x * 4 + 1];
                                            dst[x * 4 + 2] = src[x * 4 + 0];
                                            dst[x * 4 + 3] = 255;
                                        }
                                    }
                                    rw::writePNG(pImg, s_screenshotPath);
                                    std::printf("Screenshot: %s\n",
                                                s_screenshotPath);
                                    pImg->destroy();
                                }
                                pSys->UnlockRect();
                            }
                        }
                        pSys->Release();
                    }
                    pRT->Release();
                }
            }
        }
#endif
        s_screenshotPath[0] = '\0'; // one-time
    }
    if (s_pCamera->frameBuffer != nullptr) {
        s_pCamera->showRaster(0);
    }
    // GL3: GLFW events (ESC/close) -- main loop ends with ShouldClose.
    RwPlatformPollEvents();
}

void RenderPlatform_RequestScreenshot(const char *path) {
    if (path == nullptr) {
        s_screenshotPath[0] = '\0';
        return;
    }
    std::snprintf(s_screenshotPath, sizeof(s_screenshotPath), "%s", path);
}

RwBool RenderPlatform_RenderTestQuad(RwReal cx, RwReal cy,
                                     RwReal halfW, RwReal halfH,
                                     RwUInt32 argb) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return FALSE;
    }

    // Top matrix -- ScaleXY + TranslateXY (RenderPipeline stack; FUN_
    // 004cc480 / FUN_004cc4d0). Corner positions are 2D-affine multiplied
    // with this matrix (view-window space -> screen space bridge; in the GL3
    // pass, the full BuildViewMatrix path).
    RenderPipeline_MatrixIdentity();
    RenderPipeline_MatrixScaleXY(halfW * 2.0f, halfH * 2.0f);
    RenderPipeline_MatrixTranslateXY(cx, cy);

    const CCMatrix *pTop = RenderPipeline_GetTopMatrix();
    if (pTop == nullptr) {
        return FALSE;
    }

    const RwReal z = rw::im2d::GetNearZ();
    const RwReal rhw = 1.0f;
    const RwUInt8 r = static_cast<RwUInt8>((argb >> 16) & 0xFF);
    const RwUInt8 g = static_cast<RwUInt8>((argb >> 8) & 0xFF);
    const RwUInt8 b = static_cast<RwUInt8>(argb & 0xFF);
    const RwUInt8 a = static_cast<RwUInt8>((argb >> 24) & 0xFF);

    // Unit square (-0.5..+0.5) -- with scale (halfW×2) + translate (cx,cy)
    // on the top matrix; transform: x' = right·x + up·y + pos.
    const RwReal kCorners[4][2] = {{-0.5f, -0.5f},
                                   {0.5f, -0.5f},
                                   {-0.5f, 0.5f},
                                   {0.5f, 0.5f}};
    RwIm2DVertex verts[4];
    for (int i = 0; i < 4; ++i) {
        const RwReal lx = kCorners[i][0];
        const RwReal ly = kCorners[i][1];
        verts[i].x = pTop->right[0] * lx + pTop->up[0] * ly + pTop->pos[0];
        verts[i].y = pTop->right[1] * lx + pTop->up[1] * ly + pTop->pos[1];
        verts[i].z = z;
        verts[i].rhw = rhw;
#if defined(RW_D3D9)
        verts[i].color.red = b;
        verts[i].color.green = g;
        verts[i].color.blue = r;
#else
        verts[i].color.red = r;
        verts[i].color.green = g;
        verts[i].color.blue = b;
#endif
        verts[i].color.alpha = a;
        verts[i].u = (lx + 0.5f);
        verts[i].v = (ly + 0.5f);
    }

    // Two triangles (TRILIST + index) -- NULL platform no-op driver;
    // direct rw::im2d (does not fall into the rwstubs stub C API no-ops).
    RwUInt16 indices[6] = {0, 1, 2, 2, 1, 3};
    rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, verts, 4,
                                     indices, 6);
    return TRUE;
}

void RenderPlatform_Terminate(void) {
    if (s_pWorld != nullptr) {
        if (s_pCamera != nullptr) {
            s_pWorld->removeCamera(s_pCamera);
#if defined(RW_GL3) || defined(RW_D3D9)
            // Rasters are freed before the camera (skeleton CameraDestroy).
            if (s_pCamera->frameBuffer != nullptr) {
                s_pCamera->frameBuffer->destroy();
                s_pCamera->frameBuffer = nullptr;
            }
            if (s_pCamera->zBuffer != nullptr) {
                s_pCamera->zBuffer->destroy();
                s_pCamera->zBuffer = nullptr;
            }
#endif
            s_pCamera->destroy();
            s_pCamera = nullptr;
        }
        if (s_pAmbient != nullptr) {
            // Only an added light is removed (an LLLink that was not added
            // SEGVs in removeLight -- NOLIGHTS crash lesson).
            if (s_lightsAdded == TRUE) {
                s_pWorld->removeLight(s_pAmbient);
            }
            s_pAmbient->destroy();
            s_pAmbient = nullptr;
        }
        if (s_pDirectional != nullptr) {
            if (s_lightsAdded == TRUE) {
                s_pWorld->removeLight(s_pDirectional);
            }
            s_pDirectional->destroy();
            s_pDirectional = nullptr;
        }
        s_lightsAdded = FALSE;
        s_pWorld->destroy();
        s_pWorld = nullptr;
    }
    if (s_pCameraFrame != nullptr) {
        s_pCameraFrame->destroy();
        s_pCameraFrame = nullptr;
    }
    if (s_pLightFrame != nullptr) {
        s_pLightFrame->destroy();
        s_pLightFrame = nullptr;
    }
    RenderPlatform_ClearTextures();
    RenderPlatform_ClearWorldDictionary();
    s_width = 0;
    s_height = 0;
}

RwBool RenderPlatform_AddClump(void *pClump) {
    if (RenderPlatform_IsInitialized() != TRUE || pClump == nullptr) {
        return FALSE;
    }
    s_pWorld->addClump(static_cast<rw::Clump *>(pClump));
    return TRUE;
}

void RenderPlatform_RenderScene(void) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    s_pWorld->render();
}

// ---------------------------------------------------------------------------
// Im2D overlay -- pixel-coordinate 2D (CShowText::Render pattern).
// The 3D scene writes depth, so ZTEST must be off or Im2D gets culled.
// librw C++ API (rw::SetRenderState) -- the C stub RwRenderStateSet does not exist in GL3.
// ---------------------------------------------------------------------------
void RenderPlatform_BeginOverlay2D(void) {
    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
    rw::SetRenderState(rw::ZTESTENABLE, 0);
    rw::SetRenderState(rw::ZWRITEENABLE, 0);
    rw::SetRenderState(rw::VERTEXALPHA, 1);
    rw::SetRenderState(rw::SRCBLEND, rw::BLENDSRCALPHA);
    rw::SetRenderState(rw::DESTBLEND, rw::BLENDINVSRCALPHA);
    // Darkens the mipmap-filtered textured Im2D left from the 3D scene
    // ("incomplete texture"); mipmap-less linear + clamp in the overlay.
    rw::SetRenderState(rw::TEXTUREFILTER, rw::Texture::LINEAR);
    rw::SetRenderState(rw::TEXTUREADDRESS, rw::Texture::CLAMP);
}

void RenderPlatform_EndOverlay2D(void) {
    rw::SetRenderState(rw::VERTEXALPHA, 0);
    rw::SetRenderState(rw::ZTESTENABLE, 1);
    rw::SetRenderState(rw::ZWRITEENABLE, 1);
}

void RenderPlatform_DrawOverlayQuad(RwReal x, RwReal y, RwReal w, RwReal h,
                                    RwUInt32 argb) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    const RwReal z = rw::im2d::GetNearZ();
    const RwReal rhw = 1.0f;
    const RwUInt8 r = static_cast<RwUInt8>((argb >> 16) & 0xFF);
    const RwUInt8 g = static_cast<RwUInt8>((argb >> 8) & 0xFF);
    const RwUInt8 b = static_cast<RwUInt8>(argb & 0xFF);
    const RwUInt8 a = static_cast<RwUInt8>((argb >> 24) & 0xFF);

    // Pixel-coordinate quad (CShowText bg pattern): top-left, bottom-left,
    // top-right, bottom-right -- TRILIST + index.
    const RwReal vx[4] = {x, x, x + w, x + w};
    const RwReal vy[4] = {y, y + h, y, y + h};
    RwIm2DVertex verts[4];
    for (int i = 0; i < 4; ++i) {
        verts[i].x = vx[i];
        verts[i].y = vy[i];
        verts[i].z = z;
        verts[i].rhw = rhw;
#if defined(RW_D3D9)
        // D3D9: the RwRGBA byte array (R,G,B,A) is reinterpreted
        // as D3DCOLOR; GL3 reads the bytes directly as R,G,B,A.
        verts[i].color.red = b;
        verts[i].color.green = g;
        verts[i].color.blue = r;
#else
        verts[i].color.red = r;
        verts[i].color.green = g;
        verts[i].color.blue = b;
#endif
        verts[i].color.alpha = a;
        verts[i].u = 0.0f;
        verts[i].v = 0.0f;
    }
    RwUInt16 indices[6] = {0, 1, 2, 2, 1, 3};
    rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, verts, 4,
                                     indices, 6);
}

// ---------------------------------------------------------------------------
// Named UI/procedural textures (RGBA -> librw Texture). The level dictionary
// is managed separately so world material lookup keeps its own lifetime.
// ---------------------------------------------------------------------------
static std::vector<rw::Texture *> s_menuTextures;

static rw::Texture *FindMenuTexture(const char *name)
{
    if (name == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < s_menuTextures.size(); ++i) {
        if (std::strcmp(s_menuTextures[i]->name, name) == 0) {
            return s_menuTextures[i];
        }
    }
    return nullptr;
}

// librw texture from RGBA pixels (Image->Raster->Texture). NULL on failure.
static rw::Texture *CreateTextureFromRGBA(const char *name, RwInt32 width,
                                          RwInt32 height,
                                          const RwUInt8 *rgbaPixels,
                                          RwInt32 pitch)
{
    if (name == nullptr || rgbaPixels == nullptr || width <= 0 ||
        height <= 0 || pitch < width * 4) {
        return nullptr;
    }
    rw::Image *img = rw::Image::create(width, height, 32);
    if (img == nullptr) {
        return nullptr;
    }
    img->allocate();
    if (img->pixels == nullptr) {
        img->destroy();
        return nullptr;
    }
    for (RwInt32 y = 0; y < height; ++y) {
        const RwUInt8 *src = rgbaPixels + (size_t)y * (size_t)pitch;
        RwUInt8 *dst = img->pixels + (size_t)y * (size_t)img->stride;
        for (RwInt32 x = 0; x < width; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 0]; // R (TxdDecode zaten RGBA)
            dst[x * 4 + 1] = src[x * 4 + 1]; // G
            dst[x * 4 + 2] = src[x * 4 + 2]; // B
            dst[x * 4 + 3] = src[x * 4 + 3]; // A
        }
    }
    // Diagnosis: CANNONCRUISE_REDTEX=1 -> all textures red (sampling test).
    if (std::getenv("CANNONCRUISE_REDTEX") != nullptr) {
        for (RwInt32 y = 0; y < height; ++y) {
            RwUInt8 *dst = img->pixels + (size_t)y * (size_t)img->stride;
            for (RwInt32 x = 0; x < width; ++x) {
                dst[x * 4 + 0] = 255;
                dst[x * 4 + 1] = 16;
                dst[x * 4 + 2] = 16;
                dst[x * 4 + 3] = 255;
            }
        }
    }
    rw::Raster *raster = rw::Raster::createFromImage(img, 0);
    img->destroy();
    if (raster == nullptr) {
        return nullptr;
    }
    rw::Texture *tex = rw::Texture::create(raster);
    if (tex == nullptr) {
        raster->destroy();
        return nullptr;
    }
    std::snprintf(tex->name, sizeof(tex->name), "%s", name);
#ifdef RW_GL3
    // Mipmap completion: librw Gl3Raster does not initialize numLevels/autogenMipmap
    // (garbage MIP filter value -> "incomplete texture" = black).
    // If real mips are generated and MAX_LEVEL is clamped, EVERY mip gets complete.
    {
        void *prevTex = rw::GetRenderStatePtr(rw::TEXTURERASTER);
        rw::SetRenderStatePtr(rw::TEXTURERASTER, raster);
        // Generate mipmaps + clamp MAX_LEVEL: EVERY mip gets complete (garbage
        // numLevels/autogenMipmap -> the MIP filter made things black).
        int levels = 1;
        {
            int span = (width > height) ? width : height;
            while (span > 1) {
                span /= 2;
                ++levels;
            }
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Diagnosis: CANNONCRUISE_NEAREST=1 -> NEAREST sampling.
        if (std::getenv("CANNONCRUISE_NEAREST") != nullptr) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            GL_NEAREST);
        }
        rw::SetRenderStatePtr(rw::TEXTURERASTER, prevTex);
    }
#endif
    return tex;
}

RwBool RenderPlatform_UploadTexture(const char *name, RwInt32 width,
                                    RwInt32 height, const RwUInt8 *rgbaPixels,
                                    RwInt32 pitch)
{
    if (name == nullptr || rgbaPixels == nullptr || width <= 0 ||
        height <= 0 || pitch < width * 4) {
        return FALSE;
    }
    rw::Texture *old = FindMenuTexture(name);
    if (old != nullptr) {
        for (size_t i = 0; i < s_menuTextures.size(); ++i) {
            if (s_menuTextures[i] == old) {
                s_menuTextures.erase(s_menuTextures.begin() +
                                     (ptrdiff_t)i);
                break;
            }
        }
        old->destroy();
    }
    rw::Texture *tex =
        CreateTextureFromRGBA(name, width, height, rgbaPixels, pitch);
    if (tex == nullptr) {
        return FALSE;
    }
    // 2D UI: no mipmap, linear + clamp (asset filterFlags ignored --
    // a missing mip level above the shelf makes "incomplete texture" = black).
    tex->setFilter(rw::Texture::LINEAR);
    tex->setAddressU(rw::Texture::CLAMP);
    tex->setAddressV(rw::Texture::CLAMP);
    s_menuTextures.push_back(tex);
    return TRUE;
}

RwBool RenderPlatform_HasTexture(const char *name)
{
    return (RenderPlatform_GetTexture(name) != nullptr) ? TRUE : FALSE;
}

void *RenderPlatform_GetTexture(const char *name)
{
    if (name == nullptr) {
        return nullptr;
    }
    rw::Texture *texture = FindMenuTexture(name);
    if (texture == nullptr && s_worldDict != nullptr) {
        texture = s_worldDict->find(name);
    }
    return texture;
}

void RenderPlatform_DrawTexturedQuad(RwReal x, RwReal y, RwReal w, RwReal h,
                                     const char *texName)
{
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    rw::Texture *tex = FindMenuTexture(texName);
    if (tex == nullptr || tex->raster == nullptr) {
        return;
    }
    rw::SetRenderStatePtr(rw::TEXTURERASTER, tex->raster);
    const RwReal z = rw::im2d::GetNearZ();
    RwIm2DVertex verts[4];
    const RwReal vx[4] = {x, x, x + w, x + w};
    const RwReal vy[4] = {y, y + h, y, y + h};
    const RwReal vu[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    const RwReal vv[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    for (int i = 0; i < 4; ++i) {
        verts[i].x = vx[i];
        verts[i].y = vy[i];
        verts[i].z = z;
        verts[i].rhw = 1.0f;
        verts[i].color.red = 255;
        verts[i].color.green = 255;
        verts[i].color.blue = 255;
        verts[i].color.alpha = 255;
        verts[i].u = vu[i];
        verts[i].v = vv[i];
    }
    RwUInt16 indices[6] = {0, 1, 2, 2, 1, 3};
    rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST, verts, 4,
                                     indices, 6);
    rw::SetRenderStatePtr(rw::TEXTURERASTER, nullptr);
}

void RenderPlatform_ClearTextures(void)
{
    for (size_t i = 0; i < s_menuTextures.size(); ++i) {
        s_menuTextures[i]->destroy();
    }
    s_menuTextures.clear();
}

// ---------------------------------------------------------------------------
// Level TexDictionary (3D geometry materials resolve by name at stream time).
// Lifetime: modify/destroy AFTER pipeline unload (materials hold raw pointers).
// ---------------------------------------------------------------------------
RwBool RenderPlatform_LoadWorldDictionary(const char *rwsPath)
{
    if (rwsPath == nullptr) {
        return FALSE;
    }
    FILE *f = std::fopen(rwsPath, "rb");
    if (f == nullptr) {
        return FALSE;
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    RwBool any = FALSE;
    rw::TexDictionary *dict = nullptr;
    if (size > 12 + 8) {
        RwUInt8 *buf = static_cast<RwUInt8 *>(std::malloc((size_t)size));
        if (buf != nullptr &&
            std::fread(buf, 1, (size_t)size, f) == (size_t)size) {
            // Level texture pack: the first 0x716 chunk (custom record format;
            // different from the menu d3d8-IMAGE).
            RwUInt32 c0len = 0;
            std::memcpy(&c0len, buf + 4, 4);
            RwUInt32 scanLen = (RwUInt32)size;
            if (c0len > 12 && 12 + c0len <= (RwUInt32)size) {
                scanLen = 12 + c0len;
            }
            // UI texture pack (GUI_/compass_/fishbasic -- nested d3d8-IMAGE):
            // HUD textures are found in the tail of level chunk0; LevelPack
            // cannot see them. Load as menu textures (HUD draws).
            {
                RwTxdImage *pGui = static_cast<RwTxdImage *>(
                    std::malloc(sizeof(RwTxdImage)));
                if (pGui != nullptr) {
                    if (RwTxdDecodeGuiPack(buf, scanLen, pGui) == TRUE) {
                        for (RwUInt32 i = 0; i < pGui->count; ++i) {
                            const RwTxdTexture *t = &pGui->textures[i];
                            if (t->pPixels == nullptr || t->width == 0 ||
                                t->height == 0) {
                                continue;
                            }
                            RenderPlatform_UploadTexture(
                                t->name, (RwInt32)t->width,
                                (RwInt32)t->height, t->pPixels,
                                (RwInt32)t->pitch);
                        }
                        std::printf("WorldDict: %s: %u UI textures\n",
                                    rwsPath, pGui->count);
                    }
                    std::free(pGui);
                }
            }
            RwTxdImage *pImage = static_cast<RwTxdImage *>(
                std::malloc(sizeof(RwTxdImage)));
            if (pImage != nullptr) {
                if (RwTxdDecodeLevelPack(buf, scanLen, pImage) == TRUE &&
                    pImage->count > 0) {
                    std::printf("WorldDict: %s: %u textures\n", rwsPath,
                                pImage->count);
                    dict = rw::TexDictionary::create();
                    if (dict != nullptr) {
                        for (RwUInt32 i = 0; i < pImage->count; ++i) {
                            const RwTxdTexture *t = &pImage->textures[i];
                            if (t->pPixels == nullptr || t->width == 0 ||
                                t->height == 0) {
                                continue;
                            }
                            rw::Texture *tex = CreateTextureFromRGBA(
                                t->name, (RwInt32)t->width,
                                (RwInt32)t->height, t->pPixels,
                                (RwInt32)t->pitch);
                            if (tex == nullptr) {
                                continue;
                            }
                            // Terrain tiling: wrap + linear (no mipmap).
                            tex->setFilter(rw::Texture::LINEAR);
                            tex->setAddressU(rw::Texture::WRAP);
                            tex->setAddressV(rw::Texture::WRAP);
                            dict->add(tex);
                            any = TRUE;
                        }
                    }
                }
                std::free(pImage);
            }
            std::free(buf);
        }
    }
    std::fclose(f);
    if (any == FALSE) {
        if (dict != nullptr) {
            dict->destroy();
        }
        return FALSE;
    }
    if (s_worldDict != nullptr) {
        s_worldDict->destroy();
    }
    s_worldDict = dict;
    rw::TexDictionary::setCurrent(s_worldDict);
#ifdef RW_GL3
    // Diagnosis: CANNONCRUISE_DUMPTEX=name -> writes GL content as PPM.
    {
        const char *dumpName = std::getenv("CANNONCRUISE_DUMPTEX");
        if (dumpName != nullptr) {
            rw::Texture *dt = s_worldDict->find(dumpName);
            if (dt != nullptr && dt->raster != nullptr) {
                void *prevTex =
                    rw::GetRenderStatePtr(rw::TEXTURERASTER);
                rw::SetRenderStatePtr(rw::TEXTURERASTER, dt->raster);
                const int dw = dt->raster->width;
                const int dh = dt->raster->height;
                RwUInt8 *dpix = static_cast<RwUInt8 *>(
                    std::malloc((size_t)dw * dh * 4));
                if (dpix != nullptr) {
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA,
                                  GL_UNSIGNED_BYTE, dpix);
                    FILE *o = std::fopen("/tmp/gltex.ppm", "wb");
                    if (o != nullptr) {
                        std::fprintf(o, "P6\n%d %d\n255\n", dw, dh);
                        for (int yy = dh - 1; yy >= 0; --yy) {
                            for (int xx = 0; xx < dw; ++xx) {
                                RwUInt8 *p =
                                    dpix + ((size_t)yy * dw + xx) * 4;
                                std::fputc(p[0], o);
                                std::fputc(p[1], o);
                                std::fputc(p[2], o);
                            }
                        }
                        std::fclose(o);
                        std::printf("DumpTex: %s %dx%d -> /tmp/gltex.ppm\n",
                                    dumpName, dw, dh);
                    }
                    std::free(dpix);
                }
                rw::SetRenderStatePtr(rw::TEXTURERASTER, prevTex);
            } else {
                std::printf("DumpTex: '%s' not found\n", dumpName);
            }
        }
    }
    // Filter state readback (MIN/MAG/MAX) -- independent of DUMPTEX.
    {
        const char *qName = std::getenv("CANNONCRUISE_TEXQUERY");
        if (qName != nullptr && s_worldDict != nullptr) {
            rw::Texture *qt = s_worldDict->find(qName);
            void *pq = rw::GetRenderStatePtr(rw::TEXTURERASTER);
            rw::SetRenderStatePtr(rw::TEXTURERASTER,
                                  qt != nullptr ? qt->raster : nullptr);
            GLint mn = 0, mg = 0, mx = 0;
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                &mn);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                &mg);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,
                                &mx);
            std::printf("TexQuery %s: MIN=0x%x MAG=0x%x MAX=%d\n", qName,
                        mn, mg, mx);
            rw::SetRenderStatePtr(rw::TEXTURERASTER, pq);
        }
    }
#endif
    return TRUE;
}

void RenderPlatform_ClearWorldDictionary(void)
{
    if (s_worldDict != nullptr) {
        s_worldDict->destroy();
        s_worldDict = nullptr;
    }
}

void RenderPlatform_SetCameraPose(const RwReal position[3],
                                  const RwReal lookAt[3]) {
    if (RenderPlatform_IsInitialized() != TRUE) {
        return;
    }
    rw::Matrix *pMat = &s_pCameraFrame->matrix;

    // at = normalize(lookAt - pos); right = normalize(cross(worldUp, at));
    // up = cross(at, right) -- orthonormal RW frame (CCMatrix layout).
    rw::V3d at = rw::makeV3d(lookAt[0] - position[0],
                             lookAt[1] - position[1],
                             lookAt[2] - position[2]);
    const float lenAt = rw::length(at);
    if (lenAt > 1e-6f) {
        at = rw::normalize(at);
    } else {
        at = rw::makeV3d(0.0f, 0.0f, -1.0f);
    }
    rw::V3d worldUp = rw::makeV3d(0.0f, 1.0f, 0.0f);
    rw::V3d right = rw::cross(worldUp, at);
    const float lenRight = rw::length(right);
    if (lenRight > 1e-6f) {
        right = rw::normalize(right);
    } else {
        // at ∥ worldUp -- the x axis is taken as the basis.
        right = rw::makeV3d(1.0f, 0.0f, 0.0f);
    }
    const rw::V3d up = rw::cross(at, right);

    pMat->right = right;
    pMat->up = up;
    pMat->at = at;
    pMat->pos = rw::makeV3d(position[0], position[1], position[2]);
    pMat->update();
    s_pCameraFrame->updateObjects();
}
