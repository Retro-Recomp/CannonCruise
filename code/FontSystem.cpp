// ============================================================================
// CannonCruise - Font Metrics System (FontSystem.cpp)
// Original path: D:\Projects\CannonCruisePC\code\FontSystem.cpp
// Reverse-engineered via Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_004cefd0 — FontSystemAllocBuffers (dword_66D5C4 vertex buffer 0x2400
//                  + dword_66D5C0 0x78-sized draw pool)
//   FUN_004cd660 — FontSystemInit ("fonts\" path buffer dword_66D5CC,
//                  unk_66D2C4..0x66D5C4 quad index table,
//                  dword_66D5DC/E0 list head, dword_66DCC0 = loader)
//   FUN_004cd9a0 — FontSystemLoad ("fonts\<name>.met", METRICS1)
//   FUN_004cd4d0 — FontAlloc (pool dword_66D5C8, 0x144 object)
//   FUN_004cd8c0 — FontFree (glyph data sub_4D0C20, texture sub_515300)
//   FUN_004cdbc0 — ParseMetrics1
//   FUN_004ce870 — ParseMetrics3 (METRICS3 vector contour format — begin/
//                  moveto/lineto/curveto/closepath/end; contour drawing
//                  sub_4CCAF0 -> raster sub_4D54C0 Phase 4.4b TODO)
//   FUN_004ce000 — FontFinalizeExtMap (dword_66DE1C extended map)
//   FUN_004cee90 — FontSystemGet (dword_66D5D8 name list)
//   FUN_004cd580 — FontSystemTeardown (__ioterm+62)
//   FUN_004cef90 — FontSystemShutdown (__ioterm+67)
//   FUN_004cd7e0 — FontSystem_MeasureText (text width; glyphFlags&2
//                  -> UTF-16LE units)
//   FUN_004ccc40 — FontSystem_DrawText (font->drawHandler draw path;
//                  matrix stack sub_4CC2F0/4CC4D0/4CC480 and engine
//                  vcall's Phase 4.4b — port produces exact UV/
//                  progress/batch semantics in screen space; UTF-16LE support
//                  same as MeasureText)
//
// Engine-independent parts (memory, string, file) modeled in portable C;
// engine vtable mappings:
//   +0x120 strlen, +0xF8 strcpy, +0x100 strcat, +0x130 sscanf,
//   +0x114 strcmp, +0x134 malloc, +0x13C realloc, +0x138 free,
//   +0x144 pool allocation, +0x148 pool free
// ============================================================================

#include "FontSystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>

namespace
{
    // dword_66D5CC + dword_66D5D0 (capacity 0x51) — binary uses Windows "fonts\"
    // path; uses '/' separator since it works in fopen on both Windows
    // and POSIX, written unified.
    const char kFontPath[] = "fonts/";

    // unk_66D2C4..(0x66D5C4) — 64 quads x 6 x uint16 = 0x300 bytes.
    // Binary loop: ecx=3, step 4; each 12-byte block:
    //   {ecx-3, ecx-1, ecx, ecx-3, ecx, ecx-2}
    RwUInt16 s_QuadIndices[64][6];

    // dword_66D5D8 — font name cache list head (sub_4CEE90 / sub_4CD580)
    CCFontNode *s_pFontListHead = 0;

    // dword_66D5DC / dword_66D5E0 — sub_515130 list head node.
    // Binary circular node + added to engine list; port uses simple pointer.
    void *s_pListNode = 0;

    // dword_66D5C4 — 0x2400-byte glyph vertex buffer (sub_4CEFD0)
    void *s_pVertexBuffer = 0;

    // dword_66DE1C / dword_66DE20 — extended code draw sequence (cache).
    // Glyph index -> code mapping; terminated with 0xFFFFFFFF.
    RwInt32 *s_pExtScratch = 0;
    RwInt32  s_extScratchCap = 0;

    // sub_5154D0 texture reader (test injection) — 0 = default path
    FontTextureReader s_pTextureReader = 0;

    // dword_66DCC0 — load callback (registered by sub_4CD660;
    // sub_4CEE90 calls it from here). The port uses FontSystemLoad directly.
    typedef CCFont *(*FontLoaderCallback)(const char *);
    FontLoaderCallback s_pLoader = &FontSystemLoad;

    // sub_51A0A0/sub_51A230 — pool/free counterpart: directly malloc.
    // TODO(Phase 4.4b): engine pool semantics (engine+0x144/+0x148 freelist).
    void *PoolAlloc(size_t bytes) { return std::malloc(bytes); }
    void  PoolFree(void *p) { std::free(p); }

    // unk_66DDCC — sprintf/scratch buffer for header parsing
    char s_Scratch[256];

    // sub_517CF0 / sub_517DB0 — global string buffer Set/Get counterpart
    // (binary: dword_65DDFC[+4] buffer, [+8] capacity). Port uses static string.
    char s_StringBuf[512];
    void ScratchString_Set(const char *pStr) {
        std::snprintf(s_StringBuf, sizeof(s_StringBuf), "%s", pStr ? pStr : "");
    }
    const char *ScratchString_Get(void) { return s_StringBuf; }

    // sub_5154D0 — (name, mask) -> texture. Default path RwTextureRead
    // declared in rwcore.h; rwstubs return NULL when no real texture
    // (binary: font load fails if texture not found).
    RwTexture *ReadTexture(const char *pName, const char *pMask,
                           RwInt32 *pWidth, RwInt32 *pHeight) {
        *pWidth = 0;
        *pHeight = 0;
        if (s_pTextureReader) {
            return s_pTextureReader(pName, pMask, pWidth, pHeight);
        }
        RwTexture *pTex = RwTextureRead(pName, pMask);
        if (pTex) {
            RwRaster *pRaster = RwTextureGetRaster(pTex);
            if (pRaster) {
                *pWidth = RwRasterGetWidth(pRaster);
                *pHeight = RwRasterGetHeight(pRaster);
            }
        }
        return pTex;
    }

    // sub_515300 — texture release (refcount decrement counterpart)
    void TextureRelease(RwTexture *pTex) {
        if (pTex) {
            RwTextureDestroy(pTex);
        }
    }

    // ctx vtable[0] — file existence probe (for single-token texture mask)
    RwBool FileExists(const char *pPath) {
        std::FILE *pFile = std::fopen(pPath, "rb");
        if (!pFile) {
            return FALSE;
        }
        std::fclose(pFile);
        return TRUE;
    }

    // fgets strips trailing \r\n (binary engine string comparison
    // uses engine+0x114; port gives same result on CRLF/LF files).
    // engine+0x114 uses; port gives same result on CRLF/LF files).
    void StripNewline(char *pLine) {
        size_t len = std::strlen(pLine);
        while (len > 0 && (pLine[len - 1] == '\n' || pLine[len - 1] == '\r')) {
            pLine[--len] = '\0';
        }
    }

    // sub_4CE000 — builds the extended (>=0x80) code map.
    // dword_66DE1C[] (glyph index → code) is scanned; in the min..max range
    // a uint16 table is produced (empty entries 0xFFFF — binary: or eax,-1 fill).
    void FontFinalizeExtMap(CCFont *pFont) {
        RwInt32 count = pFont->glyphCount;
        if (count <= 0 || !s_pExtScratch) {
            return;
        }

        RwInt32 minCode = 0xFFFF; // binary: esi = 0FFFFh
        RwInt32 maxCode = 0;
        RwBool  found = FALSE;
        for (RwInt32 i = 0; i < count; ++i) {
            RwInt32 code = s_pExtScratch[i];
            if (code > 0) {
                if (code < minCode) {
                    minCode = code;
                }
                if (code > maxCode) {
                    maxCode = code;
                }
                found = TRUE;
            }
        }
        if (!found) {
            return;
        }

        RwInt32 range = maxCode - minCode + 1;
        RwUInt16 *pTable = static_cast<RwUInt16 *>(PoolAlloc(range * sizeof(RwUInt16)));
        if (!pTable) {
            return;
        }
        for (RwInt32 i = 0; i < range; ++i) {
            pTable[i] = 0xFFFF;
        }
        for (RwInt32 i = 0; i < count; ++i) {
            RwInt32 code = s_pExtScratch[i];
            if (code > 0) {
                pTable[code - minCode] = static_cast<RwUInt16>(i);
            }
        }

        pFont->extMin = minCode;
        pFont->extCount = range;
        pFont->extTable = pTable;
    }

    // ---------------------------------------------------------------------------
    // Glyph code registration — common block of sub_4CDBC0 (M1) and sub_4CE870 (M3):
    // <0x80 → ascii[]; >=0x80 → glyphFlags|=2 + dword_66DE1C paint array
    // (capacity grows by +0x20; each element starts as 0xFFFFFFFF).
    // ---------------------------------------------------------------------------
    RwBool RegisterGlyphCode(CCFont *pFont, RwInt32 code, RwInt32 glyphIndex) {
        if (code < 0x80) {
            pFont->ascii[code] = static_cast<RwUInt16>(glyphIndex);
            return TRUE;
        }
        if (glyphIndex >= s_extScratchCap) {
            RwInt32 newCap = s_extScratchCap + 0x20;
            RwInt32 *pNew = static_cast<RwInt32 *>(
                PoolAlloc(static_cast<size_t>(newCap) * sizeof(RwInt32)));
            if (!pNew) {
                return FALSE;
            }
            for (RwInt32 i = 0; i < newCap; ++i) {
                pNew[i] = -1;
            }
            for (RwInt32 i = 0; i < s_extScratchCap; ++i) {
                pNew[i] = s_pExtScratch[i];
            }
            PoolFree(s_pExtScratch);
            s_pExtScratch = pNew;
            s_extScratchCap = newCap;
        }
        pFont->glyphFlags |= 2;                 // [esi+0x10] |= 2
        s_pExtScratch[glyphIndex] = code;       // dword_66DE1C[idx] = code
        return TRUE;
    }

