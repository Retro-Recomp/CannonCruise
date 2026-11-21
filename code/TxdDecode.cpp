// ============================================================================
// CannonCruise - TXD (d3d8 IMAGE) decode implementation
// ============================================================================

#include "TxdDecode.h"

#include <cstring>

namespace {

struct Cursor {
    const RwUInt8 *pData;
    RwUInt32 len;
    RwUInt32 pos;
};

RwBool ReadU32(Cursor *pCur, RwUInt32 *pOut) {
    if (pCur->pos + 4 > pCur->len) {
        return FALSE;
    }
    std::memcpy(pOut, pCur->pData + pCur->pos, 4);
    pCur->pos += 4;
    return TRUE;
}

RwBool ReadU16(Cursor *pCur, RwUInt16 *pOut) {
    if (pCur->pos + 2 > pCur->len) {
        return FALSE;
    }
    std::memcpy(pOut, pCur->pData + pCur->pos, 2);
    pCur->pos += 2;
    return TRUE;
}

// [id][len][ver] header; if ver != 0x1803FFFF or overflow, FALSE
RwBool ReadChunkHeader(Cursor *pCur, RwUInt32 *pId, RwUInt32 *pLen) {
    RwUInt32 ver = 0;
    if (!ReadU32(pCur, pId) || !ReadU32(pCur, pLen) || !ReadU32(pCur, &ver)) {
        return FALSE;
    }
    if (ver != 0x1803FFFF || *pLen > pCur->len - pCur->pos) {
        return FALSE;
    }
    return TRUE;
}

void SkipAlign4(Cursor *pCur) {
    while ((pCur->pos & 3) != 0 && pCur->pos < pCur->len) {
        ++pCur->pos;
    }
}

// String chunk body: NUL-terminated, remaining bytes 0/0xBF pad — copied up to NUL
void ReadPaddedString(const RwUInt8 *pBody, RwUInt32 bodyLen, RwChar *pOut,
                      RwUInt32 maxOut) {
    RwUInt32 n = 0;
    for (; n < bodyLen && pBody[n] != '\0' && n < maxOut - 1; ++n) {
        pOut[n] = static_cast<RwChar>(pBody[n]);
    }
    pOut[n] = '\0';
}

} // namespace

