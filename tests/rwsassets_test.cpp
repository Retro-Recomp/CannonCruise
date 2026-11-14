// ============================================================================
// CannonCruise - .RWS Asset Path Resolver Test (rwsassets_test.cpp)
//
// Verified behavior (code/RwsAssets.cpp):
//   - the CANNONCRUISE_STREAMS environment variable takes priority and is normalized
//     (a trailing / or \ is appended/preserved)
//   - OTHERWISE a candidate prefix scan with the sentinel (Streams/MAINMENU.RWS) —
//     deep VS build layouts (code/build_<x>/tests → root 3 levels
//     up) are included
//   - RwsAssets_Resolve returns a path only if the file EXISTS
//   - file not found → 0 and an empty string
//
// Previous bug: a hand-written 3-prefix list ("" / "../" / "../../") existed in
// 5 separate places in main.cpp/SceneManager.cpp; since the build output is deeper,
// assets could never be found (clump=0, assets=0).
// ============================================================================

#include "RwsAssets.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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

// platform-dependent environment variable setter (Windows/POSIX)
static void SetEnv(const char *pName, const char *pValue) {
#if defined(_WIN32)
    _putenv_s(pName, pValue != nullptr ? pValue : "");
#else
    if (pValue != nullptr) {
        setenv(pName, pValue, 1);
    } else {
        unsetenv(pName);
    }
#endif
}

// ---------------------------------------------------------------------------
// 1) Environment variable override path — highest priority, normalized string
// ---------------------------------------------------------------------------
static void TestEnvOverride() {
    RwsAssets_Reset();
    SetEnv("CANNONCRUISE_STREAMS", "Z:/bogus_streams"); // NO trailing separator
    const char *pDir = RwsAssets_Dir();
    CHECK(pDir != nullptr);
    // normalize: trailing '/' must be appended
    CHECK(std::string(pDir) == "Z:/bogus_streams/");
    SetEnv("CANNONCRUISE_STREAMS", "Z:/bogus_streams/"); // trailing '/' PRESENT
    RwsAssets_Reset();
    CHECK(std::string(RwsAssets_Dir()) == "Z:/bogus_streams/"); // no double append

    // when env is invalid Resolve cannot find it → 0 + empty string
    char out[512];
    CHECK(RwsAssets_Resolve("Streams/MAINMENU.RWS", out, sizeof(out)) == 0);
    CHECK(out[0] == '\0');

    SetEnv("CANNONCRUISE_STREAMS", nullptr);
    RwsAssets_Reset();
}

// ---------------------------------------------------------------------------
// 2) Sentinel scan — finds the Streams/ root from the prefix depth
// ---------------------------------------------------------------------------
static void TestSentinelScan() {
    RwsAssets_Reset();
    const char *pDir = RwsAssets_Dir();
    CHECK(pDir != nullptr);
    // must be non-empty no matter how far up the root is from build/tests CWD
    // (5 candidate prefixes: "" → "../../../../../")
    CHECK(pDir[0] != '\0');

    // if the root was found, the sentinel must be resolvable
    char out[512];
    const int found =
        RwsAssets_Resolve("Streams/MAINMENU.RWS", out, sizeof(out));
    if (found) {
        // path must start with the root dir + file name
        CHECK(std::strncmp(out, pDir, std::strlen(pDir)) == 0);
        CHECK(std::strstr(out, "MAINMENU.RWS") != nullptr);
        // and it should actually be openable
        FILE *f = std::fopen(out, "rb");
        CHECK(f != nullptr);
        if (f != nullptr) {
            std::fclose(f);
        }
    } else {
        // CWD may be outside the tree — this is not a failure, but
        // let's report it so the scan depth doesn't fall short.
        std::printf("NOTE: Streams/MAINMENU.RWS sentinel not found "
                    "(CWD may be outside the root), pDir='%s'\n", pDir);
    }
}

// ---------------------------------------------------------------------------
// 3) Cache — second call returns the same result, returns 0 if file is missing
// ---------------------------------------------------------------------------
static void TestCacheAndMissing() {
    RwsAssets_Reset();
    char a[512], b[512];
    const int ra = RwsAssets_Resolve("Streams/MAINMENU.RWS", a, sizeof(a));
    const int rb = RwsAssets_Resolve("Streams/MAINMENU.RWS", b, sizeof(b));
    CHECK(ra == rb);
    CHECK(std::string(a) == std::string(b));

    // nonexistent file → 0, buffer is cleared
    char m[512] = {0x7f};
    CHECK(RwsAssets_Resolve("Streams/__NO_SUCH_FILE__.RWS", m, sizeof(m)) ==
          0);
    CHECK(m[0] == '\0');

    // null/empty input → safe
    CHECK(RwsAssets_Resolve(nullptr, m, sizeof(m)) == 0);
    CHECK(RwsAssets_Resolve("", m, sizeof(m)) == 0);
    CHECK(RwsAssets_Resolve("Streams/MAINMENU.RWS", nullptr, 512) == 0);

    // existing but different candidates (GLOBAL/VIKING01) must resolve consistently
    char g[512];
    const int rg = RwsAssets_Resolve("Streams/GLOBAL.RWS", g, sizeof(g));
    if (rg) {
        FILE *f = std::fopen(g, "rb");
        CHECK(f != nullptr);
        if (f != nullptr) {
            std::fclose(f);
        }
    }
}

// ---------------------------------------------------------------------------
// 4) Reset — env variable is re-evaluated
// ---------------------------------------------------------------------------
static void TestResetReevaluatesEnv() {
    RwsAssets_Reset();
    SetEnv("CANNONCRUISE_STREAMS", "Y:/first/");
    CHECK(std::string(RwsAssets_Dir()) == "Y:/first/");

    SetEnv("CANNONCRUISE_STREAMS", "Y:/second/");
    // while cached, the OLD value must be kept
    CHECK(std::string(RwsAssets_Dir()) == "Y:/first/");
    // re-evaluated after Reset
    RwsAssets_Reset();
    CHECK(std::string(RwsAssets_Dir()) == "Y:/second/");

    SetEnv("CANNONCRUISE_STREAMS", nullptr);
    RwsAssets_Reset();
}

int main() {
    TestEnvOverride();
    TestSentinelScan();
    TestCacheAndMissing();
    TestResetReevaluatesEnv();

    std::printf("rwsassets_test: %d checks, %d failures\n", g_checks,
                g_failures);
    return (g_failures == 0) ? 0 : 1;
}