    // ---------------------------------------------------------------------------
    // Vector contour helpers — sub_4D0C90 (subpath allocation), sub_4CFE80/
    // 4CFF40/4D0040 (point insertion), sub_4D0220 (closepath), sub_4D0BA0 +
    // sub_4D0DB0 (finalization). The binary uses the engine pool/array (sub_51A0A0/
    // sub_51A230); the port flat-mallocs while modeling the same chain semantics,
    // establishing a caller contract:
    //   *ppHead → chain head; moveto (when the active path is full) advances to the next subpath
    //     ; PPHead is updated.
    // ---------------------------------------------------------------------------
    CCFontContour **ContourCurrent(CCFontContour **ppHead) {
        CCFontContour **ppCur = ppHead;
        while (*ppCur && (*ppCur)->pNext) {
            ppCur = &(*ppCur)->pNext;
        }
        return ppCur;
    }

    CCFontContour *ContourAlloc(void) {
        CCFontContour *pC = static_cast<CCFontContour *>(PoolAlloc(sizeof(CCFontContour)));
        if (!pC) {
            return 0;
        }
        pC->points = 0;
        pC->pointCount = 0;
        pC->capacity = 0;
        pC->closed = FALSE;
        pC->pNext = 0;
        return pC;
    }

    // sub_4CFE80 moveto: if the active path is empty, add the point; if full (binary: root
    // array count!=0) start a new subpath and add there.
    RwBool ContourMoveTo(CCFontContour **ppHead, RwReal x, RwReal y) {
        CCFontContour **ppCur = ContourCurrent(ppHead);
        if (!*ppCur) {
            *ppCur = ContourAlloc();
            if (!*ppCur) {
                return FALSE;
            }
        } else if ((*ppCur)->pointCount > 0) {
            CCFontContour *pNew = ContourAlloc();
            if (!pNew) {
                return FALSE;
            }
            (*ppCur)->pNext = pNew;
            ppCur = &(*ppCur)->pNext;
        }
        CCFontContour *pC = *ppCur;
        if (pC->pointCount >= pC->capacity) {
            RwInt32 newCap = pC->capacity > 0 ? pC->capacity * 2 : 8;
            CCFontPathPoint *pNewPts = static_cast<CCFontPathPoint *>(
                PoolAlloc(static_cast<size_t>(newCap) * sizeof(CCFontPathPoint)));
            if (!pNewPts) {
                return FALSE;
            }
            if (pC->points) {
                std::memcpy(pNewPts, pC->points,
                            static_cast<size_t>(pC->pointCount) *
                                sizeof(CCFontPathPoint));
                PoolFree(pC->points);
            }
            pC->points = pNewPts;
            pC->capacity = newCap;
        }
        CCFontPathPoint &pt = pC->points[pC->pointCount++];
        std::memset(&pt, 0, sizeof(pt));
        pt.type = 0; // binary: [eax] = 0 (moveto)
        pt.x = x;
        pt.y = y;
        return TRUE;
    }

    // point insertion (lineto/curveto) — to the last subpath of the active path; otherwise
    // start like moveto (binary arg probing — port robustness deviation).
    RwBool ContourAddPoint(CCFontContour **ppHead, RwUInt32 type, const RwReal *pF,
                           RwInt32 floatCount) {
        CCFontContour **ppCur = ContourCurrent(ppHead);
        if (!*ppCur) {
            if (!ContourMoveTo(ppHead, 0.0f, 0.0f)) {
                return FALSE;
            }
            ppCur = ContourCurrent(ppHead);
        }
        CCFontContour *pC = *ppCur;
        if (pC->pointCount >= pC->capacity) {
            RwInt32 newCap = pC->capacity > 0 ? pC->capacity * 2 : 8;
            CCFontPathPoint *pNewPts = static_cast<CCFontPathPoint *>(
                PoolAlloc(static_cast<size_t>(newCap) * sizeof(CCFontPathPoint)));
            if (!pNewPts) {
                return FALSE;
            }
            if (pC->points) {
                std::memcpy(pNewPts, pC->points,
                            static_cast<size_t>(pC->pointCount) *
                                sizeof(CCFontPathPoint));
                PoolFree(pC->points);
            }
            pC->points = pNewPts;
            pC->capacity = newCap;
        }
        CCFontPathPoint &pt = pC->points[pC->pointCount++];
        std::memset(&pt, 0, sizeof(pt));
        pt.type = type;
        for (RwInt32 i = 0; i < floatCount && i < 8; ++i) {
            (&pt.x)[i] = pF[i];
        }
        return TRUE;
    }

    // sub_4D0C20 / sub_4D0BA0 + sub_4D0DB0 — release the glyph contour chain.
    void ContourFreeChain(CCFontContour *pHead) {
        while (pHead) {
            CCFontContour *pNext = pHead->pNext;
            if (pHead->points) {
                PoolFree(pHead->points);
            }
            PoolFree(pHead);
            pHead = pNext;
        }
    }

