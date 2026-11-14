#ifndef FONTSYSTEM_H
#define FONTSYSTEM_H

// ============================================================================
// CannonCruise - Font Metrics System (FontSystem.h)
// Original path: D:\Projects\CannonCruisePC\code\FontSystem.h
// Reverse-engineered via Ghidra and CC.asm analysis.
//
// Binary Functions:
//   FUN_004cefd0 — FontSystemAllocBuffers (glyph vertex buffer + 0x78 pool)
//   FUN_004cd660 — FontSystemInit         (path, glyph quad index table)
//   FUN_004cd9a0 — FontSystemLoad         (fonts/*.met, METRICS1 parser)
//   FUN_004cd4d0 — FontAlloc              (font object allocation)
//   FUN_004cd8c0 — FontFree               (font object release)
//   FUN_004cdbc0 — ParseMetrics1          (METRICS1 text format)
//   FUN_004ce870 — ParseMetrics3          (METRICS3 vector contour format;
//                  begin/moveto/lineto/curveto/closepath/end — PostScript
//                  similar; draw handler sub_4CCAF0, contour raster
//                  sub_4D54C0)
//   FUN_004cc690 — FontSystem_LoadFromStream (binary stream font loader
//                  — sub_491700 chain; type byte ≥1, texture/vector two paths)
//   FUN_004d11d0 — LoadContourFromStream (type-5 contour record: 0x10 header
//                  + 0x18 prebaked stroke records; flags&4 → chain)
//   FUN_004ce000 — FontFinalizeExtMap     (extended code map)
//   FUN_004cee90 — FontSystemGet          (name cache: request → node)
//   FUN_004cd580 — FontSystemTeardown     (__ioterm+62: all caches/pools)
//   FUN_004cef90 — FontSystemShutdown     (__ioterm+67: vertex buffer)
//
// Note: Since the original .met assets are not stored in the repo, unit tests verify with
//      synthetic files. METRICS2 (FUN_004ce0d0) was analyzed — raster
//      pixel-scan based format, portable port still blocked (see
//      ROADMAP.md); METRICS3 restored (contour data + advance; contour
//      rasterization sub_4D54C0 Phase 4.4b).
// ============================================================================

#include <rwcore.h>

/**
 * @brief Vector contour point — binary sub_4CFE80/4CFF40/4D0040 records.
 *
 * Point types (binary: [esi] dword): 0 = moveto (sub_4CFE80), 1 = lineto
 * (sub_4CFF40), 2 = curveto (sub_4D0040 — PostScript order x1 y1 x2 y2
 * x3 y3: x/y = first control, cx1/cy1 = second control, cx2/cy2 = end
 * point). Binary 0x18-byte record; the port keeps the bezier raw (rasterization
 * sub_4D54C0 — Phase 4.4b). ex/ey are reserved (unused).
 */
struct CCFontPathPoint {
    RwUInt32 type;  ///< 0=moveto, 1=lineto, 2=curveto
    RwReal   x;     ///< moveto/lineto point; curveto: control1 X
    RwReal   y;
    RwReal   cx1;   ///< curveto: control2 X
    RwReal   cy1;
    RwReal   cx2;   ///< curveto: end point X
    RwReal   cy2;
    RwReal   ex;    ///< reserved (unused)
    RwReal   ey;
};

/**
 * @brief Vector subpath — binary sub_4D0C90 contour object.
 *
 * Binary: each moveto (when the array is full) starts a new contour object and
 * chains it via +0x14; the port models the same semantics with a flat
 * dynamic array using pNext (malloc instead of the engine pool/array).
 */
struct CCFontContour {
    CCFontPathPoint *points;      ///< point array
    RwInt32          pointCount;
    RwInt32          capacity;
    RwBool           closed;      ///< closepath (sub_4D0220) seen
    CCFontContour   *pNext;       ///< next subpath (moveto chain)
};

/**
 * @brief 32-byte metric record of a glyph (binary: +0x00..+0x1F).
 *
 * CC_ida.asm sub_4CDBC0 glyph line ("%i %i %i %i %i") fields:
 * code, left, top, right, bottom are pixel coordinates; UVs are divided by
 * texture dimensions and stored (pixel center +0.5).
 */
struct CCFontGlyph {
    RwReal     aspect;    ///< +0x00 (right-left)/(bottom-top) aspect ratio
    RwTexture *texture;   ///< +0x04 glyph texture candidate (font texture [0] in M1)
    RwReal     u0;        ///< +0x08 left UV
    RwReal     v0;        ///< +0x0C top UV
    RwReal     u1;        ///< +0x10 right UV
    RwReal     v1;        ///< +0x14 bottom UV
    void      *data;      ///< +0x18 contour/kern data (M1 = 0; M3 =
                          ///<      CCFontContour* subpath chain head —
                          ///<      FontFree releases it via sub_4D0C20)
    RwUInt8    flags;     ///< +0x1C flags (M1 = 0)
    RwUInt8    pad[3];
};

