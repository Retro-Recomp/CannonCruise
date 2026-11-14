// ============================================================================
// CannonCruise - Render Pipeline state (RenderPipeline.h)
// Original path: D:\Projects\CannonCruisePC\code\RenderPipeline.h
// Reconstructed via reverse engineering with CC_ida.asm analysis.
//
// Binary functions:
//   FUN_004cc1f0 — RenderPipeline_Reset       (Im2D/matrix state reset)
//   FUN_004cbd20 — RenderPipeline_MatrixIdentity (make the top matrix identity)
//   FUN_004cc2f0 — RenderPipeline_MatrixPush  (push matrix stack)
//   FUN_004cc350 — RenderPipeline_MatrixPushCopy (src x top push)
//   FUN_004cc480 — RenderPipeline_MatrixScaleXY  (scale top matrix (x,y,1))
//   FUN_004cc4d0 — RenderPipeline_MatrixTranslateXY ((x,y,0) translate)
//   FUN_004cc020 — RenderPipeline_SetViewWindow (view window/aspect --
//                  portable core without camera object access)
//                  + RenderPipeline_SetViewWindowFromCamera (full path:
//                  camera fields + cache invalidation + aspect)
//   FUN_004cc120 — RenderPipeline_GetCamera   (active camera cache)
//   FUN_004cc130 — RenderPipeline_SetContext   (dword_66d79c context ptr)
//   FUN_004cbe10 — RenderPipeline_GetViewParams (window offset + aspect)
//   FUN_004cc520 — RenderPipeline_BuildViewMatrix (projection param
//                  block: cached scale matrix + its inverse + camera
//                  frame multiply)
//   FUN_004cc140 — RenderPipeline_Init        (startup subsystem chain;
//                  lives in renderpipeline.cpp, this module provides the
//                  reset step)
//
// Layout: binary RwMatrix is 64 bytes (right/up/at/pos + flags) — CCMatrix
// is exactly the same. Stack: dword_66D644[0..31] slot pointers, dword_66D6C4
// depth (0-based top); slot[N] = copy of slot[N-1] (sub_4CC2F0).
// Root/global matrix dword_66D604 (flags dword_66D610 |= 0x20003).
//
// Engine side (not ported — engine context required):
//   sub_4d5450/sub_4d0e70/sub_4d41c0 — 8/0x14/0x10/0x1C byte stream records
//   (sub_50dbf0) + freelist (dword_65d7a4 vtable +0x144 tahsisi); portta
//   counterpart pool allocation (PoolAlloc) — record is a no-op.
//   sub_4cc020 vtable hook ([camera+0x1C] -> sub_4cc110 thunk,
//   original dword_66d818) — engine object writes; skipped in the port
//   (the game calls SetViewWindowFromCamera on camera update).
//   sub_50e990 (RwFrameGetMatrix — frame+0x50 LTM) — the port gives a
//   direct matrix via the RCCCameraView pFrameMatrix field.
//
// sub_4cc520 param block (portable): cached scale matrix
// unk_66d6d8 = (top x root) · scale(zScale,zScale,1) · scale(-w,w,1) ·
// translate(winX2×zScale×0.5, -winY2×zScale×0.5, 0); its inverse unk_66d718
// (sub_50ff20); result unk_66d758 = scale x camera frame (every call).
// ============================================================================
#pragma once

#include "rwcore.h"

/**
 * @brief 64-byte 3x4 affine matrix — binary RwMatrix layout.
 * right/up/at columns (base vectors) + flags; pos = translation.
 */
struct CCMatrix {
    RwReal right[3];  ///< +0x00 column 0 (X base)
    RwInt32 flags;    ///< +0x0C (binary: |= 0x20003 — unit flags)
    RwReal up[3];     ///< +0x10 column 1 (Y base)
    RwInt32 pad0;     ///< +0x1C
    RwReal at[3];     ///< +0x20 column 2 (Z base)
    RwInt32 pad1;     ///< +0x2C
    RwReal pos[3];    ///< +0x30 translation
    RwInt32 pad2;     ///< +0x3C
};

// RenderPipeline_Reset — FUN_004cc1f0. Stack depth 0, slot[0] identity
// matrix, root matrix (dword_66d604) identity, index table identity, aspect
// ratio pair 1.0, z (dword_66d7a0) = 1.0, half-pixel (dword_66d7a4) = 0.5,
// cache flags cleared. Camera/window parameters arrive later via
// RenderPipeline_SetViewWindow (otherwise stays 1.0 — like the binary).
RwBool RenderPipeline_Reset(void);

// RenderPipeline_MatrixIdentity — FUN_004cbd20. Makes the top matrix identity,
// flags |= 0x20003, clears the cache flags.
RwBool RenderPipeline_MatrixIdentity(void);

// RenderPipeline_MatrixPush — FUN_004cc2f0. If depth < 31, adds one level on
// top and writes a copy of the parent; overflow -> FALSE (top unchanged).
RwBool RenderPipeline_MatrixPush(void);

