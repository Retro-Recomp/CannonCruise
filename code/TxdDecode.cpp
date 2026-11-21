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


// Level texture pack: a platform-independent texture dictionary
// (rwID_PITEXDICTIONARY, 0x23) inside the level's first asset record:
//   u16 textureCount, u16 deviceId, then per texture
//   u32 mipCount, mipCount x rwID_IMAGE { STRUCT {w, h, depth, stride},
//   pixels (stride*h), palette (depth 8: 256 RGBA) }, then rwID_TEXTURE
//   { STRUCT filter, STRING name, STRING mask, EXTENSION }.
// The images come before the texture's name; the base level is decoded.
namespace {

RwUInt32 PackU32(const RwUInt8 *p) {
    RwUInt32 v = 0;
    std::memcpy(&v, p, 4);
    return v;
}

bool IsRwVersion(RwUInt32 v) { return (v & 0xFFFF0000u) == 0x18030000u || (v & 0xFFFF0000u) == 0x18020000u; }

// Parses one PI dictionary at `o`; appends to pOut. Returns false on a
// malformed dictionary (nothing appended past the failure point).
bool ParsePiDictionary(const RwUInt8 *pData, RwUInt32 len, RwUInt32 o, RwTxdImage *pOut) {
    const RwUInt32 dictSize = PackU32(pData + o + 4);
    const RwUInt32 dictEnd = o + 12 + dictSize;
    if (dictEnd > len || dictSize < 4) {
        return false;
    }
    RwUInt32 p = o + 12;
    RwUInt16 count = 0;
    std::memcpy(&count, pData + p, 2);
    p += 4;
    for (RwUInt32 t = 0; t < count; ++t) {
        if (p + 4 > dictEnd) return false;
        const RwUInt32 mips = PackU32(pData + p);
        p += 4;
        if (mips == 0 || mips > 16) return false;
        RwUInt32 w = 0, h = 0, dep = 0, pitch = 0, px = 0;
        for (RwUInt32 m = 0; m < mips; ++m) {
            if (p + 40 > dictEnd || PackU32(pData + p) != 0x18 ||
                PackU32(pData + p + 12) != 0x01 || PackU32(pData + p + 16) != 16) {
                return false;
            }
            const RwUInt32 imgSize = PackU32(pData + p + 4);
            if (m == 0) {
                w = PackU32(pData + p + 24);
                h = PackU32(pData + p + 28);
                dep = PackU32(pData + p + 32);
                pitch = PackU32(pData + p + 36);
                px = p + 40;
            }
            p += 12 + imgSize;
        }
        // rwID_TEXTURE: STRUCT (filter), STRING name.
        if (p + 12 > dictEnd || PackU32(pData + p) != 0x06) return false;
        const RwUInt32 texEnd = p + 12 + PackU32(pData + p + 4);
        if (texEnd > dictEnd) return false;
        RwUInt32 q = p + 12;
        q += 12 + PackU32(pData + q + 4); // filter struct
        char name[rwtxdMAX_NAME] = {0};
        if (q + 12 <= texEnd && PackU32(pData + q) == 0x02) {
            const RwUInt32 nlen = PackU32(pData + q + 4);
            for (RwUInt32 i = 0; i < nlen && i + 1 < sizeof(name) && q + 12 + i < texEnd; ++i) {
                const char c = (char)pData[q + 12 + i];
                if (c == 0) break;
                name[i] = c;
            }
        }
        p = texEnd;
        const RwUInt32 bpp = dep / 8;
        const bool supported = (dep == 32 || dep == 8) && w > 0 && h > 0 && w <= 2048 &&
                               h <= 2048 && pitch >= w * bpp &&
                               px + pitch * h + (dep == 8 ? 1024u : 0u) <= dictEnd;
        if (!supported || name[0] == 0) {
            continue;
        }
        if (pOut->count >= rwtxdMAX_TEXTURES) {
            return true;
        }
        RwTxdTexture *pTex = &pOut->textures[pOut->count++];
        std::memcpy(pTex->name, name, sizeof(pTex->name));
        pTex->width = w;
        pTex->height = h;
        pTex->depth = dep;
        pTex->pitch = pitch;
        pTex->pPixels = pData + px;
        pTex->pixelBytes = pitch * h;
        pTex->pPalette = (dep == 8) ? pData + px + pitch * h : 0;
    }
    return true;
}

} // namespace

RwBool RwTxdDecodeLevelPack(const RwUInt8 *pData, RwUInt32 len,
                            RwTxdImage *pOut) {
    if (!pData || !pOut || len < 32) {
        return FALSE;
    }
    std::memset(pOut, 0, sizeof(*pOut));
    for (RwUInt32 o = 0; o + 16 <= len; o += 4) {
        if (PackU32(pData + o) != 0x23 || !IsRwVersion(PackU32(pData + o + 8))) {
            continue;
        }
        if (ParsePiDictionary(pData, len, o, pOut)) {
            o += 12 + PackU32(pData + o + 4) - 4; // continue after this dictionary
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