/**
 * @brief Font object — binary size 0x144 (324 bytes), sub_4CD4D0.
 *
 * Layout matches the binary 1:1; sub_4CD4D0 field initial
 * values (refCount=1, sizeScale=1.0f, ascii[]=0xFFFF...) are preserved.
 */
struct CCFont {
    RwUInt32   refCount;      ///< +0x00 (sub_4CD4D0: 1)
    RwReal     lineExtent;    ///< +0x04 last glyph's (bottom-top) value (pixels)
    RwReal     sizeScale;     ///< +0x08 .met size / lineExtent
    RwReal     spacing;       ///< +0x0C advance spacing between glyphs
                              ///<   (M1: 0 — written at end of line by sub_4CDBC0)
    RwUInt32   glyphFlags;    ///< +0x10 (M1: |= 2 — extended code present)
    RwTexture *textures[4];   ///< +0x14,+0x18,+0x1C,+0x20 (M1 only [0])
    RwUInt16   ascii[128];    ///< +0x24..+0x123 — 0..127 code → glyph index
                              ///<   (empty = 0xFFFF; sub_4CD4D0 memset -1)
    RwInt32    extMin;        ///< +0x124 first code of the extended range
    RwInt32    extCount;      ///< +0x128 extended range length
    RwUInt16  *extTable;      ///< +0x12C (code - extMin) → glyph index
    RwInt32    glyphCount;    ///< +0x130 capacity during parsing,
                              ///<   effective glyph count at the end (sub_4CDBC0)
    CCFontGlyph *glyphs;      ///< +0x134 glyph array
    void      *drawHandler;   ///< +0x138 M1: sub_4CCC40 (texture), M3:
                              ///<   sub_4CCAF0 (vector contour). The port has a single
                              ///<   path via FontSystem_DrawText (Phase 2/3).
    RwUInt32   reserved1;     ///< +0x13C (sub_4CD4D0: 0)
    RwUInt32   reserved2;     ///< +0x140 (sub_4CD4D0: 0)
};

/**
 * @brief Text draw material — binary sub_4CCC40/sub_4D54C0 arg_10.
 *
 * Solid color path (no +0x64 flag): bytes +0x60..+0x63 RGBA.
 * Lighting path (+0x64 & 1): each apex vertex is colored with ambient + directional light x
 * horizontal gradient (in invokeWideText invW = 1/width; in the vector raster
 * grad = path length/total).
 * Textured stroke (+0x64 & 2): UV gradient — u = uStart + uDelta×grad
 * (top +0x10/+0x28, bottom +0x40/+0x58; sub_4D54C0).
 * strokeWidth (+0x70): vector contour half width (sub_4D54C0 var_84).
 * Layout matches the binary field ranges 1:1.
 */
struct CCFontMaterial {
    RwReal topAmbient[4];    ///< +0x00 (top vertices ambient RGBA)
    RwReal uTop;             ///< +0x10 textured stroke: top U (grad 0)
    RwReal vTop;             ///< +0x14
    RwReal topLight[4];      ///< +0x18 (top vertices gradient dir/color)
    RwReal uTopDelta;        ///< +0x28 textured stroke: top U delta
    RwReal vTopDelta;        ///< +0x2C
    RwReal bottomAmbient[4]; ///< +0x30 (bottom vertices ambient RGBA)
    RwReal uBottom;          ///< +0x40 textured stroke: bottom U (grad 0)
    RwReal vBottom;          ///< +0x44
    RwReal bottomLight[4];   ///< +0x48 (bottom vertices gradient)
    RwReal uBottomDelta;     ///< +0x58 textured stroke: bottom U delta
    RwReal vBottomDelta;     ///< +0x5C
    RwUInt8  red;            ///< +0x60 flat color
    RwUInt8  green;          ///< +0x61
    RwUInt8  blue;           ///< +0x62
    RwUInt8  alpha;          ///< +0x63
    RwUInt8  flags;          ///< +0x64 bit0 = lighting, bit1 = textured stroke
    RwUInt8  pad04[3];       ///< +0x65
    RwTexture *texture;      ///< +0x68 (binary texture state — glyph
                              ///<   textures come from the glyph)
    RwReal pad05;            ///< +0x6C (32-bit binary layout; in the 64-bit port
                             ///<   pointer width shifts it)
    RwReal strokeWidth;      ///< +0x70 vector stroke half width (var_84)
};

/**
 * @brief Name cache node — binary 12 bytes (pool dword_66D5D4).
 *
 * sub_4CEE90: when a font name is requested this linked list is searched first; if not found
 * it is loaded via FontSystemLoad and prepended.
 */