RwBool RwTxdDecodeImage(const RwUInt8 *pPayload, RwUInt32 len,
                        RwTxdImage *pOut) {
    if (!pPayload || !pOut || len < 8) {
        return FALSE;
    }
    std::memset(pOut, 0, sizeof(*pOut));

    Cursor cur = {pPayload, len, 0};
    if (!ReadU16(&cur, &pOut->textureCount) ||
        !ReadU16(&cur, &pOut->deviceId)) {
        return FALSE;
    }

    for (RwUInt32 t = 0; t < pOut->textureCount; ++t) {
        if (pOut->count >= rwtxdMAX_TEXTURES) {
            return FALSE;
        }
        RwTxdTexture *pTex = &pOut->textures[pOut->count];

        // ---- per-texture flag ----
        RwUInt32 texFlag = 0;
        if (!ReadU32(&cur, &texFlag)) {
            return FALSE;
        }
        (void)texFlag; // meaning unknown (1 in file)

        // ---- 0x18 raster ----
        RwUInt32 id = 0, clen = 0;
        if (!ReadChunkHeader(&cur, &id, &clen) || id != 0x18) {
            return FALSE;
        }
        {
            Cursor body = {cur.pData, cur.pos + clen, cur.pos};
            RwUInt32 nid = 0, nlen = 0;
            if (!ReadChunkHeader(&body, &nid, &nlen) || nid != 1 ||
                nlen != 16) {
                return FALSE;
            }
            if (!ReadU32(&body, &pTex->width) ||
                !ReadU32(&body, &pTex->height) ||
                !ReadU32(&body, &pTex->depth) ||
                !ReadU32(&body, &pTex->pitch)) {
                return FALSE;
            }
            pTex->pixelBytes = pTex->pitch * pTex->height;
            if (pTex->pixelBytes > body.len - body.pos) {
                return FALSE;
            }
            pTex->pPixels = body.pData + body.pos;
        }
        cur.pos += clen;
        SkipAlign4(&cur);

        // ---- 0x06 texture struct ----
        if (!ReadChunkHeader(&cur, &id, &clen) || id != 0x06) {
            return FALSE;
        }
        {
            Cursor body = {cur.pData, cur.pos + clen, cur.pos};
            RwUInt32 nid = 0, nlen = 0;
            // nested id=1: filterFlags
            if (!ReadChunkHeader(&body, &nid, &nlen) || nid != 1 ||
                nlen != 4 || !ReadU32(&body, &pTex->filterFlags)) {
                return FALSE;
            }
            // nested id=2: name
            if (!ReadChunkHeader(&body, &nid, &nlen) || nid != 2 ||
                nlen > body.len - body.pos) {
                return FALSE;
            }
            ReadPaddedString(body.pData + body.pos, nlen, pTex->name,
                             rwtxdMAX_NAME);
            body.pos += nlen;
            // nested id=2: maskName
            if (!ReadChunkHeader(&body, &nid, &nlen) || nid != 2 ||
                nlen > body.len - body.pos) {
                return FALSE;
            }
            ReadPaddedString(body.pData + body.pos, nlen, pTex->maskName,
                             rwtxdMAX_NAME);
            // body tail: [id=3][0][ver] terminator — the enclosed area is skipped
        }
        cur.pos += clen;
        SkipAlign4(&cur);
        ++pOut->count;
    }
    return pOut->count == pOut->textureCount;
}

