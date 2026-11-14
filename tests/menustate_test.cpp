// ============================================================================
// CannonCruise - Menu/map state machine unit test (menustate_test.cpp)
//
// CMenuBehavior (Menu.anm hierarchy) + CAfterGameMenuBehavior (CAG level
// parse/selection) — no Render (headless), no engine (pure logic).
// Usage: menustate_test <MAINMENU.RWS> <MAPMENU.RWS>
// ============================================================================

#include "MenuBehavior.h"
#include "AfterGameMenuBehavior.h"
#include "Rt2dMaestro.h"
#include "Rt2dMaestroStream.h"

#include <cstdio>
#include <cstring>

static int s_checks = 0;
static int s_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++s_checks;                                                            \
        if (!(cond)) {                                                         \
            ++s_failures;                                                      \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                \
        }                                                                      \
    } while (0)

int main(int argc, char **argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: menustate_test MAINMENU.RWS MAPMENU.RWS\n");
        return 2;
    }

    // ---- MAINMENU: load + screen hierarchy ----
    {
        Rt2dMaestro *m = Rt2dMaestro_LoadFromRwsFile(argv[1]);
        CHECK(m != 0);
        if (m == 0) {
            return 1;
        }
        CHECK(m->info.nodeCount == 32);
        CMenuBehavior menu;
        menu.AttachMaestro(m);
        CHECK(menu.GetMaestro() != 0);
        CHECK(menu.GetScreen() == CMenuBehavior::MENUSCREEN_MAIN);
        CHECK(menu.TakePendingAction() ==
              CMenuBehavior::MENUACTION_NONE);

        // MAIN[0]=NEW GAME → CONFIRMNEW (not terminal)
        CHECK(menu.Confirm() == FALSE);
        CHECK(menu.GetScreen() == CMenuBehavior::MENUSCREEN_CONFIRMNEW);
        // SAVE GAME → NEWGAME terminal
        CHECK(menu.Confirm() == TRUE);
        CHECK(menu.TakePendingAction() ==
              CMenuBehavior::MENUACTION_NEWGAME);
        CHECK(menu.TakePendingAction() ==
              CMenuBehavior::MENUACTION_NONE); // consumed

        // MAIN[2]=OPTIONS → OPTIONS screen
        CMenuBehavior menu2;
        menu2.AttachMaestro(Rt2dMaestro_LoadFromRwsFile(argv[1]));
        CHECK(menu2.GetMaestro() != 0);
        menu2.MoveNext();
        menu2.MoveNext();
        CHECK(menu2.Confirm() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_OPTIONS);
        // OPTIONS[0]=SOUND → SOUND screen
        CHECK(menu2.Confirm() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_SOUND);
        // bar adjustment (VOICES 7 → 8 → 7)
        CHECK(menu2.AdjustRight() == TRUE);
        CHECK(menu2.AdjustLeft() == TRUE);
        CHECK(menu2.AdjustLeft() == TRUE);
        // SOUND confirm → back to OPTIONS
        CHECK(menu2.Confirm() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_OPTIONS);
        // OPTIONS[1]=DIFFICULTY screen → select HARD → back
        menu2.MoveNext();
        CHECK(menu2.Confirm() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_DIFFICULTY);
        menu2.MoveNext();
        menu2.MoveNext(); // HARD
        CHECK(menu2.Confirm() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_OPTIONS);
        // Esc chain: OPTIONS→MAIN→exit
        CHECK(menu2.Back() == FALSE);
        CHECK(menu2.GetScreen() == CMenuBehavior::MENUSCREEN_MAIN);
        CHECK(menu2.Back() == TRUE);
        // QUIT + CREDITS + LOAD terminals (direct selection)
        CMenuBehavior menu3;
        menu3.AttachMaestro(Rt2dMaestro_LoadFromRwsFile(argv[1]));
        for (int i = 0; i < 4; ++i) {
            menu3.MoveNext();
        }
        CHECK(menu3.Confirm() == TRUE); // QUIT GAME
        CHECK(menu3.TakePendingAction() ==
              CMenuBehavior::MENUACTION_QUITGAME);
    }

    // ---- MAPMENU: 12 levels parse + selection ----
    {
        Rt2dMaestro *m = Rt2dMaestro_LoadFromRwsFile(argv[2]);
        CHECK(m != 0);
        if (m == 0) {
            return 1;
        }
        CHECK(m->info.nodeCount == 44);
        CAfterGameMenuBehavior map;
        map.AttachMaestro(m);
        CHECK(map.GetLevelCount() == 12);
        CHECK(std::strcmp(map.GetLevelDisplay(0), "VIKING 1") == 0);
        CHECK(std::strcmp(map.GetLevelFile(0), "VIKING01") == 0);
        CHECK(std::strcmp(map.GetLevelFile(1), "NORDIC01") == 0);
        CHECK(std::strcmp(map.GetLevelFile(11), "IDUNA01") == 0);
        CHECK(map.GetLevelDisplay(-1) == 0);
        CHECK(map.GetLevelFile(12) == 0);
        CHECK(map.TakeSelectedLevel() == -1); // no confirmation
        map.MoveNext();
        CHECK(map.Confirm() == TRUE);
        CHECK(map.TakeSelectedLevel() == 1); // NORDIC 1
        CHECK(map.TakeSelectedLevel() == -1);
        CHECK(map.Back() == TRUE); // exit from map
        // summary mode: toggle
        map.ShowSummary(1234, 56, 2);
        CHECK(map.Confirm() == FALSE); // from summary back to map
        map.ShowSummary(10, 20, 1);
        CHECK(map.Back() == FALSE);
    }

    // ---- invalid inputs ----
    CHECK(Rt2dMaestro_LoadFromRwsFile(0) == 0);
    CHECK(Rt2dMaestro_LoadFromRwsFile("/yok/boyle/bir/dosya.rws") == 0);

    std::printf("menustate_test: %d checks, %d failures\n", s_checks,
                s_failures);
    return (s_failures == 0) ? 0 : 1;
}