struct CCFontNode {
    char        *pName;  ///< +0x00 strdup'd requested name
    CCFont      *pFont;  ///< +0x04 loaded font (node not created on failure)
    CCFontNode  *pNext;  ///< +0x08 next node (head = dword_66D5D8)
};

/**
 * @brief Texture reader — counterpart of sub_5154D0 (name, mask → texture).
 *
 * The default RwTextureRead route returns NULL when the real texture is missing and
 * font loading fails as in the binary; unit tests inject a fake
 * reader (dimensions are needed for the UV calculation: the binary reads the texture
 * raster width/height).
 */
typedef RwTexture *(*FontTextureReader)(const char *pName, const char *pMask,
                                        RwInt32 *pWidth, RwInt32 *pHeight);

/**
 * @brief Binary font stream — counterpart of sub_50D940/sub_50CD80/sub_50D790.
 *
 * sub_4CC690 (stream font loader) stream abstraction: the caller binds an in-memory
 * file/asset stream via these three callbacks. pUser is the caller's
 * state (cursor, file, etc.).
 */
struct CCFontStream {
    void *pUser;
    /// sub_50D940 — read size bytes; TRUE if all read (EOF → FALSE).
    RwBool (*read)(CCFontStream *pSelf, void *pBuf, RwInt32 size);
    /// sub_50CD80 — exact read; TRUE if the returned byte count equals size.
    RwBool (*readExact)(CCFontStream *pSelf, void *pBuf, RwInt32 size);
    /// sub_50D790 — find/enter chunk (0x1A1 before type-5 contour).
    RwBool (*findChunk)(CCFontStream *pSelf, RwUInt32 type);
};

/**
 * @brief Binary stream font loader (port of sub_4CC690).
 *
 * Stream: type tag 4B ((tag>>24) >= 1), font header (refCount,
 * lineExtent, sizeScale, spacing, glyphFlags, extMin, glyphCount,
 * extCount), extTable (extCount×2), ascii map (0x100), glyphs.
 * The refCount field in the font header SELECTS THE PATH: 0 → texture path (each glyph
 * UV+aspect+flags byte, texture names 0x20; glyph.texture = textures[
 * (RwInt8)flags]);≠0 → vector path (each glyph: 4B waste + aspect + type;
 * type==5 → findChunk(0x1A1) + contour record). drawHandler is 0 in the port
 * (FontSystem_DrawText single path).
 *
 * DEVIATIONS (documented): texture context save/restore (sub_5154C0/5154A0)
 * no-op; prebaked stroke records (0x18) are converted to CCFontContour path points
 * (type0 → moveto, others → lineto; normal/length recomputed at render
 * time — no miter, same deviation as 3b3b Phase 3b);
 * contour[+0x10]/node+8 flag fields are read but ignored; texture
 * index range [0,3] is validated in the port (the binary does not validate).
 *
 * @param pStream Stream (callbacks filled; NULL → NULL).
 * @return The loaded font; NULL on failure (the font is released).
 * @see FUN_004cc690 (sub_491700 chain)
 */
CCFont *FontSystem_LoadFromStream(CCFontStream *pStream);

/**
 * @brief Creates the font vertex buffer + draw pool.
 * @return TRUE if allocation succeeded.
 * @see FUN_004cefd0 (sub_4CC140 chain, step 3)
 */
RwBool FontSystemAllocBuffers(void);

/**
 * @brief Starts the font subsystem: "fonts/" path buffer, 64 quad
 * index table and list heads.
 * @return TRUE on success.
 * @see FUN_004cd660 (sub_4CC140 chain, step 4)
 */
RwBool FontSystemInit(void);

/**
 * @brief Loads the "fonts/<name>.met" file (METRICS1).
 *
 * @param pFontList Semicolon-separated fallback name list
 *                  (binary: copy loop + strcat + ';').
 * @return The font of the first successful candidate; NULL if none succeed.
 * @see FUN_004cd9a0 (dword_66DCC0 load callback)
 */
CCFont *FontSystemLoad(const char *pFontList);

/**
 * @brief Looks up a font by name; loads it if absent and adds it to the name cache.
 * @param pFontName Font name to look up.
 * @return Cache node (pFont field); NULL on failure.
 * @see FUN_004cee90
 */
CCFontNode *FontSystemGet(const char *pFontName);

/**
 * @brief Looks up a glyph metric by code (direct ASCII + extended map).
 * @param pFont Font nesnesi (NULL olabilir).
 * @param code Unicode code point (0..127 direct; >=0x80 extTable).
 * @return Glyph record; NULL if not found.
 */
const CCFontGlyph *FontSystemGetGlyph(const CCFont *pFont, RwUInt32 code);

