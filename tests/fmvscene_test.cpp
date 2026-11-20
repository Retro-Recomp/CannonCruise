// ============================================================================
// CannonCruise - FMV Scene Descriptor Test (fmvscene_test.cpp)
//
// Verified behavior (code/FmvScene.cpp):
//   - FMV descriptor (CFMVBehavior) parsing: GUID + FMV name + TARGET
//     SCENE — the full 8-file transition graph is extracted from file data:
//       JINGLE  → Jingle  → MainMenu   (binary sub_4A2010 STARTUP)
//       INTRO1  → Intro   → Nordic01   (intro → nordic LINK)
//       INTRO2  → Intro2  → Nordic02
//       INTRO3  → Intro3  → MAPMENU
//       CREDITS → Credits → MainMenu
//       OUTRO1  → Outro   → Iduna02
//       OUTRO2  → Outro2  → Iduna03
//       OUTRO3  → Outro3  → Credits
//   - level/world files (>64 KB) do NOT count as descriptors → 0 (end of chain)
//   - video path is binary-exact: ".\Video\Win32\<name>.bik" (sub_45F950 case0)
//   - .bik files EXIST in the repo (Video/Win32/*.bik, Phase 4.6y) →
//     FmvScene_VideoExists = 1; a missing name → 0
//   - target → sequence file; for a target case DIFFERENCE an UPPERCASE
//     retry ("Nordic01" → Streams/NORDIC01.RWS)
//
// Priority: the previous code only printed the target and moved on — the nordic
// link was missing; also the intro INTRO1→2→3 assumption was wrong.
// ============================================================================

#include "FmvScene.h"
#include "RwsAssets.h"

#include <cstdio>
#include <cstring>

static int g_checks = 0;
static int g_failures = 0;

static void Check(bool cond, const char *pExpr, int line) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("FAIL line %d: %s\n", line, pExpr);
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

// Case-insensitive string comparison — Windows file system
// is case-insensitive so FmvScene_NextScenePath tries verbatim first and
// returns a mixed-case path like "Streams/Nordic01.RWS"; on a case-insensitive FS
// UPPERCASE retry yields "Streams/NORDIC01.RWS". Both are correct behavior →
// the test verifies not which form is returned, but that it resolves to the SAME file.
static bool EqualsNoCase(const char *pA, const char *pB) {
    if (pA == nullptr || pB == nullptr) {
        return false;
    }
    for (; *pA != 0 && *pB != 0; ++pA, ++pB) {
        char ca = *pA;
        char cb = *pB;
        if (ca >= 'a' && ca <= 'z') {
            ca = (char)(ca - 'a' + 'A');
        }
        if (cb >= 'a' && cb <= 'z') {
            cb = (char)(cb - 'a' + 'A');
        }
        if (ca != cb) {
            return false;
        }
    }
    return *pA == 0 && *pB == 0;
}

// ---------------------------------------------------------------------------
// 1) Transition graph — all 8 FMV descriptors (FMV name + target)
// ---------------------------------------------------------------------------
static void TestTransitionGraph() {
    struct Case {
        const char *pFile;
        const char *pFmv;
        const char *pTarget;
    };
    static const Case kCases[] = {
        {"Streams/JINGLE.RWS", "Jingle", "MainMenu"},
        {"Streams/INTRO1.RWS", "Intro", "Nordic01"},
        {"Streams/INTRO2.RWS", "Intro2", "Nordic02"},
        {"Streams/INTRO3.RWS", "Intro3", "MAPMENU"},
        {"Streams/CREDITS.RWS", "Credits", "MainMenu"},
        {"Streams/OUTRO1.RWS", "Outro", "Iduna02"},
        {"Streams/OUTRO2.RWS", "Outro2", "Iduna03"},
        {"Streams/OUTRO3.RWS", "Outro3", "Credits"},
    };
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i) {
        FmvSceneDescriptor d;
        std::memset(&d, 0xAA, sizeof(d));
        const int ok = FmvScene_Load(kCases[i].pFile, &d);
        if (ok != 1) {
            std::printf("FAIL: %s not a descriptor\n", kCases[i].pFile);
        }
        CHECK(ok == 1);
        CHECK(std::strcmp(d.behaviour, kCases[i].pFmv) == 0);
        CHECK(std::strcmp(d.target, kCases[i].pTarget) == 0);
        // GUID: external {GUID} id present (contains '{' and '}')
        CHECK(d.guid[0] == '{');
        CHECK(std::strchr(d.guid, '}') != nullptr);
        if (ok == 1) {
            std::printf("  %s: fmv='%s' target='%s' guid=%s\n",
                        kCases[i].pFile, d.behaviour, d.target, d.guid);
        }
    }
}

// ---------------------------------------------------------------------------
// 2) Level/world files are NOT descriptors → 0 (end-of-chain guarantee)
//    missing file → 0
// ---------------------------------------------------------------------------
static void TestNonDescriptorAndMissing() {
    FmvSceneDescriptor d;
    // 15.9 MB level — far above the 64 KB limit
    CHECK(FmvScene_Load("Streams/VIKING01.RWS", &d) == 0);
    // 1.8 MB menu — counts as level/world (the JINGLE target ends here)
    CHECK(FmvScene_Load("Streams/MAINMENU.RWS", &d) == 0);
    // nonexistent file
    CHECK(FmvScene_Load("Streams/NO_SUCH_SCENE.RWS", &d) == 0);
    // empty/invalid input
    CHECK(FmvScene_Load(nullptr, &d) == 0);
    CHECK(FmvScene_Load("Streams/JINGLE.RWS", nullptr) == 0);
}

