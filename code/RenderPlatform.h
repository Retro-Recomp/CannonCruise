#ifndef CANNONCRUISE_RENDERPLATFORM_H
#define CANNONCRUISE_RENDERPLATFORM_H

// ============================================================================
// CannonCruise - Render Platform (RenderPlatform.h)
// Phase 4.6n: librw-based scene platform -- while the engine is running it builds the world +
// camera + light; the frame begin/end chain (clear -> BeginUpdate ->
// application drawing -> EndUpdate -> ShowRaster) and an Im2D test quad.
//
// Connection: librw C++ API (rw::World/rw::Camera/rw::Light -- like rw_librw.cpp).
// The stub C API (rwstubs.cpp) is NOT used -- the camera/world functions
// there are no-ops. All device calls are no-ops on the NULL platform
// (headless smoke); the GL3 platform produces real pixels (ShowRaster,
// skipped if frameBuffer is nil).
//
// Binary counterpart of the frame loop: sub_4189E0 (MainLoop) / FUN_0041C380
// (Idle) / sub_41CEB0 (Render -- double Begin/EndUpdate wrapping).
// ============================================================================

#include "rwcore.h"

/**
 * @brief Sets up the platform (engine Init/Open/Start MUST HAVE BEEN CALLED).
 *
 * World (BBox ±500 -- binary physics AABB), perspective camera (near 0.1,
 * far 5000, viewWindow 1x1), creates ambient + directional lights and
 * adds them to the world. A repeated call returns TRUE (idempotent).
 *
 * @param width  Raster width (informational -- RCCCameraView raster
 *               fields; real framebuffer size in GL3).
 * @param height Raster height.
 * @return TRUE set up/already set up; FALSE engine not running or
 *         allocation error.
 */
RwBool RenderPlatform_Initialize(RwInt32 width, RwInt32 height);

/// Is the platform set up (single world/camera)?
RwBool RenderPlatform_IsInitialized(void);

/// Gets the initialized raster dimensions in pixels.
/// Returns FALSE and writes zeroes when dimensions are unavailable.
RwBool RenderPlatform_GetScreenSize(RwInt32 *width, RwInt32 *height);

/// World (valid if not NULL).
void *RenderPlatform_GetWorld(void);

/// Camera (valid if not NULL).
void *RenderPlatform_GetCamera(void);

/**
 * @brief Frame begin: clear (IMAGE|Z) -> CameraBeginUpdate ->
 * RenderPipeline_Reset + SetViewWindowFromCamera (camera viewWindow +
 * frame LTM -- RCCCameraView schema).
 *
 * @param pClearCol Clear color (NULL -> opaque black).
 * @return TRUE frame started; FALSE platform not set up.
 */
RwBool RenderPlatform_BeginFrame(const RwRGBA *pClearCol);

/// Frame end: CameraEndUpdate -> (if frameBuffer exists) ShowRaster.
void RenderPlatform_EndFrame(void);

/**
 * @brief Im2D test quad -- drawn by applying the RenderPipeline matrix
 * stack's top matrix (ScaleXY + TranslateXY) to a screen-space quad.
 *
 * Vertex layout: RwIm2DVertex (x,y,z,rhw,RGBA,u,v) — gl3::Im2DVertex
 * same byte layout (compatible with the GL3 transition). z = im2d::GetNearZ.
 *
 * @param cx,cy  Center (view-window units -- before the top matrix).
 * @param halfW,halfH Half size (view-window units).
 * @param argb   Draw color (0xAARRGGBB).
 * @return TRUE drawn; FALSE platform not set up.
 */
RwBool RenderPlatform_RenderTestQuad(RwReal cx, RwReal cy,
                                     RwReal halfW, RwReal halfH,
                                     RwUInt32 argb);

/**
 * @brief Destroys the scene (camera/light/world destroy). Engine
 * lifecycle is NOT OWNED -- the caller (test/main) does stop/close/term.
 * A repeated call is safe.
 */
void RenderPlatform_Terminate(void);

/**
 * @brief Camera placement (trial/visual verification -- Phase 4.6 visual).
 *
 * Builds the camera frame matrix orthonormal (at/right/up) and syncs
 * the LTM via updateObjects. No-op if the platform is not set up.
 *
 * @param position Camera position (3 float).
 * @param lookAt   Target point (3 float -- the at vector looks here).
 */
void RenderPlatform_SetCameraPose(const RwReal position[3],
                                  const RwReal lookAt[3]);

/** @brief Projects a world point to screen pixels with the current camera
 *         pose; FALSE when it is behind the camera. */
RwBool RenderPlatform_WorldToScreen(const RwReal p[3], RwReal *pX, RwReal *pY);

/**
 * @brief Adds the clump to the platform world (render + world ownership
 * at the ATOMIC/CLUMP level -- clump destroy is the caller's job).
 *
 * Phase 4.6o: the WorldGeometry_CreateClump output is wired in here.
 *
 * @param pClump rw::Clump* (NULL → FALSE).
 * @return TRUE added; FALSE platform not set up or NULL.
 */
RwBool RenderPlatform_AddClump(void *pClump);

