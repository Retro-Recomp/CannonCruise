#ifndef MENUBEHAVIOR_H
#define MENUBEHAVIOR_H

// ============================================================================
// CannonCruise - 2D Maestro Menu Behavior (MenuBehavior.h)
// Original path: D:\Projects\CannonCruisePC\code\MenuBehavior.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>
#include <rt2d.h>
#include <rt2danim.h>

/**
 * @class CMenuBehavior
 * @brief RenderWare 2D (Rt2dMaestro) UI menu controller.
 *
 * Hosts and drives interactive RenderWare 2D vector / bitmap animated menus,
 * buttons, and screens using the Rt2dMaestro engine.
 */
class CMenuBehavior : public RWS::CEventHandler
{
public:
    /**
     * @brief Constructs a new CMenuBehavior instance and claims an input focus ID.
     * @see FUN_00443050
     */
    CMenuBehavior();

    /**
     * @brief Destructor. Destroys Rt2dMaestro instance and frees input focus.
     */
    virtual ~CMenuBehavior();

    /**
     * @brief Handles incoming event messages (e.g. MSG_MENU_RENDER_FRONT_RT2D).
     * @param msg Event message container.
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Ticks 2D menu animation and input dispatch.
     * @param deltaTime Elapsed frame time in seconds.
     * @see FUN_00443800
     */
    void Update(RwReal deltaTime);

    /**
     * @brief Renders 2D menu layout using RenderWare 2D Maestro.
     * @see FUN_00443c20
     */
    void Render();

    /**
     * @brief Retrieves the active Rt2dMaestro object.
     * @return Pointer to Rt2dMaestro or NULL.
     */
    Rt2dMaestro* GetMaestro() const { return m_pMaestro; }

    /**
     * @brief Ownership: attaches the parsed maestro to the behavior
     *        (any previous one is destroyed). Video-scene: MAINMENU/
     *        OPTIONS/SOUND/PAUSE states share the same Menu.anm.
     */
    void AttachMaestro(Rt2dMaestro* pMaestro);

    /**
     * @brief Loads the embedded menu textures (RWS TexDictionary → GL).
     *        Menu_074 (640×480 background) + Menu_076 (logo) + bar/cursor.
     * @return TRUE (at least one texture loaded).
     */
    RwBool LoadTextures(const char* rwsPath);
    RwBool HasTextures() const;
    /// Video-scene menu screens (1:1 with the lang_eng.txt IDs).
    enum EMenuScreen {
        MENUSCREEN_MAIN,      ///< NEW GAME/LOAD GAME/OPTIONS/CREDITS/QUIT GAME
        MENUSCREEN_OPTIONS,   ///< SOUND/DIFFICULTY•x/SINGLE PLAYER
        MENUSCREEN_SOUND,     ///< VOICES/EFFECTS/MUSIC bars
        MENUSCREEN_DIFFICULTY,///< EASY/NORMAL/HARD
        MENUSCREEN_CONFIRMNEW ///< SAVE GAME? (NewSave/NewNoSave)
    };
    /// Terminal actions (the main.cpp loop exits).
    enum EMenuAction {
        MENUACTION_NONE, MENUACTION_NEWGAME, MENUACTION_LOADGAME,
        MENUACTION_QUITGAME, MENUACTION_CREDITS
    };

    /// Selection navigation (W/S/↑/↓) — on the active screen; the maestro also advances.
    RwBool MoveNext();
    RwBool MovePrev();
    /// Sound bar adjustment (A/D/←/→) — on the SOUND screen.
    RwBool AdjustLeft();
    RwBool AdjustRight();
    /// Confirm (Enter/Space): screen transition or terminal action. TRUE=exit the loop.
    RwBool Confirm();
    /// Back (Esc): to the parent screen; on MAIN TRUE=exit the loop (not counted as quit).
    RwBool Back();
    /// Name of the confirmed command (NULL if no selection).
    const char* GetSelectedCommand() const;
    EMenuAction TakePendingAction();
    EMenuScreen GetScreen() const { return m_screen; }

  private:
    Rt2dMaestro* m_pMaestro;      ///< RenderWare 2D Maestro animation scene manager
    RwInt32      m_iInputFocusID; ///< Input focus registration ID
    EMenuScreen  m_screen;        ///< Active video-scene screen
    RwInt32      m_selection;     ///< Selected row on the active screen
    RwInt32      m_volVoices;     ///< 0..10 (SpeakVolSet)
    RwInt32      m_volEffects;    ///< 0..10 (EffectVolSet)
    RwInt32      m_volMusic;      ///< 0..10 (MusicVolSet)
    RwInt32      m_difficulty;    ///< 0=EASY 1=NORMAL 2=HARD (SetDifficulty)
    RwInt32      m_multiplayer;   ///< 0=SINGLE 1=TWO (SetMultiPlayer)
    EMenuAction  m_pendingAction; ///< Terminal action (consumed by Take)
    RwInt32 ScreenItemCount() const;
};

/**
 * @brief Reads the RWS embedded TexDictionary directly (0xe8 fixed layout —
 *        MAINMENU/MAPMENU/GLOBAL/LANGSEL/levels) and loads the textures into GL.
 *        Does not go through the RwStudio parser (skips MAPMENU).
 * @return TRUE (at least one texture loaded).
 */
RwBool MenuTextures_LoadFromRws(const char* rwsPath);

/**
 * @brief Finds, decodes and loads into GL ALL embedded TexDictionary assets inside the RWS
 *        (embeddedType=0x23, libId=0x1803FFFF) using the RwStudio parser.
 *        The direct-0xe8 path is not valid for GLOBAL (the embedded asset in GLOBAL is
 *        at a different offset); for INFO/LOG the name of each texture is printed.
 * @return Number of textures loaded (0 = no embedded txd / decode error).
 */
RwUInt32 MenuTextures_UploadEmbeddedFromRws(const char* rwsPath);

#endif // MENUBEHAVIOR_H