    // ---------------------------------------------------------------------------
    // ParseMetrics3 (sub_4CE870) — METRICS3 vector contour format:
    //   METRICS3\n                       — consumed by FontSystemLoad (dispatch)
    //   <name>\n                          — "%[^\n]" (local; port reads and skips)
    //   <prefix><UTF-8 code>\n           — pending==0 line: binary from buf[1]
    //                                      UTF-8 (1..6 bytes, legacy 5/6 byte
    //                                      forms included) decoded to set the pending
    //                                      code; prefix buf[0] is treated as absent
    //                                      (port format: ')
    //   begin\n / end\n                  — subpath start / glyph end
    //   moveto %f %f / lineto %f %f / curveto %f×6 / closepath\n
    // At glyph end: aspect = first float of the last path-op (var_A0 — binary
    // quirk), data = contour chain, texture = 0; drawn by sub_4CCAF0 (vector).
    // ---------------------------------------------------------------------------
    RwBool ParseMetrics3(CCFont *pFont, std::FILE *pFile) {
        char line[256];

        // 1) name line — binary reads it into var_80 (never observed used); port
        //    skips.
        if (!std::fgets(line, sizeof(line), pFile)) {
            return FALSE;
        }

        // 2) glyph array — capacity [font+0x130] (0 in a fresh object → 0x100)
        RwInt32 capacity = pFont->glyphCount > 0 ? pFont->glyphCount : 0x100;
        pFont->glyphs = static_cast<CCFontGlyph *>(
            PoolAlloc(static_cast<size_t>(capacity) * sizeof(CCFontGlyph)));
        if (!pFont->glyphs) {
            return FALSE;
        }
        std::memset(pFont->glyphs, 0, static_cast<size_t>(capacity) * sizeof(CCFontGlyph));

        RwInt32 glyphCount = 0;   // var_B0
        RwInt32 pendingCode = 0;  // var_B8 — 0 = waiting for a code line
        CCFontContour *pContour = 0;
        // var_A0 — first float of the last path-op (glyph aspect). In the binary
        // it is a stack slot and is NOT zeroed between glyphs (sticky); the port
        // initializes it to 0 up front to remove the ambiguity.
        RwReal lastFirstFloat = 0.0f;
        RwBool failed = FALSE;

        while (std::fgets(line, sizeof(line), pFile)) {
            // binary ReadLine keeps \n and commands stay exact like "begin\n"
            // compared via strcmp with the line; the port compares after StripNewline.
            StripNewline(line);

            if (pendingCode == 0) {
                // code line — UTF-8 from buf[1] (binary: 1..6 bytes legacy form)
                const unsigned char *p = reinterpret_cast<const unsigned char *>(line) + 1;
                RwUInt32 code = 0;
                unsigned char b0 = p[0];
                if (b0 == 0) {
                    failed = TRUE; // empty line after prefix — hard error
                    break;
                }
                if ((b0 & 0x80u) == 0) {
                    code = b0;
                } else if ((b0 & 0xE0u) == 0xC0) {
                    code = (static_cast<RwUInt32>(b0 & 0x1Fu) << 6) |
                           (p[1] & 0x3Fu);
                } else if ((b0 & 0xF0u) == 0xE0) {
                    code = (static_cast<RwUInt32>(b0 & 0x0Fu) << 12) |
                           (static_cast<RwUInt32>(p[1] & 0x3Fu) << 6) |
                           (p[2] & 0x3Fu);
                } else if ((b0 & 0xF8u) == 0xF0) {
                    code = (static_cast<RwUInt32>(b0 & 0x07u) << 18) |
                           (static_cast<RwUInt32>(p[1] & 0x3Fu) << 12) |
                           (static_cast<RwUInt32>(p[2] & 0x3Fu) << 6) |
                           (p[3] & 0x3Fu);
                } else if ((b0 & 0xFCu) == 0xF8) {
                    code = (static_cast<RwUInt32>(b0 & 0x03u) << 24) |
                           (static_cast<RwUInt32>(p[1] & 0x3Fu) << 18) |
                           (static_cast<RwUInt32>(p[2] & 0x3Fu) << 12) |
                           (static_cast<RwUInt32>(p[3] & 0x3Fu) << 6) |
                           (p[4] & 0x3Fu);
                } else if ((b0 & 0xFEu) == 0xFC) {
                    code = (static_cast<RwUInt32>(b0 & 0x01u) << 30) |
                           (static_cast<RwUInt32>(p[1] & 0x3Fu) << 24) |
                           (static_cast<RwUInt32>(p[2] & 0x3Fu) << 18) |
                           (static_cast<RwUInt32>(p[3] & 0x3Fu) << 12) |
                           (static_cast<RwUInt32>(p[4] & 0x3Fu) << 6) |
                           (p[5] & 0x3Fu);
                } else {
                    failed = TRUE; // invalid prefix — binary: var_B4=0 → fail
                    break;
                }
                if (code > 0xFFFF) {
                    failed = TRUE;
                    break;
                }
                pendingCode = static_cast<RwInt32>(code);
                continue;
            }

            RwReal fx[6];
            if (std::strcmp(line, "begin") == 0) {
                if (pContour) {
                    ContourFreeChain(pContour);
                }
                pContour = ContourAlloc();
                if (!pContour) {
                    failed = TRUE;
                    break;
                }
            } else if (std::strcmp(line, "end") == 0) {
                // glyph registration — binary sub_4D0BA0 + sub_4D0DB0 (finalize),
                // code registration, glyph[data]=contour, glyph[aspect]=var_A0
                if (glyphCount >= capacity) {
                    capacity += 0x20;
                    CCFontGlyph *pNewGlyphs = static_cast<CCFontGlyph *>(
                        PoolAlloc(static_cast<size_t>(capacity) * sizeof(CCFontGlyph)));
                    if (!pNewGlyphs) {
                        failed = TRUE;
                        break;
                    }
                    std::memcpy(pNewGlyphs, pFont->glyphs,
                                static_cast<size_t>(glyphCount) * sizeof(CCFontGlyph));
                    std::memset(pNewGlyphs + glyphCount, 0,
                                static_cast<size_t>(capacity - glyphCount) *
                                    sizeof(CCFontGlyph));
                    PoolFree(pFont->glyphs);
                    pFont->glyphs = pNewGlyphs;
                }
                if (!RegisterGlyphCode(pFont, pendingCode, glyphCount)) {
                    failed = TRUE;
                    break;
                }
                CCFontGlyph &glyph = pFont->glyphs[glyphCount];
                glyph.aspect = lastFirstFloat; // var_A0 — last op first float
                glyph.texture = 0;             // vector glyph — no texture
                glyph.data = pContour;
                glyph.flags = 0;
                pContour = 0;
                pendingCode = 0;
                ++glyphCount;
            } else if (std::sscanf(line, "moveto %f %f", &fx[0], &fx[1]) == 2) {
                lastFirstFloat = fx[0]; // var_A0
                if (!ContourMoveTo(&pContour, fx[0], fx[1])) {
                    failed = TRUE;
                    break;
                }
            } else if (std::sscanf(line, "lineto %f %f", &fx[0], &fx[1]) == 2) {
                lastFirstFloat = fx[0];
                if (!ContourAddPoint(&pContour, 1, fx, 2)) {
                    failed = TRUE;
                    break;
                }
            } else if (std::sscanf(line, "curveto %f %f %f %f %f %f", &fx[0],
                                   &fx[1], &fx[2], &fx[3], &fx[4], &fx[5]) == 6) {
                lastFirstFloat = fx[0];
                if (!ContourAddPoint(&pContour, 2, fx, 6)) {
                    failed = TRUE;
                    break;
                }
            } else if (std::strcmp(line, "closepath") == 0) {
                // sub_4D0220 — binary measures first/last point difference and
                // appends a closing segment; the port just marks the closed flag (raster TODO).
                CCFontContour **ppCur = ContourCurrent(&pContour);
                if (*ppCur) {
                    (*ppCur)->closed = TRUE;
                }
            }
            // unknown line — the binary strcmp/sscanf chain silently
            // falls through; the port skips it too.
        }

        if (failed) {
            ContourFreeChain(pContour);
            return FALSE;
        }
        if (pendingCode != 0) {
            // code line left without a block — the binary stays stuck waiting for a block and
            // returns without a glyph at EOF; the port skips the half-done glyph.
            ContourFreeChain(pContour);
        }

        // finalization — sub_4CE870 trailing block
        pFont->glyphCount = glyphCount;
        FontFinalizeExtMap(pFont);
        if (s_pExtScratch) {
            for (RwInt32 i = 0; i < s_extScratchCap; ++i) {
                s_pExtScratch[i] = -1;
            }
        }
        // [font+0x138] = &sub_4CCAF0 (vector handler) — single path in the port
        // FontSystem_DrawText; raster sub_4D54C0 Phase 4.4b TODO.
        pFont->drawHandler = 0;
        return TRUE;
    }

    // ---------------------------------------------------------------------------
    // ParseMetrics1 (sub_4CDBC0) — METRICS1 text format:
    //   METRICS1\n
    //   <texture-name> [mask]\n     - "%s %s" (mask optional)
    //   <size>\n               - "%d"
    //   <code> <left> <top> <right> <bottom>  — "%i %i %i %i %i" (until EOF)
    // ---------------------------------------------------------------------------
    RwBool ParseMetrics1(CCFont *pFont, std::FILE *pFile) {
        char line[256];

        // 1) header line — token1 = texture name, token2 = mask (optional)
        if (!std::fgets(line, sizeof(line), pFile)) {
            return FALSE;
        }
        StripNewline(line);
        char token1[128] = {0};
        char token2[128] = {0};
        int tokens = std::sscanf(line, "%127s %127s", token1, token2);
        if (tokens < 1) {
            return FALSE;
        }

        // 2) texture resolution
        const char *pMask = 0;
        if (tokens == 1) {
            // Binary: sprintf(scratch, "%sm%s", "fonts\", name) → asset probe;
            // if found mask = "m<name>", otherwise NULL.
            std::snprintf(s_Scratch, sizeof(s_Scratch), "%sm%s", kFontPath, token1);
            if (FileExists(s_Scratch)) {
                std::snprintf(s_Scratch, sizeof(s_Scratch), "m%s", token1);
                pMask = s_Scratch;
            }
        } else {
            pMask = token2;
        }

        RwInt32 texWidth = 0;
        RwInt32 texHeight = 0;
        RwTexture *pTex = ReadTexture(token1, pMask, &texWidth, &texHeight);
        if (!pTex || texWidth <= 0 || texHeight <= 0) {
            return FALSE; // binary: failure to find the texture fails the whole load
        }
        pFont->textures[0] = pTex;

        // 3) size line
        if (!std::fgets(line, sizeof(line), pFile)) {
            return FALSE;
        }
        StripNewline(line);
        int sizePx = 0;
        if (std::sscanf(line, "%d", &sizePx) != 1) {
            return FALSE;
        }

        const RwReal recipWidth = 1.0f / static_cast<RwReal>(texWidth);
        const RwReal recipHeight = 1.0f / static_cast<RwReal>(texHeight);

        // 4) glyph array — capacity [esi+0x130] (0 in a fresh object → default 256)
        RwInt32 capacity = pFont->glyphCount > 0 ? pFont->glyphCount : 0x100;
        pFont->refCount = 0; // sub_4CDBC0: dword ptr [esi] = 0
        pFont->glyphs = static_cast<CCFontGlyph *>(
            PoolAlloc(static_cast<size_t>(capacity) * sizeof(CCFontGlyph)));
        if (!pFont->glyphs) {
            return FALSE;
        }
        std::memset(pFont->glyphs, 0, static_cast<size_t>(capacity) * sizeof(CCFontGlyph));

        RwInt32 glyphCount = 0;              // var_30
        RwInt32 lastTop = 0;                 // var_28 (last line's top value)
        RwInt32 lastBottom = 0;              // var_2C (last line's bottom value)
        RwBool  failed = FALSE;              // var_38 == font cursor → success

        while (std::fgets(line, sizeof(line), pFile)) {
            StripNewline(line);

            RwInt32 code, left, top, right, bottom;
            if (std::sscanf(line, "%i %i %i %i %i", &code, &left, &top, &right,
                            &bottom) != 5) {
                continue; // binary: line skipped if not exactly 5 fields
            }
            if (code > 0xFFFF) {
                failed = TRUE; // binary: var_38 = 0 → hard error
                break;
            }

            // extended code paint array — capacity check per glyph
            // (common registration block RegisterGlyphCode)
            if (!RegisterGlyphCode(pFont, code, glyphCount)) {
                failed = TRUE;
                break;
            }

            // UV + aspect — pixel center correction 0.5 (flt_5EF624)
            CCFontGlyph &glyph = pFont->glyphs[glyphCount];
            glyph.u0 = (static_cast<RwReal>(left) + 0.5f) * recipWidth;
            glyph.v0 = (static_cast<RwReal>(top) + 0.5f) * recipHeight;
            glyph.u1 = (static_cast<RwReal>(right) + 0.5f) * recipWidth;
            glyph.v1 = (static_cast<RwReal>(bottom) + 0.5f) * recipHeight;
            glyph.texture = pTex;
            glyph.aspect = static_cast<RwReal>(right - left) /
                           static_cast<RwReal>(bottom - top);
            glyph.flags = 0;
            lastTop = top;
            lastBottom = bottom;

            ++glyphCount;
            if (glyphCount >= capacity) {
                capacity += 0x20;
                CCFontGlyph *pNewGlyphs = static_cast<CCFontGlyph *>(
                    PoolAlloc(static_cast<size_t>(capacity) * sizeof(CCFontGlyph)));
                if (!pNewGlyphs) {
                    failed = TRUE;
                    break;
                }
                std::memcpy(pNewGlyphs, pFont->glyphs,
                            static_cast<size_t>(glyphCount) * sizeof(CCFontGlyph));
                std::memset(pNewGlyphs + glyphCount, 0,
                            static_cast<size_t>(capacity - glyphCount) *
                                sizeof(CCFontGlyph));
                PoolFree(pFont->glyphs);
                pFont->glyphs = pNewGlyphs;
            }
        }

        if (failed) {
            return FALSE;
        }

        // finalization — sub_4CDBC0 trailing block
        pFont->glyphCount = glyphCount;
        FontFinalizeExtMap(pFont);
        if (s_pExtScratch) {
            // binary: zero the extended paint array with 0xFFFFFFFF
            for (RwInt32 i = 0; i < s_extScratchCap; ++i) {
                s_pExtScratch[i] = -1;
            }
        }
        pFont->spacing = 0.0f; // [esi+0Ch] = 0 — inter-glyph spacing (M1)
        pFont->lineExtent = static_cast<RwReal>(lastBottom - lastTop);
        pFont->sizeScale = pFont->lineExtent != 0.0f
                               ? static_cast<RwReal>(sizePx) / pFont->lineExtent
                               : 0.0f;
        return TRUE;
    }