// RenderPipeline_MatrixPushCopy — FUN_004cc350. pSrc == NULL → MatrixPush
// (parent copy); otherwise new top = pSrc x parent (sub_50f760 multiply,
// parent applied first).
RwBool RenderPipeline_MatrixPushCopy(const CCMatrix *pSrc);

// RenderPipeline_MatrixScaleXY — FUN_004cc480. Scales the top matrix bases by
// (x, y, 1) (RwMatrixScale — does not touch pos); clear cache.
RwBool RenderPipeline_MatrixScaleXY(RwReal x, RwReal y);

// RenderPipeline_MatrixTranslateXY — FUN_004cc4d0. pos += right×x + up×y
// (RwMatrixTranslate — translation is routed through the current scale/rotation);
// clear cache.
RwBool RenderPipeline_MatrixTranslateXY(RwReal x, RwReal y);

// RenderPipeline_SetViewWindow — portable core of FUN_004cc020.
// winX2/winY2 = camera view window x2 (binary [camera+0x68/+0x6C]x2);
// aspect pair (flt_66d7c0, flt_66d7c4): max/min = the other is 1.0.
// width/height = raster pixel dimensions (flt_66d6d0/d4 — informational).
RwBool RenderPipeline_SetViewWindow(RwReal width, RwReal height,
                                    RwReal winX2, RwReal winY2);

/**
 * @brief Portable camera view for the binary RwCamera — fields read by sub_4cc020/
 * sub_4cc520; offsets document the binary layout.
 */
struct RCCCameraView {
    RwInt32 projFlag;             ///< +0x14 — ==2 -> depth z 1.0 (otherwise
                                  ///< dword_66d7a0); rwCAMERAPROJECTION
    RwInt32 rasterWidth;          ///< [raster+0xC] — fild → flt_66d6d0
    RwInt32 rasterHeight;         ///< [raster+0x10] — fild → flt_66d6d4
    RwReal viewWindowX;           ///< +0x68 — half width (x2 -> flt_66d6c8)
    RwReal viewWindowY;           ///< +0x6C — half height (x2 -> flt_66d6cc)
    const CCMatrix *pFrameMatrix; ///< sub_50e990([camera+4]) -> frame LTM
};

// RenderPipeline_SetViewWindowFromCamera — FUN_004cc020 tam yolu.
// RenderPipeline_SetViewWindowFromCamera — FUN_004cc020 full path.
// Camera NULL -> active camera cache cleared, FALSE; raster NULL ->
// FALSE (cache preserved — like the binary). On success: SetViewWindow core
// + active camera record + window offsets zeroed + sub_4cc520 caches
// invalidated. vtable hook is skipped (engine side — header block).
RwBool RenderPipeline_SetViewWindowFromCamera(const RCCCameraView *pCam);

// RenderPipeline_GetCamera — FUN_004cc120. dword_66d7cc active camera
// cache (saved by SetViewWindowFromCamera; cleared by Reset).
const RCCCameraView *RenderPipeline_GetCamera(void);

// RenderPipeline_SetContext — FUN_004cc130. dword_66d79c — the game's
// current draw context (scene/root); the first step of FUN_004cc140 zeroes it.
RwBool RenderPipeline_SetContext(void *pContext);
void RenderPipeline_ClearContext(void); // dword_66d79c = 0
void *RenderPipeline_GetContext(void);

// RenderPipeline_GetViewParams — FUN_004cbe10. window offset (dword_
// 66d7b8/bc — zeroed by SetViewWindowFromCamera) + aspect pair.
void RenderPipeline_GetViewParams(RwReal *pOffX, RwReal *pOffY,
                                  RwReal *pAspectX, RwReal *pAspectY);

// RenderPipeline_BuildViewMatrix — FUN_004cc520. Rebuilds the cached
// scale matrix (unk_66d6d8) if invalid (top x root · scale(zScale,
// zScale, 1) · scale(-w,w,1) · translate(winX2×zScale×0.5, -winY2×zScale×
// 0.5, 0); its inverse unk_66d718. Result unk_66d758 = scale x camera frame —
// EVERY CALL (camera frame refreshes the cache). If there is no camera/frame
// matrix, the port guide: the scale matrix itself is returned (binary would lock).
const CCMatrix *RenderPipeline_BuildViewMatrix(void);

// Test access — top matrix (dword_66d644[dword_66d6c4]) and depth
// (dword_66d6c4); aspect pair (flt_66d7c0/66d7c4).
const CCMatrix *RenderPipeline_GetTopMatrix(void);
RwInt32 RenderPipeline_GetStackDepth(void);
void RenderPipeline_GetAspect(RwReal *pAspectX, RwReal *pAspectY);
void RenderPipeline_GetViewWindow(RwReal *pWidth, RwReal *pHeight,
                                  RwReal *pWinX2, RwReal *pWinY2);
const CCMatrix *RenderPipeline_GetViewScaleMatrix(void); // unk_66d6d8
const CCMatrix *RenderPipeline_GetViewInverseMatrix(void); // unk_66d718