// ---------------------------------------------------------------------------
// 3) Video path — sub_45F950 case0: <base>"Video\Win32\"<name>".bik", base ".\"
// ---------------------------------------------------------------------------
static void TestVideoPath() {
    char buf[512];
    CHECK(FmvScene_VideoPath("Jingle", buf, sizeof(buf)) == 1);
    CHECK(std::strcmp(buf, ".\\Video\\Win32\\Jingle.bik") == 0);
    CHECK(FmvScene_VideoPath("Intro2", buf, sizeof(buf)) == 1);
    CHECK(std::strcmp(buf, ".\\Video\\Win32\\Intro2.bik") == 0);
    // invalid input
    CHECK(FmvScene_VideoPath(nullptr, buf, sizeof(buf)) == 0);
    CHECK(FmvScene_VideoPath("", buf, sizeof(buf)) == 0);
    CHECK(FmvScene_VideoPath("Jingle", nullptr, 0) == 0);

    // Phase 4.6y: .bik files exist in the repo (Video/Win32/*.bik) → 1.
    // Playback was linked with BinkPlayer in Phase 4.6y; a name not in the repo → 0.
    CHECK(FmvScene_VideoExists("Jingle") == 1);
    CHECK(FmvScene_VideoExists("Intro") == 1);
    CHECK(FmvScene_VideoExists("NoSuchFmvXYZ") == 0);
    CHECK(FmvScene_VideoExists(nullptr) == 0);
}

// ---------------------------------------------------------------------------
// 4) Target → next descriptor path. Critical: the files are UPPERCASE
//    (NORDIC01.RWS, MAINMENU.RWS) but the target is "Nordic01"/"MainMenu" →
//    an UPPERCASE retry is required (otherwise the nordic link is LOST).
// ---------------------------------------------------------------------------
static void TestNextScenePath() {
    char rel[512];

    CHECK(FmvScene_NextScenePath("Nordic01", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/NORDIC01.RWS"));

    CHECK(FmvScene_NextScenePath("Nordic02", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/NORDIC02.RWS"));

    CHECK(FmvScene_NextScenePath("MainMenu", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/MAINMENU.RWS"));

    CHECK(FmvScene_NextScenePath("MAPMENU", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/MAPMENU.RWS"));

    CHECK(FmvScene_NextScenePath("Iduna02", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/IDUNA02.RWS"));

    // verbatim match uppercase target (CREDITS)
    CHECK(FmvScene_NextScenePath("CREDITS", rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/CREDITS.RWS"));

    // nonexistent target → 0
    CHECK(FmvScene_NextScenePath("NoSuchLevel", rel, sizeof(rel)) == 0);
    // invalid input
    CHECK(FmvScene_NextScenePath(nullptr, rel, sizeof(rel)) == 0);
    CHECK(FmvScene_NextScenePath("", rel, sizeof(rel)) == 0);
}

// ---------------------------------------------------------------------------
// 5) Chain setup: JINGLE → (MAINMENU) → not a descriptor → END.
//    INTRO1 → (NORDIC01) → not a descriptor → END + target registered.
// ---------------------------------------------------------------------------
static void TestChainTermination() {
    char rel[512] = {0};
    char terminal[64] = {0};

    // a) binary startup: JINGLE
    FmvSceneDescriptor d;
    CHECK(FmvScene_Load("Streams/JINGLE.RWS", &d) == 1);
    std::snprintf(terminal, sizeof(terminal), "%s", d.target);
    CHECK(FmvScene_NextScenePath(d.target, rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/MAINMENU.RWS"));
    // the chain is not a descriptor at the next step (1.8 MB) → END
    CHECK(FmvScene_Load(rel, &d) == 0);
    CHECK(std::strcmp(terminal, "MainMenu") == 0);

    // b) INTRO1 → nordic link
    CHECK(FmvScene_Load("Streams/INTRO1.RWS", &d) == 1);
    std::snprintf(terminal, sizeof(terminal), "%s", d.target);
    CHECK(FmvScene_NextScenePath(d.target, rel, sizeof(rel)) == 1);
    CHECK(EqualsNoCase(rel, "Streams/NORDIC01.RWS"));
    // NORDIC01 not a descriptor (14 MB level) → end of chain, target preserved
    CHECK(FmvScene_Load(rel, &d) == 0);
    CHECK(std::strcmp(terminal, "Nordic01") == 0);

    // c) OUTRO chain: OUTRO3 → Credits (DESCRIPTOR → chain continues)
    CHECK(FmvScene_Load("Streams/OUTRO3.RWS", &d) == 1);
    CHECK(FmvScene_NextScenePath(d.target, rel, sizeof(rel)) == 1);
    CHECK(FmvScene_Load(rel, &d) == 1);
    CHECK(std::strcmp(d.behaviour, "Credits") == 0);
    // Credits → MainMenu → not a descriptor → END
    CHECK(FmvScene_NextScenePath(d.target, rel, sizeof(rel)) == 1);
    CHECK(FmvScene_Load(rel, &d) == 0);
}

int main() {
    // Sentinel scan finds the root from the test CWD (same as rwsassets_test).
    RwsAssets_Reset();

    TestTransitionGraph();
    TestNonDescriptorAndMissing();
    TestVideoPath();
    TestNextScenePath();
    TestChainTermination();

    std::printf("fmvscene_test: %d checks, %d failures\n", g_checks,
                g_failures);
    if (g_failures != 0) {
        return 1;
    }
    std::printf("fmvscene_test: OK\n");
    return 0;
}