    // sub_4CD4D0 — font object allocation (0x144 pool + field initializers)
    CCFont *FontAlloc(void) {
        CCFont *pFont = static_cast<CCFont *>(PoolAlloc(sizeof(CCFont)));
        if (!pFont) {
            return 0;
        }
        pFont->refCount = 1;
        pFont->lineExtent = 1.0f; // [esi+4] = 3F800000h
        pFont->sizeScale = 0.0f;
        pFont->spacing = 0.0f; // [esi+0Ch] (written at the end in M1)
        pFont->glyphFlags = 0;
        for (int i = 0; i < 4; ++i) {
            pFont->textures[i] = 0;
        }
        pFont->extMin = 0;
        pFont->extCount = 0;
        pFont->extTable = 0;
        pFont->glyphCount = 0;
        pFont->glyphs = static_cast<CCFontGlyph *>(
            PoolAlloc(0x20 * sizeof(CCFontGlyph))); // sub_51A0A0(0x20, 0x30190)
        pFont->reserved1 = 0;
        pFont->reserved2 = 0;
        // +0x138 — binary offset sub_4CCC40 (per-font draw handler).
        // TODO(Phase 2): wire it up when the sub_4CCC40 body is restored.
        pFont->drawHandler = 0;
        for (int i = 0; i < 128; ++i) {
            pFont->ascii[i] = 0xFFFF; // sub_4CD4D0: rep stosd -1
        }
        if (!pFont->glyphs) {
            PoolFree(pFont);
            return 0;
        }
        std::memset(pFont->glyphs, 0, 0x20 * sizeof(CCFontGlyph));
        return pFont;
    }

    // sub_4CD8C0 — font release
    RwBool FontFree(CCFont *pFont) {
        if (!pFont) {
            return FALSE;
        }
        if (pFont->glyphs) {
            for (RwInt32 i = 0; i < pFont->glyphCount; ++i) {
                if (pFont->glyphs[i].data) {
                    // sub_4D0C20 — glyph contour data (M3: CCFontContour*
                    // subpath chain; never filled in M1)
                    ContourFreeChain(
                        static_cast<CCFontContour *>(pFont->glyphs[i].data));
                    pFont->glyphs[i].data = 0;
                }
            }
            PoolFree(pFont->glyphs);
            pFont->glyphs = 0;
        }
        if (pFont->extTable) {
            PoolFree(pFont->extTable);
            pFont->extTable = 0;
        }
        for (int i = 0; i < 4; ++i) {
            TextureRelease(pFont->textures[i]);
            pFont->textures[i] = 0;
        }
        PoolFree(pFont);
        return TRUE;
    }
} // namespace

// ============================================================================
// FontSystemAllocBuffers (FUN_004CEFD0)
// Binary: engine+0x134(0x2400, 0x40190) → dword_66D5C4;
//         sub_50DBF0(0x78, ...) → dword_66D5C0; return value 1.
// The 0x2400 vertex buffer is for the glyph draw step (sub_4CC690, Phase 2).
// ============================================================================
RwBool FontSystemAllocBuffers(void) {
    if (!s_pVertexBuffer) {
        s_pVertexBuffer = PoolAlloc(0x2400);
    }
    // dword_66D5C0 0x78-sized pool — POD static (TODO: freelist semantics)
    return s_pVertexBuffer != 0;
}

// ============================================================================
// FontSystemInit (FUN_004CD660)
// Binary order: path buffer copy ("fonts\"), 64×quad index
// table, dword_66D5D8=0, sub_515130 list head, dword_66DCC0 = &sub_4CD9A0.
// ============================================================================
RwBool FontSystemInit(void) {
    // index table — binary loop (ecx=3, step 4, 64 blocks)
    RwUInt16 *pOut = &s_QuadIndices[0][0];
    RwUInt32 base = 3;
    for (RwUInt32 block = 0; block < 64; ++block, base += 4) {
        pOut[0] = static_cast<RwUInt16>(base - 3);
        pOut[1] = static_cast<RwUInt16>(base - 1);
        pOut[2] = static_cast<RwUInt16>(base);
        pOut[3] = static_cast<RwUInt16>(base - 3);
        pOut[4] = static_cast<RwUInt16>(base);
        pOut[5] = static_cast<RwUInt16>(base - 2);
        pOut += 6;
    }

    s_pFontListHead = 0; // dword_66D5D8
    if (!s_pListNode) {
        s_pListNode = PoolAlloc(sizeof(RwUInt32)); // counterpart of sub_515130
    }
    ScratchString_Set(kFontPath); // sub_517CF0(dword_66D5CC) — "fonts\"
    s_pLoader = &FontSystemLoad;  // dword_66DCC0 = offset sub_4CD9A0
    return s_pListNode != 0;
}

// ============================================================================
// FontSystemLoad (FUN_004CD9A0)
// Binary: the "fonts\<name>.met" buffer is built, candidates separated by ';' are tried in order
// ; header METRICS1/METRICS2/METRICS3 check is delegated to the parser.
// NOTE (port): the binary copies the dword_66D5CC buffer with engine string operators
// (strcat + ';' loop); the port splits the name list directly —
// the observed behavior (if earlier candidates fail the next is tried) is the same.
// METRICS1/METRICS3 parsers are wired up; METRICS2 (sub_004ce0d0) still blocked in the port.
// ============================================================================
CCFont *FontSystemLoad(const char *pFontList) {
    if (!pFontList || !pFontList[0]) {
        return 0;
    }

    CCFont *pFont = FontAlloc(); // sub_4CD4D0
    if (!pFont) {
        return 0;
    }

    const char *pCandidate = pFontList;
    for (;;) {
        char name[192];
        size_t nameLen = 0;
        while (pCandidate[0] && pCandidate[0] != ';' &&
               nameLen + 1 < sizeof(name)) {
            name[nameLen++] = *pCandidate++;
        }
        name[nameLen] = '\0';

        char path[sizeof(kFontPath) + sizeof(name) + 8];
        std::snprintf(path, sizeof(path), "%s%s.met", kFontPath, name);

        std::FILE *pFile = std::fopen(path, "r");
        if (pFile) {
            char header[64] = {0};
            RwBool parsed = FALSE;
            if (std::fgets(header, sizeof(header), pFile)) {
                StripNewline(header);
                if (std::strcmp(header, "METRICS1") == 0) {
                    parsed = ParseMetrics1(pFont, pFile);
                } else if (std::strcmp(header, "METRICS3") == 0) {
                    parsed = ParseMetrics3(pFont, pFile);
                } else {
                    // TODO(Phase 4.4b): METRICS2 (FUN_004ce0d0) - raster
                    // based on pixel scanning; blocked in the port (ROADMAP.md)
                    parsed = FALSE;
                }
            }
            std::fclose(pFile);
            if (parsed) {
                return pFont;
            }
        }

        if (pCandidate[0] != ';') {
            break; // NUL — candidate list exhausted (binary: jz exit)
        }
        ++pCandidate; // ';' — try the next candidate (fallback chain)
    }

    FontFree(pFont);
    return 0;
}

