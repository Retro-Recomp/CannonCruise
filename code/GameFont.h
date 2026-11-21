#ifndef CANNONCRUISE_GAMEFONT_H
#define CANNONCRUISE_GAMEFONT_H

// ============================================================================
// CannonCruise - Game font (GameFont.h)
//
// The game's own text font: Localiza/lang_<lang>.fnt (glyph UVs, code map)
// with its atlas "brady48" from Localiza/Win/win_<lang>.txd, which
// CLocalizationManager loads. Glyphs are drawn as textured Im2D quads.
// ============================================================================

#include "rwcore.h"

/// Loads the font for the current language (idempotent). FALSE if missing.
RwBool GameFont_Load(void);

/// TRUE once the font and its atlas texture are available.
RwBool GameFont_IsReady(void);

/// Width in pixels of one line drawn with a glyph cell of `height` pixels.
RwReal GameFont_Measure(const char *pText, RwReal height);

/// Draws one line ('\n' starts a new line) with its top-left at (x, y).
/// Call between RenderPlatform_BeginOverlay2D/EndOverlay2D.
void GameFont_Draw(const char *pText, RwReal x, RwReal y, RwReal height,
                   RwUInt32 argb);

#endif // CANNONCRUISE_GAMEFONT_H
