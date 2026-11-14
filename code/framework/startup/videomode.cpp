// ============================================================================
// CannonCruise - Video Mode & Camera Setup (videomode.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\startup\videomode.cpp
// Reverse-engineered and rewritten by analyzing with CC_ida.asm.
//
// Binary Functions:
//   FUN_0041af10 — CameraInit  (assert "No best sub-system", videomode.cpp:228)
//   FUN_0041ac00 — CameraShutdown (first step of the startup shutdown chain)
//
// Note: The file context of FUN_0041ac00 is only known from the call in startup.cpp;
//      the actual "videomode.cpp" assert (line 0xE4 = 228)
//      is inside FUN_0041af10 and proves this file contains CameraInit.
// ============================================================================

#include "rwcore.h"
#include "PhysicsWorld.h"
#include <cassert>

// ---------------------------------------------------------------------------
// Scoring helpers (FUN_0041af10 score comparison — binary weights)
// ---------------------------------------------------------------------------
namespace
{
    // Scores a candidate video mode against the requested width/height/depth values.
    // Weights as in the binary: depth match 16, size match 4,
    // dimension exceeding the requested value 2 (possibly a match bonus).
    // Note: In the ASM there is an extra low-weight (+1) match step as well;
    // it is folded into the width match because the stack offset read is ambiguous.
    int ScoreVideoMode(const RwVideoMode& vm, int width, int height, int depth)
    {
        int score = 0;

        if (vm.width == width)
            score += 4;
        else if (vm.width > width)
            score += 2;

        if (vm.height == height)
            score += 4;
        else if (vm.height > height)
            score += 2;

        if (vm.depth == depth)
            score += 16;

        return score;
    }

    // For two tied candidates: in the binary an area (width*height) comparison
    // is done, and the smaller area is preferred.
    RwBool IsBetterVideoMode(const RwVideoMode& candidate, int candidateScore,
                             const RwVideoMode& currentBest, int currentScore)
    {
        if (candidateScore != currentScore)
            return candidateScore > currentScore;

        const long long candidateArea =
            (long long)candidate.width * (long long)candidate.height;
        const long long bestArea =
            (long long)currentBest.width * (long long)currentBest.height;
        return candidateArea < bestArea;
    }
} // namespace

// ============================================================================
// CameraInit (FUN_0041af10)
// Binary Adresi: 0x0041af10
// XREF: sub_41A680 (startup init): 0041aae5 — startup.cpp "Step 10"
//
// Scans all subsystems and video modes; picks the mode that best matches
// the given width/height/depth values by scoring. The chosen mode
// is written through *videoMode and TRUE is returned.
//
// If no best mode is ever found, in the binary:
//   the "No best sub-system" assert (videomode.cpp, line 0xE4 = 228)
//   → INT3 → (if no assert flag) exit(0)
// ============================================================================
RwBool CameraInit(RwVideoMode* videoMode, int width, int height, int depth)
{
    int bestSubsystem = -1;
    int bestVideoMode = -1;
    RwVideoMode bestModeInfo;
    bestModeInfo.width = 0;
    bestModeInfo.height = 0;
    bestModeInfo.depth = 0;
    bestModeInfo.flags = 0;
    bestModeInfo.unknown2 = 0;
    bestModeInfo.unknown3 = 0;

    const int numSubSystems = RwEngineGetNumSubSystems();

    for (int subsystem = 0; subsystem < numSubSystems; ++subsystem)
    {
        if (!RwEngineSetSubSystem(subsystem))
            continue;

        const int numVideoModes = RwEngineGetNumVideoModes();

        for (int modeIndex = 0; modeIndex < numVideoModes; ++modeIndex)
        {
            RwVideoMode candidate;
            if (!RwEngineGetVideoModeInfo(&candidate, modeIndex))
                continue;

            if (bestVideoMode < 0)
            {
                // The first candidate is taken directly as the best mode (binary:
                // the copy path while there is no best mode yet, loc_41B05F).
                bestSubsystem = subsystem;
                bestVideoMode = modeIndex;
                bestModeInfo = candidate;
                continue;
            }

            const int candidateScore =
                ScoreVideoMode(candidate, width, height, depth);
            const int currentScore =
                ScoreVideoMode(bestModeInfo, width, height, depth);

            if (IsBetterVideoMode(candidate, candidateScore,
                                  bestModeInfo, currentScore))
            {
                bestSubsystem = subsystem;
                bestVideoMode = modeIndex;
                bestModeInfo = candidate;
            }
        }
    }

    // -----------------------------------------------------------------
    // No best mode found?
    // ASM: "No best sub-system" → In file videomode.cpp, line 0E4h (228)
    //      → "Assert failed:" → INT3 → exit(0)
    // -----------------------------------------------------------------
    if (bestSubsystem < 0 || bestVideoMode < 0)
    {
        assert(!"No best sub-system");
        return FALSE;
    }

    // Apply the chosen subsystem and video mode
    if (!RwEngineSetSubSystem(bestSubsystem))
        return FALSE;

    if (!RwEngineSetVideoMode(bestVideoMode))
        return FALSE;

    // Write the chosen mode info into the caller's videoMode structure
    // (ASM: the candidate struct's 6 dwords are copied — rep movsd, 24 bytes).
    if (videoMode)
        *videoMode = bestModeInfo;

    return TRUE;
}

// ============================================================================
// CameraShutdown (FUN_0041ac00)
// Binary Address: 0x0041ac00
// XREF: sub_41AB90 (startup shutdown): 0041ab91 — startup.cpp "Step 1"
//
// Binary behavior:
//   1. DAT_00635af4 (the main game context singleton — called "Havok
//      singleton" by the startup code) is read.
//   2. If NULL: the "ms_pSingleton != NULL Failed" assert (Singleton.h:58)
//      is triggered; if no assert flag, exit(0).
//   3. Otherwise: the singleton's first virtual method is called with 1 argument
//      (vtable slot 0 — a shutdown-like context teardown).
//   4. DAT_00635af4 = NULL is written (the object is not freed from memory).
//
// Codebase note: the concrete type of the singleton (FUN_0049c870 ctor) has not
// been matched yet; once the Havok/physics integration (Phase 4.4b) is complete,
// the call will be hooked up. Only the assert contract is implemented for now.
// ============================================================================
void CameraShutdown(void)
{
    CPhysicsWorld* pContext = CPhysicsWorld::GetSingletonPtr();

    assert(pContext != 0 && "ms_pSingleton != NULL Failed");

    if (pContext)
    {
        // TODO(Phase 4.4b): the virtual slot0(pContext, 1) call in the binary —
        // the first virtual method of the FUN_0049c870 ctor (shutdown-like).
        // Not called until the counterpart is modeled in CPhysicsWorld.
    }

    // Binary: DAT_00635af4 = NULL — the singleton pointer is zeroed,
    // the object memory is not freed (the Singleton.h dtor does that).
}