// ---------------------------------------------------------------------------
// FontSystem_LoadFromStream (FUN_004CC690) + LoadContourFromStream
// (FUN_004D11D0) — binary stream font loader (sub_491700 chain).
//
// Stream: type tag ((tag>>24) >= 1), font header (refCount SELECTS THE
// PATH: 0 → texture, ≠0 → vector), extTable raw, ascii 0x100, glyphs.
// DEVIATIONS: texture context save/restore (sub_5154C0/5154A0) no-op; prebaked
// stroke records (0x18) are converted to path points (type0 → moveto,
// others → lineto — curveto already flattened during parse); normal/length
// recomputed at render (same deviation as Phase 3b); contour[+0x10] vestigial;
// texture index [0,3] + texCount ≤ 4 are port validations (the binary
// does not validate); drawHandler = 0 (single DrawText path).
// ---------------------------------------------------------------------------

// sub_4D11D0 — type-5 contour record: 0x10 header {version=1, pointCount,
// flags, extra} + pointCount×0x18 prebaked stroke records (type/x/y/normal/
// accLen); flags&4 → findChunk(0x1A1) + chained sub-contour ([head+0x14]).
static CCFontContour *LoadContourFromStream(CCFontStream *pStream) {
    RwUInt32 header[4];
    if (!pStream->readExact(pStream, header, 0x10)) {
        return 0;
    }
    if (header[0] != 1) {
        return 0; // binary: dec eax / jnz fail — version 1
    }
    const RwUInt32 pointCount = header[1];
    const RwUInt32 flags = header[2];
    // header[3] → binary node+0x10 (var_88 — to strokeWidth at render
    // appended; the port assumes 0, read but ignored)
    CCFontContour *pContour = ContourAlloc();
    if (!pContour) {
        return 0;
    }
    pContour->closed = ((flags >> 1) & 1) ? TRUE : FALSE; // bit1

    for (RwUInt32 i = 0; i < pointCount; ++i) {
        struct StrokeRecord {
            RwUInt32 type;
            RwReal x, y;
            RwReal nx, ny;
            RwReal accLen;
        } rec;
        if (!pStream->readExact(pStream, &rec, 0x18)) {
            ContourFreeChain(pContour);
            return 0;
        }
        // type0 → moveto; others (1/2 — curveto already flattened)
        // → lineto; normal/accLen recomputed at render
        const RwReal f[2] = {rec.x, rec.y};
        if (!ContourAddPoint(&pContour, (rec.type == 0) ? 0u : 1u, f, 2)) {
            ContourFreeChain(pContour);
            return 0;
        }
    }
    if (flags & 4) {
        // chained sub-contour — binary: [head+0x14] = child
        if (!pStream->findChunk(pStream, 0x1A1)) {
            ContourFreeChain(pContour);
            return 0;
        }
        CCFontContour *pChild = LoadContourFromStream(pStream);
        if (!pChild) {
            ContourFreeChain(pContour);
            return 0;
        }
        pContour->pNext = pChild;
    }
    return pContour;
}

CCFont *FontSystem_LoadFromStream(CCFontStream *pStream) {
    if (!pStream || !pStream->read || !pStream->readExact ||
        !pStream->findChunk) {
        return 0;
    }
    CCFont *pFont = FontAlloc(); // sub_4CD4D0
    if (!pFont) {
        return 0;
    }

    RwUInt32 tag = 0;
    if (!pStream->read(pStream, &tag, 4)) {
        FontFree(pFont);
        return 0;
    }
    if ((static_cast<RwInt32>(tag) >> 24) < 1) {
        FontFree(pFont); // binary: shr 18h / jge — type byte ≥ 1
        return 0;
    }
    // font header — refCount comes from the stream and selects the path
    if (!pStream->read(pStream, &pFont->refCount, 4) ||
        !pStream->read(pStream, &pFont->lineExtent, 4) ||
        !pStream->read(pStream, &pFont->sizeScale, 4) ||
        !pStream->read(pStream, &pFont->spacing, 4) ||
        !pStream->read(pStream, &pFont->glyphFlags, 4) ||
        !pStream->read(pStream, &pFont->extMin, 4) ||
        !pStream->read(pStream, &pFont->glyphCount, 4) ||
        !pStream->read(pStream, &pFont->extCount, 4)) {
        FontFree(pFont);
        return 0;
    }
    if (pFont->glyphCount < 0) { // port robustness (the binary does not validate)
        FontFree(pFont);
        return 0;
    }
    // extTable — raw (extCount ×2 bytes); finalize unnecessary (map is ready)
    if (pFont->extCount > 0) {
        pFont->extTable = static_cast<RwUInt16 *>(PoolAlloc(
            static_cast<size_t>(pFont->extCount) * sizeof(RwUInt16)));
        if (!pFont->extTable) {
            FontFree(pFont);
            return 0;
        }
        std::memset(pFont->extTable, 0,
                    static_cast<size_t>(pFont->extCount) * sizeof(RwUInt16));
        if (!pStream->readExact(pStream, pFont->extTable,
                                 pFont->extCount * 2)) {
            FontFree(pFont);
            return 0;
        }
    }
    // ascii map — 0x100 bytes raw (128 × RwUInt16)
    if (!pStream->read(pStream, pFont->ascii, 0x100)) {
        FontFree(pFont);
        return 0;
    }
    // glyph array — glyphCount × 0x20 (sub_51A230 resize; FontAlloc's
    // 32-element preallocation is left as-is)
    const RwInt32 glyphCount = pFont->glyphCount;
    PoolFree(pFont->glyphs);
    pFont->glyphs = 0;
    if (glyphCount > 0) {
        pFont->glyphs = static_cast<CCFontGlyph *>(PoolAlloc(
            static_cast<size_t>(glyphCount) * sizeof(CCFontGlyph)));
        if (!pFont->glyphs) {
            pFont->glyphCount = 0;
            FontFree(pFont);
            return 0;
        }
        std::memset(pFont->glyphs, 0,
                    static_cast<size_t>(glyphCount) * sizeof(CCFontGlyph));
    }

    if (pFont->refCount == 0) {
        // ----- texture path (loc_4CC993) -----
        // sub_5154C0/5154A0 texture context — port ReadTexture independent (no-op)
        for (RwInt32 i = 0; i < glyphCount; ++i) {
            CCFontGlyph &glyph = pFont->glyphs[i];
            if (!pStream->readExact(pStream, &glyph.u0, 0x10) || // u0,v0,u1,v1
                !pStream->read(pStream, &glyph.aspect, 4) ||
                !pStream->readExact(pStream, &glyph.flags, 1)) {
                FontFree(pFont);
                return 0;
            }
        }
        RwUInt32 texCount = 0;
        if (!pStream->read(pStream, &texCount, 4)) {
            FontFree(pFont);
            return 0;
        }
        if (texCount > 4) { // port robustness (the binary does not validate textures[4])
            FontFree(pFont);
            return 0;
        }
        for (RwUInt32 i = 0; i < texCount; ++i) {
            char name[0x21] = {0}; // 0x20 + NUL safety
            if (!pStream->readExact(pStream, name, 0x20)) {
                FontFree(pFont);
                return 0;
            }
            RwInt32 texW = 0;
            RwInt32 texH = 0;
            pFont->textures[i] = ReadTexture(name, 0, &texW, &texH);
            if (!pFont->textures[i]) {
                FontFree(pFont);
                return 0;
            }
        }
        for (RwInt32 i = 0; i < glyphCount; ++i) {
            CCFontGlyph &glyph = pFont->glyphs[i];
            const int idx =
                static_cast<int>(static_cast<RwInt8>(glyph.flags));
            if (idx < 0 || idx > 3 || !pFont->textures[idx]) {
                FontFree(pFont); // binary: [tex]==0 → fail; index[port]
                return 0;
            }
            glyph.texture = pFont->textures[idx];
        }
        pFont->reserved1 = 0;
        pFont->reserved2 = 0;
        pFont->drawHandler = 0; // sub_4CCC40 - port: DrawText single path
    } else {
        // ----- vector path -----
        for (RwInt32 i = 0; i < glyphCount; ++i) {
            CCFontGlyph &glyph = pFont->glyphs[i];
            RwUInt32 scratch = 0;
            RwUInt32 type = 0;
            if (!pStream->read(pStream, &scratch, 4) || // waste (arg_0)
                !pStream->read(pStream, &glyph.aspect, 4) ||
                !pStream->read(pStream, &type, 4)) {
                FontFree(pFont);
                return 0;
            }
            if (type == 5) {
                if (!pStream->findChunk(pStream, 0x1A1)) {
                    FontFree(pFont);
                    return 0;
                }
                CCFontContour *pContour = LoadContourFromStream(pStream);
                if (!pContour) {
                    FontFree(pFont);
                    return 0;
                }
                glyph.data = pContour;
                if (!pStream->read(pStream, &scratch, 4)) {
                    FontFree(pFont); // post-contour waste read
                    return 0;
                }
            }
            if (!pStream->read(pStream, &scratch, 4)) {
                FontFree(pFont); // common waste (loc_4CC939)
                return 0;
            }
        }
        pFont->drawHandler = 0; // sub_4CCAF0 - port: DrawText single path
    }
    return pFont;
}