/// Draws a clump that is not in the world right now (between Begin/EndFrame),
/// e.g. the player ship after the ocean so the water cannot paint over it.
RwBool RenderPlatform_RenderClump(void *pClump);

/// Draws a sky layer clump with depth test/write off (call before the scene).
RwBool RenderPlatform_RenderBackgroundClump(void *pClump);

/// Removes a clump added with RenderPlatform_AddClump (does not destroy it).
RwBool RenderPlatform_RemoveClump(void *pClump);

/**
 * @brief World render -- called BETWEEN BeginFrame and EndFrame
 * (world->render(); NULL platform no-op).
 */
void RenderPlatform_RenderScene(void);

/**
 * @brief Start Im2D overlay pass -- prepares render states for 2D
 *        (no texture, Z test/write off, vertex alpha + srcalpha blend).
 *        Called BETWEEN BeginFrame and EndFrame, BEFORE Im2D drawing;
 *        if the 3D scene wrote depth, ZTEST would cull the Im2D quads.
 */
void RenderPlatform_BeginOverlay2D(void);

/**
 * @brief Finish Im2D overlay pass -- restores the 3D render states.
 */
void RenderPlatform_EndOverlay2D(void);

/**
 * @brief Filled Im2D quad in pixel coordinates (screen space; CShowText pattern).
 *        x,y top-left corner; w,h pixels; argb color. Takes the screen
 *        size from the camera raster. Called after BeginOverlay2D.
 */
void RenderPlatform_DrawOverlayQuad(RwReal x, RwReal y, RwReal w, RwReal h,
                                    RwUInt32 argb);

/**
 * @brief Registers the framebuffer to be saved as a PNG on the next EndFrame
 *        (before swap; GL3). One-time -- cleared after writing.
 *        No-op on the headless/NULL platform. path=nullptr cancels the request.
 */
void RenderPlatform_RequestScreenshot(const char *path);

/**
 * @brief Uploads RGBA pixels as a named librw texture. If the same menu texture
 *        exists, it is replaced. No-op on NULL.
 * @return TRUE (loaded).
 */
RwBool RenderPlatform_UploadTexture(const char *name, RwInt32 width,
                                    RwInt32 height, const RwUInt8 *rgbaPixels,
                                    RwInt32 pitch);

/** @brief Is a menu or current-level texture loaded (by name)? */
RwBool RenderPlatform_HasTexture(const char *name);

/**
 * @brief Returns a menu or current-level texture as an opaque rw::Texture*.
 *        For 3D materials (e.g. placeholder ship). nullptr if absent.
 *        nullptr on NULL. Valid until the owning texture dictionary is cleared.
 */
void *RenderPlatform_GetTexture(const char *name);

/**
 * @brief Textured Im2D quad (UV 0..1; white tint). Called after BeginOverlay2D;
 *        sets TEXTURERASTER then clears it. Does not draw if the texture is missing.
 */
void RenderPlatform_DrawTexturedQuad(RwReal x, RwReal y, RwReal w, RwReal h,
                                     const char *texName);

/** @brief Textured disc (menu or level texture): centre/radius in pixels, the
 *         texture sampled around (uc, vc) with radius uvRadius. Minimap. */
void RenderPlatform_DrawTextureDisc(const char *texName, RwReal cx, RwReal cy, RwReal radius,
                                    RwReal uc, RwReal vc, RwReal uvRadius, RwUInt32 argb);

/** @brief Texture quad centred at (cx, cy), rotated by angle (radians, clockwise
 *         on screen). Minimap ship arrow. */
void RenderPlatform_DrawRotatedSprite(const char *texName, RwReal cx, RwReal cy, RwReal w,
                                      RwReal h, RwReal angle, RwUInt32 argb);

/** @brief Textured quad of a loaded menu texture, or a solid quad in
 *         fallbackArgb when the texture is missing. After BeginOverlay2D. */
void RenderPlatform_DrawSprite(const char *texName, RwReal x, RwReal y, RwReal w,
                               RwReal h, RwUInt32 fallbackArgb);

/**
 * @brief Draws a UV sub-rectangle of a texture (rw::Texture*) tinted with argb
 *        (0xAARRGGBB). Called after BeginOverlay2D (font glyphs).
 */
void RenderPlatform_DrawTextureRegion(void *pTexture, RwReal x, RwReal y, RwReal w,
                                      RwReal h, RwReal u0, RwReal v0, RwReal u1,
                                      RwReal v1, RwUInt32 argb);

/** @brief Destroys all loaded textures (menu teardown/terminate). */
void RenderPlatform_ClearTextures(void);

/**
 * @brief Sets up the level TexDictionary (0xe8 txd -> librw dict + current).
 *        Call BEFORE RwsPipeline_Load -- geometry materials resolve at
 *        stream time by name. The old dict is destroyed if present (call
 *        AFTER pipeline unload: materials hold raw pointers).
 * @return TRUE (dict set up; at least one texture).
 */
RwBool RenderPlatform_LoadWorldDictionary(const char *rwsPath);

/** @brief Destroys the world dict (after pipeline unload/terminate). */
void RenderPlatform_ClearWorldDictionary(void);

#endif // CANNONCRUISE_RENDERPLATFORM_H
