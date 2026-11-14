#ifndef SHOWTEXT_H
#define SHOWTEXT_H

// ============================================================================
// CannonCruise - Show Text Box Entity (ShowText.h)
// Original path: D:\Projects\CannonCruisePC\code\ShowText.h
// Reverse-engineered via Ghidra and CC.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>
#include <string>

/**
 * @class CShowText
 * @brief On-screen narrative subtitles / prompt text box entity.
 *
 * Displays timed textual dialogue, tutorial messages, or objective hints on screen.
 */
class CShowText : public RWS::CEventHandler
{
public:
    /**
     * @brief Constructs a new CShowText entity with default display durations.
     * @see FUN_004b38d0
     */
    CShowText();

    /**
     * @brief Destructor.
     */
    virtual ~CShowText();

    /**
     * @brief Handles incoming event messages (e.g. MSG_SHOWTEXT_RENDER).
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Displays a string message on screen for a specified duration.
     * @param pText Null-terminated string message.
     * @param duration Active display time in seconds (default 3.0s).
     * @see FUN_004b4270
     */
    void Show(const char* pText, RwReal duration = 3.0f);

    /**
     * @brief Immediately hides the text box.
     */
    void Hide();

    /**
     * @brief Renders the subtitle backdrop box and font characters.
     * @see FUN_004b4dcf
     */
    void Render();

    /**
     * @brief Sets the FontSystem font name used for rendering (Phase 2).
     *
     * Default "Arial"; if the font cannot be found (no fonts/<name>.met + texture)
     * Render falls back to the embedded 5x7 bitmap font.
     * @param pFontName Name to pass to FontSystemGet (NULL → default).
     */
    void SetFontName(const char* pFontName);

private:
    std::string m_Text;      ///< Active message text string
    RwReal      m_fDuration; ///< Total display visibility duration in seconds
    RwReal      m_fTimer;    ///< Elapsed display time in seconds
    RwBool      m_bVisible;  ///< Whether the text box is currently visible
    RWS::CEventId m_RenderMsgId; ///< MSG_SHOWTEXT_RENDER event id (registered in ctor)
    char        m_FontName[32];  ///< FontSystem font name (Phase 2)
};

/**
 * @brief Draws text in screen coordinates with the embedded 5x7 bitmap font
 *        (menu labels, status line — without a CShowText box).
 * @param pText Null-terminated text (spaces/skipped).
 * @param x,y Top-left corner (pixels, screen space).
 * @param scale Pixel scale (2 or 3 recommended).
 * @param r,g,b,a Color.
 */
void ShowText_DrawBitmapText(const char *pText, RwReal x, RwReal y,
                             int scale, RwUInt8 r, RwUInt8 g, RwUInt8 b,
                             RwUInt8 a);

#endif // SHOWTEXT_H