// ============================================================================
// FontSystemGet (FUN_004CEE90)
// Binary: searches the dword_66D5D8 list with strcmp (engine+0x114); if not found
// it loads via dword_66DCC0 (FontSystemLoad) and takes a node from the dword_66D5D4 pool
// prepending it (node: +0 name strdup, +4 font, +8 old head).
// ============================================================================
CCFontNode *FontSystemGet(const char *pFontName) {
    if (!pFontName) {
        return 0;
    }

    for (CCFontNode *pNode = s_pFontListHead; pNode; pNode = pNode->pNext) {
        if (pNode->pName && std::strcmp(pFontName, pNode->pName) == 0) {
            return pNode;
        }
    }

    CCFont *pFont = s_pLoader(pFontName);
    if (!pFont) {
        return 0;
    }

    CCFontNode *pNode = static_cast<CCFontNode *>(PoolAlloc(sizeof(CCFontNode)));
    if (!pNode) {
        FontFree(pFont);
        return 0;
    }
    size_t nameLen = std::strlen(pFontName) + 1;
    pNode->pName = static_cast<char *>(PoolAlloc(nameLen));
    if (!pNode->pName) {
        PoolFree(pNode);
        FontFree(pFont);
        return 0;
    }
    std::memcpy(pNode->pName, pFontName, nameLen);
    pNode->pFont = pFont;
    pNode->pNext = s_pFontListHead;
    s_pFontListHead = pNode;
    return pNode;
}

// ============================================================================
// FontSystemGetGlyph
// Its binary counterpart is in the draw path (sub_4CC690, Phase 2); the field layout is
// read directly by sub_4CD4D0/sub_4CDBC0:
//   0..127  → font+0x24 (uint16[128], 0xFFFF = none)
//   >=0x80  → font+0x12C table (extMin range, 0xFFFF = none)
// ============================================================================
const CCFontGlyph *FontSystemGetGlyph(const CCFont *pFont, RwUInt32 code) {
    if (!pFont || !pFont->glyphs) {
        return 0;
    }

    RwUInt16 index = 0xFFFF;
    if (code < 0x80) {
        index = pFont->ascii[code];
    } else if (pFont->extTable &&
               static_cast<RwInt32>(code) >= pFont->extMin &&
               static_cast<RwInt32>(code) < pFont->extMin + pFont->extCount) {
        index = pFont->extTable[code - pFont->extMin];
    }

    if (index == 0xFFFF || index >= pFont->glyphCount) {
        return 0;
    }
    return &pFont->glyphs[index];
}

// ============================================================================
// FontSystemTeardown (FUN_004CD580 — __ioterm+62)
// Binary: name list node chain (name free + sub_4CD8C0 + pool free),
// sub_5151D0 list head, sub_50DC00 pool ×2, dword_66DE1C free,
// dword_66D5CC path buffer free; all zeroed out.
// ============================================================================
void FontSystemTeardown(void) {
    CCFontNode *pNode = s_pFontListHead;
    while (pNode) {
        CCFontNode *pNext = pNode->pNext;
        if (pNode->pName) {
            PoolFree(pNode->pName);
        }
        if (pNode->pFont) {
            FontFree(pNode->pFont);
        }
        PoolFree(pNode);
        pNode = pNext;
    }
    s_pFontListHead = 0;

    if (s_pListNode) {
        PoolFree(s_pListNode); // sub_5151D0
        s_pListNode = 0;
    }
    if (s_pExtScratch) {
        PoolFree(s_pExtScratch); // dword_66DE1C
        s_pExtScratch = 0;
        s_extScratchCap = 0;
    }
    // path buffer (dword_66D5CC) is a fixed string in the port → no-op;
    // pools (dword_66D5C8/D4) POD → no-op (TODO: freelist semantics)
}

// ============================================================================
// FontSystemShutdown (FUN_004CEF90 — __ioterm+67)
// Binary: sub_50DC00(dword_66D5C0) pool + engine+0x138(dword_66D5C4) free.
// ============================================================================
void FontSystemShutdown(void) {
    if (s_pVertexBuffer) {
        PoolFree(s_pVertexBuffer); // dword_66D5C4
        s_pVertexBuffer = 0;
    }
    // dword_66D5C0 0x78-sized pool POD → no-op
}

// ============================================================================
// FontSystem_SetTextureReader — sub_5154D0 test injection (NULL = default
// RwTextureRead route).
// ============================================================================
void FontSystem_SetTextureReader(FontTextureReader pReader) {
    s_pTextureReader = pReader;
}

// ============================================================================
// FontSystem_GetQuadIndices — unk_66D2C4..0x66D5C4 table (produced by sub_4CD660)
// ============================================================================
const RwUInt16 (*FontSystem_GetQuadIndices(void))[6] {
    return &s_QuadIndices[0];
}

namespace
{
    // Last batch vertex count — FontSystem_GetDrawVertices test access
    // (batch flush validation of the sub_4CCC40 port).
    RwInt32 s_lastBatchVerts = 0;

