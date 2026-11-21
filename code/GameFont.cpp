// ============================================================================
// CannonCruise - Game font (GameFont.cpp) -- see GameFont.h
//
// lang_<lang>.fnt layout (texture path of FontSystem_LoadFromStream):
//   [12-byte chunk header]
//   u32 tag (type byte >= 1), u32 mode (0 = texture font), f32 lineExtent,
//   f32 sizeScale, f32 spacing, u32 flags, i32 extMin, i32 glyphCount,
//   i32 extCount, u16 extTable[extCount], u16 ascii[128],
//   glyphCount x { f32 u0, v0, u1, v1, f32 aspect, u8 texIndex },
//   u32 texCount, texCount x char[32] texture name ("brady48.png")
// ============================================================================

#include "GameFont.h"

#include "LocalizationManager.h"
#include "RenderPlatform.h"

#include <rw.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Glyph {
    RwReal u0, v0, u1, v1, aspect;
};

RwBool s_tried = FALSE;
RwBool s_ready = FALSE;
RwUInt16 s_ascii[128];
RwInt32 s_extMin = 0;
std::vector<RwUInt16> s_ext;
std::vector<Glyph> s_glyphs;
rw::Texture *s_pTexture = nullptr;

template <typename T> RwBool Read(const std::vector<RwUInt8> &d, size_t &o, T *pOut) {
    if (o + sizeof(T) > d.size()) return FALSE;
    std::memcpy(pOut, &d[o], sizeof(T));
    o += sizeof(T);
    return TRUE;
}

const Glyph *FindGlyph(unsigned int code) {
    RwUInt16 idx = 0xFFFF;
    if (code < 128) {
        idx = s_ascii[code];
        if (idx == 0xFFFF && code >= 'a' && code <= 'z') {
            idx = s_ascii[code - 'a' + 'A']; // the font has few lowercase letters
        }
    } else if ((RwInt32)code >= s_extMin && (RwInt32)code - s_extMin < (RwInt32)s_ext.size()) {
        idx = s_ext[(size_t)((RwInt32)code - s_extMin)];
    }
    if (idx == 0xFFFF || idx >= s_glyphs.size()) return nullptr;
    return &s_glyphs[idx];
}

// Gap between glyphs as a fraction of the cell height.
const RwReal kTracking = 0.02f;

} // namespace