/**
 * @brief Releases the font caches and pools
 *        (path buffer, name list, fonts, extended map).
 * @see FUN_004cd580 (__ioterm+62)
 */
void FontSystemTeardown(void);

/**
 * @brief Releases the vertex buffer and the 0x78 draw pool.
 * @see FUN_004cef90 (__ioterm+67)
 */
void FontSystemShutdown(void);

/**
 * @brief Replaces the texture reader (test injection / later phases).
 * @param pReader NULL = return to the default RwTextureRead route.
 */
void FontSystem_SetTextureReader(FontTextureReader pReader);

/**
 * @brief Access to the 64 quad index table (produced by sub_4CD660).
 * @return [64][6] RwUInt16 table (binary: unk_66D2C4..0x66D5C4).
 */
const RwUInt16 (*FontSystem_GetQuadIndices(void))[6];

/**
 * @brief Measures text with glyph advances (port of sub_4CD7E0).
 *
 * Loop: per unit → glyph lookup; missing glyphs are SKIPPED (advance
 * not added); for found ones scale (aspect + pFont->spacing) is accumulated and
 * multiplied by scale.
 *
 * Wide mode: if font->glyphFlags&2 the text is UTF-16LE — unit is 2 bytes
 * (low first), cursor steps 2; the binary cursor stops on signed <=0 (codes
 * 0x8000+ cause early exit — same as the binary behavior).
 *
 * @param pFont Font nesnesi (NULL → 0).
 * @param pText NUL-terminated string (8-bit or UTF-16LE — glyphFlags&2).
 * @param scale Scale factor (binary arg_8 — pixel height).
 * @return Total width (pixels); 0 if pFont is NULL.
 * @see FUN_004CD7E0
 */
RwReal FontSystem_MeasureText(const CCFont *pFont, const char *pText,
                              RwReal scale);

/**
 * @brief Draws text as Im2D quads (port of sub_4CCC40).
 *
 * Binary flow (core — excluding matrix stack/engine vcalls,
 * see the deviation notes below):
 *  1. Character → glyph (FontSystemGetGlyph); missing ones are skipped.
 *  2. Texture change or batch flush on the 63rd quad
 *     (RwIm2DRenderIndexedPrimitive, rwPRIMTYPETRILIST; indices
 *     counterpart of the sub_4CD660 table: {v0,v2,v3,v0,v3,v1}).
 *  3. Quad: V0(pen,0) V1(pen,1) V2(pen+aspect,0) V3(pen+aspect,1);
 *     UVs in binary order (u0,v1)(u0,v0)(u1,v1)(u1,v0).
 *  4. Advance: pen += aspect + pFont->spacing.
 *  5. Color: material flat bytes (+0x60..63) or the lighting path
 *     (+0x64&1) gradient ambient + light x (pen/width).
 *  6. Return: *pPosX += pen * scale (binary final fadd).
 *
 * Deviation notes (up to Phase 4.4b):
 *  - The binary generates vertices in unit space (y ∈ {0,1}) and scales/translates them with the matrix stack
 *    (sub_4CC2F0/4CC4D0/4CC480, engine matrix stack);
 *    the port generates screen coordinates directly (x*scale, y*scale).
 *  - z/rhw: binary dword_66D7A0 (kamera) — port 0.0f/1.0f.
 *  - Wide (UTF-16LE) text mode (glyphFlags&2) is supported — same unit decoding
 *    as MeasureText.
 *  - Textureless (METRICS3 vector) glyphs produce no quad; contour raster
 *    sub_4D54C0 (Phase 4.4b TODO) — advance still applies.
 *
 * @param pFont     Font nesnesi.
 * @param pText     NUL-terminated string (8-bit or UTF-16LE — glyphFlags&2).
 * @param scale     Scale (pixel height; binary arg_8).
 * @param pPosX     Input: left X; output: after pen*x_scale (inout).
 * @param pPosY     Left Y (vertex y=0 top, y=1 bottom — binary unit space).
 * @param pMaterial Material (NULL → flat white).
 * @return Advance (pen*scale) — chaining for the next line/position.
 * @see FUN_004CCC40 (font->drawHandler)
 */
RwReal FontSystem_DrawText(const CCFont *pFont, const char *pText,
                           RwReal scale, RwReal *pPosX, RwReal *pPosY,
                           const CCFontMaterial *pMaterial);

/**
 * @brief Test access to the last batch's vertex array (port of sub_4CCC40).
 * @param pVertexCount Output: last batch vertex count (quad × 4); may be NULL.
 * @return Vertex buffer (NULL before AllocBuffers); with multiple batches
 *         only the last batch is valid (vertices are rewritten in the buffer).
 */
const RwIm2DVertex *FontSystem_GetDrawVertices(RwInt32 *pVertexCount);

#endif // FONTSYSTEM_H