    // sub_4CCC40 batch flush — binary sub_518EC0 (vertex range) +
    // sub_518FC0 (indices unk_66D2C0, type 3) + sub_518F90 (draw).
    // Port: direct RwIm2DRenderIndexedPrimitive (rwPRIMTYPETRILIST).
    // The batch quad starts at 0 on every flush → the first quadCount
    // block of the table is used.
    void FlushQuadBatch(RwInt32 quadCount) {
        if (quadCount <= 0) {
            return;
        }
        const RwUInt16(*quadIdx)[6] = FontSystem_GetQuadIndices();
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, &quadIdx[0][0],
                                     quadCount * 6);
        s_lastBatchVerts = quadCount * 4;
    }

    // equivalent of fistp + or ah,0Ch (round toward zero); the port clamps 0..255.
    RwUInt8 ChopColorByte(RwReal value) {
        int v = static_cast<int>(value);
        if (v < 0) {
            v = 0;
        }
        if (v > 255) {
            v = 255;
        }
        return static_cast<RwUInt8>(v);
    }

    void FillTextVertex(RwIm2DVertex *pV, RwReal x, RwReal y, RwReal u,
                        RwReal v) {
        RwIm2DVertexSetScreenX(pV, x);
        RwIm2DVertexSetScreenY(pV, y);
        RwIm2DVertexSetScreenZ(
            pV, 1.0f); // dword_66D7A0 — sub_4CC1F0 initial value
        RwIm2DVertexSetRecipCameraZ(pV, 1.0f); // rhw - TODO Phase 4.4b
        pV->u = u;
        pV->v = v;
    }

    // Lighting path (+0x64&1): color = ambient + light x gradient (binary
    // sub_4CCC40 V0/V2 taban +0x30/+0x48, V1/V3 tepe +0x00/+0x18).
    // Solid path: color is the material bytes (+0x60..63); material NULL → white.
    void GetVertexColor(const CCFontMaterial *pMat, RwBool lit, RwBool bottom,
                        RwReal gradient, RwUInt8 *pR, RwUInt8 *pG,
                        RwUInt8 *pB, RwUInt8 *pA) {
        if (!lit) {
            *pR = pMat ? pMat->red   : 255;
            *pG = pMat ? pMat->green : 255;
            *pB = pMat ? pMat->blue  : 255;
            *pA = pMat ? pMat->alpha : 255;
            return;
        }
        const RwReal *pAmb = bottom ? pMat->bottomAmbient : pMat->topAmbient;
        const RwReal *pLight = bottom ? pMat->bottomLight : pMat->topLight;
        *pR = ChopColorByte(pAmb[0] + pLight[0] * gradient);
        *pG = ChopColorByte(pAmb[1] + pLight[1] * gradient);
        *pB = ChopColorByte(pAmb[2] + pLight[2] * gradient);
        *pA = ChopColorByte(pAmb[3] + pLight[3] * gradient);
    }

    // ---------------------------------------------------------------------------
    // Vector contour raster (sub_4D54C0 / thunk sub_4D54B0) — stroke quad
    // strips. The binary writes each contour record (0x18: base x/y + unit normal
    // + accumulated length — produced in parse by sub_4CFE80/4CFF40/4D0040) into the grid
    // as base ∓ normal×w pairs; w = material strokeWidth (+0x70) +
    // contour[+0x10] (always 0 in the port). Strip = TRIANGLEFAN type 4; the port
    // converts it to TRIANGLELIST. gradient grad = accumulated/total path length
    // (var_64 = 1/last record length).
    // Deviations (documented): instead of miter joints, segment normals (sub_4CFF40
    // blends the previous normal when the next segment arrives — the port
    // does not); instead of flattening during parse, bezier is flattened at render with a fixed 8 sub-
    // segments; scaled in screen space instead of the matrix stack
    // (T(pos+pen)×S(scale)); z=0/rhw=1; texture format states (1/9/2)
    // are skipped.
    // ---------------------------------------------------------------------------
    const RwInt32 kStrokeMaxVerts = 254; // binary: cmp eax,0FEh
    const RwInt32 kStrokeCurveSegs = 8;  // bezier sub-segment (port constant)

    struct StrokeSeg {
        RwReal x0, y0, x1, y1;
    };

    StrokeSeg s_SegScratch[512];
    RwUInt16  s_StrokeIndices[kStrokeMaxVerts * 6];

    struct StrokeBatch {
        RwIm2DVertex *pVerts;
        RwInt32  vertCount;  // var_80
        RwInt32  pairCount;
        RwReal   scale;
        RwReal   posX;
        RwReal   posY;
        RwReal   w;                       // var_84 — strokeWidth + contour[+0x10]
        const CCFontMaterial *pMaterial;
    };

    // sub_518EC0 + sub_518FC0 + sub_518F90 — strip flush (type 4 → port
    // indexed TRIANGLELIST).
    void FlushStrokeStrip(StrokeBatch *pBatch) {
        if (pBatch->pairCount <= 1) {
            pBatch->vertCount = 0;
            pBatch->pairCount = 0;
            return;
        }
        RwInt32 idx = 0;
        for (RwInt32 i = 0; i + 1 < pBatch->pairCount; ++i) {
            const RwUInt16 a = static_cast<RwUInt16>(i * 2);
            const RwUInt16 b = static_cast<RwUInt16>(i * 2 + 1);
            const RwUInt16 c = static_cast<RwUInt16>(i * 2 + 2);
            const RwUInt16 d = static_cast<RwUInt16>(i * 2 + 3);
            s_StrokeIndices[idx++] = a;
            s_StrokeIndices[idx++] = b;
            s_StrokeIndices[idx++] = c;
            s_StrokeIndices[idx++] = b;
            s_StrokeIndices[idx++] = d;
            s_StrokeIndices[idx++] = c;
        }
        RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, s_StrokeIndices, idx);
        s_lastBatchVerts = pBatch->vertCount;
        pBatch->vertCount = 0;
        pBatch->pairCount = 0;
    }

    // one stroke pair: A = base − n×w (top), B = base + n×w (bottom);
    // transform T(pos)×S(scale) (counterpart of the binary matrix stack).
    void EmitStrokePair(StrokeBatch *pBatch, RwReal bx, RwReal by, RwReal nx,
                        RwReal ny, RwReal w, RwReal grad,
                        const CCFontMaterial *pMat) {
        if (pBatch->vertCount + 2 > kStrokeMaxVerts) {
            FlushStrokeStrip(pBatch);
        }
        RwIm2DVertex *pA = &pBatch->pVerts[pBatch->vertCount];
        RwIm2DVertex *pB = pA + 1;

        const RwReal ax = (bx - nx * w) * pBatch->scale + pBatch->posX;
        const RwReal ay = (by - ny * w) * pBatch->scale + pBatch->posY;
        const RwReal cx = (bx + nx * w) * pBatch->scale + pBatch->posX;
        const RwReal cy = (by + ny * w) * pBatch->scale + pBatch->posY;

        FillTextVertex(pA, ax, ay, 0.0f, 0.0f);
        FillTextVertex(pB, cx, cy, 0.0f, 0.0f);

        const RwBool lit =
            (pMat != 0 && (pMat->flags & 1)) ? TRUE : FALSE;
        RwUInt8 r, g, b, a;
        GetVertexColor(pMat, lit, FALSE, grad, &r, &g, &b, &a);
        RwIm2DVertexSetIntRGBA(pA, r, g, b, a);
        GetVertexColor(pMat, lit, TRUE, grad, &r, &g, &b, &a);
        RwIm2DVertexSetIntRGBA(pB, r, g, b, a);

        if (pMat && (pMat->flags & 2)) {
            // textured stroke: u = uStart + uDelta×grad (sub_4D54C0)
            pA->u = pMat->uTop + pMat->uTopDelta * grad;
            pA->v = pMat->vTop + pMat->vTopDelta * grad;
            pB->u = pMat->uBottom + pMat->uBottomDelta * grad;
            pB->v = pMat->vBottom + pMat->vBottomDelta * grad;
        }

        pBatch->vertCount += 2;
        ++pBatch->pairCount;
    }

    void AddStrokeSeg(RwInt32 *pCount, RwReal x0, RwReal y0, RwReal x1,
                      RwReal y1) {
        if (*pCount >= static_cast<RwInt32>(sizeof(s_SegScratch) /
                                            sizeof(s_SegScratch[0]))) {
            return; // overflow — fixed buffer (documented limit)
        }
        StrokeSeg &seg = s_SegScratch[(*pCount)++];
        seg.x0 = x0;
        seg.y0 = y0;
        seg.x1 = x1;
        seg.y1 = y1;
    }

    // single contour (subpath) — strip generation + flush; w/material in the batch
    void RenderContourStrip(const CCFontContour *pContour,
                            StrokeBatch *pBatch) {
        if (pContour->pointCount <= 0) {
            return;
        }
        const RwReal w = pBatch->w;
        const CCFontMaterial *pMat = pBatch->pMaterial;
        // start — each subpath array in the binary begins with a type-0 moveto record
        // (sub_4CFE80)
        const RwReal movetoX = pContour->points[0].x;
        const RwReal movetoY = pContour->points[0].y;
        RwReal penX = movetoX;
        RwReal penY = movetoY;
        RwInt32 segCount = 0;

        for (RwInt32 i = 1; i < pContour->pointCount; ++i) {
            const CCFontPathPoint &pt = pContour->points[i];
            if (pt.type == 1) { // lineto
                AddStrokeSeg(&segCount, penX, penY, pt.x, pt.y);
                penX = pt.x;
                penY = pt.y;
            } else if (pt.type == 2) { // curveto — fixed K sub-segment
                const RwReal p0x = penX, p0y = penY;
                const RwReal c1x = pt.x, c1y = pt.y;
                const RwReal c2x = pt.cx1, c2y = pt.cy1;
                const RwReal p1x = pt.cx2, p1y = pt.cy2;
                RwReal prevX = p0x, prevY = p0y;
                for (RwInt32 s = 1; s <= kStrokeCurveSegs; ++s) {
                    const RwReal t = static_cast<RwReal>(s) /
                                     static_cast<RwReal>(kStrokeCurveSegs);
                    const RwReal u = 1.0f - t;
                    const RwReal x = u * u * u * p0x + 3.0f * u * u * t * c1x +
                                     3.0f * u * t * t * c2x + t * t * t * p1x;
                    const RwReal y = u * u * u * p0y + 3.0f * u * u * t * c1y +
                                     3.0f * u * t * t * c2y + t * t * t * p1y;
                    AddStrokeSeg(&segCount, prevX, prevY, x, y);
                    prevX = x;
                    prevY = y;
                }
                penX = p1x;
                penY = p1y;
            }
        }
        if (pContour->closed && segCount > 0) {
            // closepath — sub_4D0220 appends a closing segment
            AddStrokeSeg(&segCount, penX, penY, movetoX, movetoY);
        }
        if (segCount <= 0) {
            return;
        }

        // lengths + total (var_64 = 1/last record length)
        RwReal total = 0.0f;
        for (RwInt32 i = 0; i < segCount; ++i) {
            const StrokeSeg &seg = s_SegScratch[i];
            const RwReal dx = seg.x1 - seg.x0;
            const RwReal dy = seg.y1 - seg.y0;
            total += std::sqrt(dx * dx + dy * dy);
        }
        const RwReal invTotal = (total > 0.0f) ? (1.0f / total) : 0.0f;

        // moveto record — normal 0, accumulation 0 (sub_4CFE80 first record)
        EmitStrokePair(pBatch, movetoX, movetoY, 0.0f, 0.0f, w, 0.0f, pMat);
        RwReal acc = 0.0f;
        for (RwInt32 i = 0; i < segCount; ++i) {
            const StrokeSeg &seg = s_SegScratch[i];
            const RwReal dx = seg.x1 - seg.x0;
            const RwReal dy = seg.y1 - seg.y0;
            const RwReal len = std::sqrt(dx * dx + dy * dy);
            acc += len;
            RwReal nx = 0.0f, ny = 0.0f;
            if (len > 0.0f) {
                nx = dy / len;   // sub_4CFF40: n = (dy, −dx) / len
                ny = -dx / len;
            }
            EmitStrokePair(pBatch, seg.x1, seg.y1, nx, ny, w,
                           acc * invTotal, pMat);
        }
        FlushStrokeStrip(pBatch);
    }

    // chain - sub_4D54C0 [contour+0x14] recursion
    void RenderContourChain(const CCFontContour *pContour,
                            StrokeBatch *pBatch) {
        for (const CCFontContour *pC = pContour; pC; pC = pC->pNext) {
            RenderContourStrip(pC, pBatch);
        }
    }

    // glyph entry — sub_4D54B0 (thunk) + sub_4D54C0 entry: material w,
    // texture state, vertex buffer; chained recursion. Called by DrawText,
    // advance applied in DrawText (penX += aspect + spacing).
    void RenderContourGlyph(const CCFontGlyph *pGlyph, RwReal scale,
                            RwReal posX, RwReal posY,
                            const CCFontMaterial *pMaterial) {
        const CCFontContour *pContour =
            static_cast<const CCFontContour *>(pGlyph->data);
        if (!pContour) {
            return;
        }
        if (pMaterial && pMaterial->texture) {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                             (void *)RwTextureGetRaster(pMaterial->texture));
        }
        StrokeBatch batch;
        batch.pVerts = static_cast<RwIm2DVertex *>(s_pVertexBuffer);
        batch.vertCount = 0;
        batch.pairCount = 0;
        batch.scale = scale;
        batch.posX = posX;
        batch.posY = posY;
        // w = mat[+0x70] + contour[+0x10] (sub_4D54C0 var_88 — 0 in the port)
        batch.w = pMaterial ? pMaterial->strokeWidth : 0.0f;
        batch.pMaterial = pMaterial;
        RenderContourChain(pContour, &batch);
    }
} // namespace