RwBool GameFont_Load(void) {
    if (s_tried == TRUE) {
        return s_ready;
    }
    CLocalizationManager *pLoc = CLocalizationManager::GetSingletonPtr();
    if (pLoc == nullptr || pLoc->GetFontTexDict() == nullptr) {
        return FALSE; // localization not up yet: try again on the next call
    }
    s_tried = TRUE;
    std::string lang = pLoc != nullptr ? pLoc->GetCurrentLanguage() : "";
    if (lang.empty()) lang = "eng";
    for (size_t i = 0; i < lang.size(); ++i) lang[i] = (char)std::tolower((unsigned char)lang[i]);
    char path[128];
    std::snprintf(path, sizeof(path), "Localiza/lang_%s.fnt", lang.c_str());
    FILE *f = std::fopen(path, "rb");
    if (f == nullptr) {
        std::printf("GameFont: %s not found\n", path);
        return FALSE;
    }
    std::vector<RwUInt8> d;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size > 0) {
        d.resize((size_t)size);
        if (std::fread(&d[0], 1, d.size(), f) != d.size()) d.clear();
    }
    std::fclose(f);
    size_t o = 12;
    RwUInt32 tag = 0, mode = 0, flags = 0;
    RwReal lineExtent = 0, sizeScale = 0, spacing = 0;
    RwInt32 glyphCount = 0, extCount = 0;
    if (Read(d, o, &tag) != TRUE || (tag >> 24) < 1 || Read(d, o, &mode) != TRUE || mode != 0 ||
        Read(d, o, &lineExtent) != TRUE || Read(d, o, &sizeScale) != TRUE ||
        Read(d, o, &spacing) != TRUE || Read(d, o, &flags) != TRUE ||
        Read(d, o, &s_extMin) != TRUE || Read(d, o, &glyphCount) != TRUE ||
        Read(d, o, &extCount) != TRUE || glyphCount <= 0 || glyphCount > 4096 ||
        extCount < 0 || extCount > 65536) {
        std::printf("GameFont: %s: unsupported header\n", path);
        return FALSE;
    }
    s_ext.resize((size_t)extCount);
    for (RwInt32 i = 0; i < extCount; ++i) {
        if (Read(d, o, &s_ext[(size_t)i]) != TRUE) return FALSE;
    }
    for (int i = 0; i < 128; ++i) {
        if (Read(d, o, &s_ascii[i]) != TRUE) return FALSE;
    }
    s_glyphs.resize((size_t)glyphCount);
    for (RwInt32 i = 0; i < glyphCount; ++i) {
        Glyph &g = s_glyphs[(size_t)i];
        RwUInt8 texIndex = 0;
        if (Read(d, o, &g.u0) != TRUE || Read(d, o, &g.v0) != TRUE || Read(d, o, &g.u1) != TRUE ||
            Read(d, o, &g.v1) != TRUE || Read(d, o, &g.aspect) != TRUE ||
            Read(d, o, &texIndex) != TRUE) {
            return FALSE;
        }
    }
    RwUInt32 texCount = 0;
    char texName[33] = {0};
    if (Read(d, o, &texCount) != TRUE || texCount < 1 || o + 32 > d.size()) {
        return FALSE;
    }
    std::memcpy(texName, &d[o], 32);
    char *pDot = std::strrchr(texName, '.');
    if (pDot) *pDot = '\0';
    rw::TexDictionary *pDict =
        pLoc != nullptr ? reinterpret_cast<rw::TexDictionary *>(pLoc->GetFontTexDict()) : nullptr;
    s_pTexture = pDict != nullptr ? pDict->find(texName) : nullptr;
    if (s_pTexture == nullptr || s_pTexture->raster == nullptr) {
        std::printf("GameFont: atlas %s not loaded\n", texName);
        s_pTexture = nullptr;
        return FALSE;
    }
    s_pTexture->setFilter(rw::Texture::LINEAR);
    s_ready = TRUE;
    std::printf("GameFont: %s (%d glyphs, atlas %s)\n", path, (int)glyphCount, texName);
    return TRUE;
}

RwBool GameFont_IsReady(void) { return s_ready; }

RwReal GameFont_Measure(const char *pText, RwReal height) {
    if (s_ready != TRUE || pText == nullptr) return 0.0f;
    RwReal w = 0.0f, best = 0.0f;
    for (const unsigned char *p = (const unsigned char *)pText; *p; ++p) {
        if (*p == '\n') {
            if (w > best) best = w;
            w = 0.0f;
            continue;
        }
        const Glyph *g = FindGlyph(*p);
        w += (g != nullptr ? g->aspect : 0.3f) * height + kTracking * height;
    }
    return w > best ? w : best;
}

void GameFont_Draw(const char *pText, RwReal x, RwReal y, RwReal height, RwUInt32 argb) {
    if (s_ready != TRUE || pText == nullptr) return;
    RwReal cx = x;
    for (const unsigned char *p = (const unsigned char *)pText; *p; ++p) {
        if (*p == '\n') {
            cx = x;
            y += height;
            continue;
        }
        const Glyph *g = FindGlyph(*p);
        if (g == nullptr) {
            cx += 0.3f * height;
            continue;
        }
        const RwReal w = g->aspect * height;
        if (*p != ' ') {
            RenderPlatform_DrawTextureRegion(s_pTexture, cx, y, w, height, g->u0, g->v0, g->u1,
                                             g->v1, argb);
        }
        cx += w + kTracking * height;
    }
}