namespace {

// Is the texture size a power of 2 (8..512)?
bool IsPow2Size(RwUInt32 v) {
    return v == 8 || v == 16 || v == 32 || v == 64 || v == 128 ||
           v == 256 || v == 512;
}

// Printable texture name? ([A-Za-z][A-Za-z0-9_]{1,..}, NUL-terminated)
bool ValidLevelName(const RwUInt8 *p, RwUInt32 len) {
    if (len == 0 || len > 48) {
        return false;
    }
    if (!((p[0] >= 'A' && p[0] <= 'Z') ||
          (p[0] >= 'a' && p[0] <= 'z'))) {
        return false;
    }
    RwUInt32 i = 0;
    for (; i < len; ++i) {
        const RwUInt8 c = p[i];
        if (c == 0) {
            break;
        }
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    return i > 1 && i < len;
}

} // namespace

RwBool RwTxdDecodeLevelPack(const RwUInt8 *pData, RwUInt32 len,
                            RwTxdImage *pOut) {
    if (!pData || !pOut || len < 32) {
        return FALSE;
    }
    std::memset(pOut, 0, sizeof(*pOut));
    // Name record signature: 06 11 00 00 02 00 00 00 len(<=48) libid
    static const RwUInt8 kSig[8] = {0x06, 0x11, 0x00, 0x00,
                                    0x02, 0x00, 0x00, 0x00};
    static const RwUInt8 kLib[4] = {0xFF, 0xFF, 0x03, 0x18};
    static const RwUInt8 kDims[12] = {0x01, 0x00, 0x00, 0x00,
                                      0x10, 0x00, 0x00, 0x00,
                                      0xFF, 0xFF, 0x03, 0x18};
    // Layout per texture (platform-independent dictionary): its IMAGE
    // records come first (one per mip level, largest first), then the
    // TEXTURE record carrying the name. So a name owns the first image
    // after the previous texture's name record. (Taking the image *after*
    // the name gave every texture the next texture's pixels.)
    RwUInt32 prevNameEnd = 0;
    for (RwUInt32 o = 0; o + 16 < len; ++o) {
        if (std::memcmp(pData + o, kSig, 8) != 0) {
            continue;
        }
        RwUInt32 nlen = 0;
        std::memcpy(&nlen, pData + o + 8, 4);
        if (nlen == 0 || nlen > 48 || o + 16 + nlen > len) {
            continue;
        }
        if (std::memcmp(pData + o + 12, kLib, 4) != 0) {
            continue;
        }
        const RwUInt8 *np = pData + o + 16;
        if (!ValidLevelName(np, nlen)) {
            continue;
        }
        // Dimensions record: first valid dims signature between the previous
        // name record and this one (= the largest mip level).
        const RwUInt32 base = prevNameEnd;
        prevNameEnd = o + 16 + nlen;
        for (RwUInt32 q = base; q + 28 < len && q < o; ++q) {
            if (std::memcmp(pData + q, kDims, 12) != 0) {
                continue;
            }
            RwUInt32 w = 0, h = 0, dep = 0, pi = 0;
            std::memcpy(&w, pData + q + 12, 4);
            std::memcpy(&h, pData + q + 16, 4);
            std::memcpy(&dep, pData + q + 20, 4);
            std::memcpy(&pi, pData + q + 24, 4);
            if (!IsPow2Size(w) || !IsPow2Size(h) || (dep != 32 && dep != 8)) {
                continue;
            }
            const RwUInt32 bpp = dep / 8;
            if (pi < w * bpp || pi > w * bpp + 64) {
                continue;
            }
            const RwUInt32 px = q + 28;
            // depth 8 (e.g. BEAVER01 "soldier"): indices, then a 256-entry
            // RGBA palette.
            const RwUInt32 palBytes = (dep == 8) ? 256u * 4u : 0u;
            if (px + pi * h + palBytes > len) {
                continue;
            }
            if (pOut->count >= rwtxdMAX_TEXTURES) {
                return (pOut->count > 0) ? TRUE : FALSE;
            }
            RwTxdTexture *t = &pOut->textures[pOut->count];
            RwUInt32 ni = 0;
            for (; ni < nlen && np[ni] != 0 &&
                   ni + 1 < (RwUInt32)rwtxdMAX_NAME;
                 ++ni) {
                t->name[ni] = static_cast<RwChar>(np[ni]);
            }
            t->name[ni] = 0;
            t->width = w;
            t->height = h;
            t->depth = dep;
            t->pitch = pi;
            t->pPixels = pData + px;
            t->pixelBytes = pi * h;
            t->pPalette = (dep == 8) ? pData + px + pi * h : 0;
            ++pOut->count;
            break; // first valid dims for this name
        }
    }
    pOut->textureCount = (RwUInt16)pOut->count;
    return (pOut->count > 0) ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// Embedded UI texture pack (d3d8-IMAGE nested; end of level chunk0)
// ---------------------------------------------------------------------------
namespace {

bool GuiIsVer(const RwUInt8 *p) {
    return p[0] == 0xFF && p[1] == 0xFF && p[2] == 0x03 && p[3] == 0x18;
}
RwUInt32 GuiU32(const RwUInt8 *p) {
    RwUInt32 v = 0;
    std::memcpy(&v, p, 4);
    return v;
}
bool GuiIsNamePrefix(const RwChar *name) {
    return std::strncmp(name, "GUI_", 4) == 0 ||
           std::strncmp(name, "compass_", 8) == 0 ||
           std::strncmp(name, "fishbasic", 9) == 0;
}

} // namespace

RwBool RwTxdDecodeGuiPack(const RwUInt8 *pData, RwUInt32 len,
                          RwTxdImage *pOut) {
    if (!pData || !pOut || len < 64) {
        return FALSE;
    }
    std::memset(pOut, 0, sizeof(*pOut));
    for (RwUInt32 o = 0; o + 24 < len; ++o) {
        // [id=6][len][ver][id=1][len=4][ver] filter
        if (GuiU32(pData + o) != 6 || !GuiIsVer(pData + o + 8)) {
            continue;
        }
        const RwUInt32 slen = GuiU32(pData + o + 4);
        if (slen == 0 || slen > 4096 || o + 12 + slen > len) {
            continue;
        }
        if (GuiU32(pData + o + 12) != 1 || GuiU32(pData + o + 16) != 4 ||
            !GuiIsVer(pData + o + 20)) {
            continue;
        }
        RwUInt32 p = o + 12;
        const RwUInt32 contentEnd = o + 12 + slen;
        p += 16; // id=1 filter chunk (12 + 4)
        // [id=2][nlen][ver] name
        if (p + 12 > len || GuiU32(pData + p) != 2) {
            continue;
        }
        const RwUInt32 nlen = GuiU32(pData + p + 4);
        if (nlen == 0 || nlen > (RwUInt32)rwtxdMAX_NAME - 1 ||
            !GuiIsVer(pData + p + 8)) {
            continue;
        }
        p += 12;
        RwChar name[rwtxdMAX_NAME];
        std::memcpy(name, pData + p, nlen);
        name[nlen] = '\0';
        bool nameOk = (nlen >= 4);
        for (RwUInt32 i = 0; i < nlen && nameOk; ++i) {
            if (name[i] != 0 && (name[i] < 32 || name[i] > 126)) {
                nameOk = false;
            }
        }
        if (!nameOk || !GuiIsNamePrefix(name)) {
            continue;
        }
        p += nlen;
        // [id=2][mlen][ver] mask
        if (p + 12 > len || GuiU32(pData + p) != 2 ||
            !GuiIsVer(pData + p + 8)) {
            continue;
        }
        const RwUInt32 mlen = GuiU32(pData + p + 4);
        if (p + 12 + mlen > len) {
            continue;
        }
        p += 12 + mlen;
        // [id=3][0][ver] terminator
        if (p + 12 > len || GuiU32(pData + p) != 3 ||
            GuiU32(pData + p + 4) != 0 || !GuiIsVer(pData + p + 8)) {
            continue;
        }
        p = contentEnd; // align to the struct tail
        // optional [u32 flag] (right before the first raster)
        if (p + 36 <= len && GuiU32(pData + p) != 0x18 &&
            GuiU32(pData + p + 4) == 0x18 && GuiIsVer(pData + p + 12)) {
            p += 4;
        }
        // raster chain: the first valid mip is the main texture
        if (p + 40 > len || GuiU32(pData + p) != 0x18 ||
            !GuiIsVer(pData + p + 8)) {
            continue;
        }
        if (GuiU32(pData + p + 12) != 1 || GuiU32(pData + p + 16) != 0x10 ||
            !GuiIsVer(pData + p + 20)) {
            continue;
        }
        const RwUInt32 w = GuiU32(pData + p + 24);
        const RwUInt32 h = GuiU32(pData + p + 28);
        const RwUInt32 depth = GuiU32(pData + p + 32);
        const RwUInt32 pitch = GuiU32(pData + p + 36);
        if (w == 0 || h == 0 || w > 512 || h > 512 || depth != 32 ||
            pitch < w * 4 || pitch > w * 4 + 64 ||
            p + 40 + pitch * h > len) {
            continue;
        }
        if (pOut->count >= rwtxdMAX_TEXTURES) {
            break;
        }
        RwTxdTexture *t = &pOut->textures[pOut->count];
        std::memcpy(t->name, name, nlen + 1);
        t->maskName[0] = '\0';
        t->filterFlags = 0;
        t->width = w;
        t->height = h;
        t->depth = depth;
        t->pitch = pitch;
        t->pPixels = pData + p + 40;
        t->pixelBytes = pitch * h;
        ++pOut->count;
    }
    pOut->textureCount = (RwUInt16)pOut->count;
    return (pOut->count > 0) ? TRUE : FALSE;
}