// ============================================================================
// FontSystem_MeasureText (FUN_004CD7E0)
// Binary: glyph lookup (ASCII +0x24 / ext +0x124..); missing code skipped via js
// skipped; for found glyphs total += glyph.aspect + [font+0x0C] (spacing);
// return = total × arg_8 (scale). If glyphFlags&2 the text is UTF-16LE — unit
// 2 bytes ((p[1]&mask)<<8|p[0], mask=0xFF), cursor stops on signed <=0.
// ============================================================================
RwReal FontSystem_MeasureText(const CCFont *pFont, const char *pText,
                              RwReal scale) {
    if (!pFont || !pText) {
        return 0.0f;
    }
    const RwBool wide = (pFont->glyphFlags & 2) != 0; // binary: test al,2
    const int step = wide ? 2 : 1;
    RwReal total = 0.0f;
    for (const char *p = pText;; p += step) {
        RwUInt32 code;
        if (wide) {
            code = (static_cast<RwUInt32>(
                        static_cast<unsigned char>(p[1]))
                    << 8) |
                   static_cast<RwUInt32>(static_cast<unsigned char>(p[0]));
        } else {
            code = static_cast<RwUInt32>(static_cast<unsigned char>(p[0]));
        }
        if (static_cast<RwInt32>(code) <= 0) {
            break; // binary: or ecx,eax / jle — NUL unit (signed)
        }
        const CCFontGlyph *pGlyph = FontSystemGetGlyph(pFont, code);
        if (!pGlyph) {
            continue; // binary: or ecx,ecx / jl — no advance
        }
        total += pGlyph->aspect + pFont->spacing;
    }
    return total * scale;
}

// ============================================================================
// FontSystem_DrawText (FUN_004CCC40 — font->drawHandler)
// Binary flow: measure → invW; per-unit glyph lookup, texture change/63 quad
// flush, unit-space quad (y ∈ {0,1}), UV (u0,v1)(u0,v0)(u1,v1)(u1,v0),
// advance pen += aspect + spacing; final *pPos[0] += pen × scale.
// Deviation: scaled directly in screen space instead of the matrix stack (sub_4CC2F0/4CC4D0/4CC480)
// ; z/rhw fixed (Phase 4.4b TODO). glyphFlags&2 → UTF-16LE
// units (same as MeasureText). Textureless (M3 vector) glyphs draw a contour
// stroke strip instead of a quad (sub_4D54C0 port — RenderContourGlyph).
// ============================================================================
RwReal FontSystem_DrawText(const CCFont *pFont, const char *pText,
                           RwReal scale, RwReal *pPosX, RwReal *pPosY,
                           const CCFontMaterial *pMaterial) {
    if (!pFont || !pText || !pPosX || !pPosY) {
        return 0.0f;
    }
    if (!s_pVertexBuffer || !pFont->glyphs) {
        return 0.0f; // no drawing without AllocBuffers
    }

    RwIm2DVertex *pVerts = static_cast<RwIm2DVertex *>(s_pVertexBuffer);
    const RwInt32 kFlushQuads = 63; // binary: cmp ebx,17Ah (378 indices)
    const RwBool lit = (pMaterial != 0 && (pMaterial->flags & 1)) ? TRUE : FALSE;

    // invW = 1 / measure(font, text, 1.0f) - degrade normalization
    const RwReal textWidth = FontSystem_MeasureText(pFont, pText, 1.0f);
    const RwReal invW = (textWidth != 0.0f) ? (1.0f / textWidth) : 0.0f;

    const RwBool wide = (pFont->glyphFlags & 2) != 0; // binary: test al,2
    const int step = wide ? 2 : 1;

    RwReal penX = 0.0f;
    RwInt32 quadCount = 0;
    RwTexture *pCurTexture = 0;
    s_lastBatchVerts = 0;

    for (const char *p = pText;; p += step) {
        RwUInt32 code;
        if (wide) {
            code = (static_cast<RwUInt32>(
                        static_cast<unsigned char>(p[1]))
                    << 8) |
                   static_cast<RwUInt32>(static_cast<unsigned char>(p[0]));
        } else {
            code = static_cast<RwUInt32>(static_cast<unsigned char>(p[0]));
        }
        if (static_cast<RwInt32>(code) <= 0) {
            break; // binary: or eax,edx / jg — NUL unit (signed)
        }
        const CCFontGlyph *pGlyph = FontSystemGetGlyph(pFont, code);
        if (!pGlyph) {
            continue;
        }

        if (!pGlyph->texture) {
            // vector glyph (METRICS3, sub_4CCAF0 → sub_4D54C0) — contour
            // stroke strip; the quad batch does not belong to this pass (M3 fonts
            // have no textures) — flush for safety, then raster + advance.
            FlushQuadBatch(quadCount);
            quadCount = 0;
            RenderContourGlyph(pGlyph, scale, *pPosX + penX * scale,
                               *pPosY, pMaterial);
            penX += pGlyph->aspect + pFont->spacing;
            continue;
        }

        if (pCurTexture != pGlyph->texture) {
            FlushQuadBatch(quadCount); // binary: texture change → flush
            quadCount = 0;
            pCurTexture = pGlyph->texture;
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER,
                             (void *)RwTextureGetRaster(pCurTexture));
        } else if (quadCount >= kFlushQuads) {
            FlushQuadBatch(quadCount); // binary: ebx > 0x17A → flush
            quadCount = 0;
        }

        const RwReal x0 = *pPosX + penX * scale;
        const RwReal x1 = *pPosX + (penX + pGlyph->aspect) * scale;
        const RwReal y0 = *pPosY;          // unit y = 0
        const RwReal y1 = *pPosY + scale;  // unit y = 1 × scale

        RwIm2DVertex *pV0 = &pVerts[quadCount * 4 + 0];
        RwIm2DVertex *pV1 = &pVerts[quadCount * 4 + 1];
        RwIm2DVertex *pV2 = &pVerts[quadCount * 4 + 2];
        RwIm2DVertex *pV3 = &pVerts[quadCount * 4 + 3];

        // UV order matches the binary sub_4CCC40 exactly:
        // V0(u0,v1) V1(u0,v0) V2(u1,v1) V3(u1,v0)
        RwUInt8 r, g, b, a;
        const RwReal gradL = penX * invW;
        const RwReal gradR = (penX + pGlyph->aspect) * invW;

        GetVertexColor(pMaterial, lit, TRUE, gradL, &r, &g, &b, &a);
        FillTextVertex(pV0, x0, y0, pGlyph->u0, pGlyph->v1);
        RwIm2DVertexSetIntRGBA(pV0, r, g, b, a);

        GetVertexColor(pMaterial, lit, FALSE, gradL, &r, &g, &b, &a);
        FillTextVertex(pV1, x0, y1, pGlyph->u0, pGlyph->v0);
        RwIm2DVertexSetIntRGBA(pV1, r, g, b, a);

        GetVertexColor(pMaterial, lit, TRUE, gradR, &r, &g, &b, &a);
        FillTextVertex(pV2, x1, y0, pGlyph->u1, pGlyph->v1);
        RwIm2DVertexSetIntRGBA(pV2, r, g, b, a);

        GetVertexColor(pMaterial, lit, FALSE, gradR, &r, &g, &b, &a);
        FillTextVertex(pV3, x1, y1, pGlyph->u1, pGlyph->v0);
        RwIm2DVertexSetIntRGBA(pV3, r, g, b, a);

        ++quadCount;
        penX += pGlyph->aspect + pFont->spacing;
    }

    FlushQuadBatch(quadCount);

    const RwReal advanced = penX * scale;
    *pPosX += advanced; // binary: fmul arg_8; fadd dword ptr [eax]
    return advanced;
}

// ============================================================================
// FontSystem_GetDrawVertices — test access (sub_4CCC40 port batch vertices)
// Last flush's vertex count is s_lastBatchVerts; buffer is AllocBuffers' 0x2400
// area (with multiple batches only the last is valid — vertices are rewritten).
// ============================================================================
const RwIm2DVertex *FontSystem_GetDrawVertices(RwInt32 *pVertexCount) {
    if (pVertexCount) {
        *pVertexCount = s_lastBatchVerts;
    }
    return s_pVertexBuffer ? static_cast<const RwIm2DVertex *>(s_pVertexBuffer)
                           : 0;
}

// Layout check for unit tests — in binary 32-bit (Win32) layout;
// field sizes naturally change with 64-bit pointer width.
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(CCFontGlyph) == 0x20, "CCFontGlyph must be 0x20 bytes");
static_assert(sizeof(CCFont) == 0x144, "CCFont must be 0x144 bytes");
static_assert(sizeof(CCFontNode) == 0x0C, "CCFontNode must be 0x0C bytes");
#endif
