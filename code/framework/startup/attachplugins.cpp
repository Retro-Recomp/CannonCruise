// ============================================================================
// CannonCruise - RenderWare Plugin Attachment (attachplugins.cpp)
// Original file: D:\Projects\CannonCruisePC\code\framework\startup\attachplugins.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
//
// FUN_00419e40 — AttachPlugins()
// This function starts all RenderWare 3.6 plugins used by the game engine
// and attaches them to the engine. It runs at application startup, before
// RwEngineInit() is called.
//
// Binary Addresses:
//   AttachPlugins = 0x00419e40
//   Plugin table = PTR_FUN_005ef440 (5 function pointers)
//   RtAnimInitialize = 0x005a7180 (rwID 0x1b7)
//   RtAnimRegisterScheme = 0x005a8b30
// ============================================================================

#include <cassert>

// RenderWare 3.6 SDK Headers
#include <rwcore.h>         // RwEngineRegisterPlugin, vs.
#include <rpworld.h>        // RpWorldPluginAttach
#include <rphanim.h>        // RpHAnimPluginAttach (rwID = 0x116)
#include <rpcollision.h>    // RpCollisionPluginAttach (rwID = 0x11e)
#include <rpskin.h>         // RpSkinPluginAttach (rwID = 0x10c)
#include <rtanim.h>         // RtAnimInitialize (rwID = 0x1b7)
#include <rtcmpkey.h>       // RtAnimRegisterInterpolationScheme (compressed keyframe)

// Game-specific plugin (FUN_0042b880)
// Comes from D:\Projects\CannonCruisePC\code\framework
extern RwBool GamePluginAttach(void);

// ============================================================================
// Plugin Function Table (PTR_FUN_005ef440)
// At address 0x005ef440 in the binary there are 5 function pointers.
// The loop iterates pcVar9 from 0 to 4 (do-while, pcVar9 < 5).
//
// Table Order and RW Plugin ID Mapping:
//   [0] FUN_004dd760 = RpWorldPluginAttach   (sub-IDs: 0x501–0x50b)
//   [1] FUN_0059f690 = RpHAnimPluginAttach   (rwID = 0x116)
//   [2] FUN_005a44a0 = RpCollisionPluginAttach (rwID = 0x11e)
//   [3] FUN_005a65a0 = RpSkinPluginAttach    (rwID = 0x10c)
//   [4] FUN_0042b880 = GamePluginAttach      (game-specific)
// ============================================================================

// Plugin attach functions — all return RwBool (success=TRUE, failure=FALSE)
typedef RwBool (*PluginAttachFunc)(void);

static PluginAttachFunc s_pluginTable[] = {
    RpWorldPluginAttach,        // [0] 0x004dd760 — World system
    RpHAnimPluginAttach,        // [1] 0x0059f690 — Hierarchical animation
    RpCollisionPluginAttach,    // [2] 0x005a44a0 — Collision system
    RpSkinPluginAttach,         // [3] 0x005a65a0 — Character skin/mesh
    GamePluginAttach,           // [4] 0x0042b880 — Game-specific plugin
};

static const int NUM_PLUGINS = sizeof(s_pluginTable) / sizeof(s_pluginTable[0]);

// ============================================================================
// AttachPlugins (FUN_00419e40)
// Binary Address: 0x00419e40
// XREF: FUN_0041a680:0041aa84(c) — called from startup.cpp
//
// Flow:
//   1. Call RtAnimInitialize() (0x005a7180)
//      -> On failure assert: "Unable to initialize RtAnim" (line 0xdf = 223)
//   2. Loop over the 5 plugins in the plugin table:
//      -> For each, CALL [EDI*4 + PTR_FUN_005ef440]
//      -> On failure assert: "Unable to Attach Plugin: Index [N]" (line 0xf9 = 249)
//   3. Call RtAnimRegisterInterpolationScheme() (0x005a8b30)
//      -> On failure assert: "Compressed keyframe scheme registration failed" (line 0x10e = 270)
//   4. Successful return
// ============================================================================
RwBool AttachPlugins(void)
{
    // ---------------------------------------------------------------
    // Step 1: Initialize the RtAnim Module
    // ASM: 00419e72  CALL FUN_005a7180
    //       00419e77  TEST EAX,EAX
    //       00419e79  JNZ  LAB_00419fce (jump to the loop if successful)
    // On failure: assert "Unable to initialize RtAnim" (line 0xdf = 223)
    // ---------------------------------------------------------------
    RwBool result = RtAnimInitialize();
    assert(result && "Unable to initialize RtAnim");

    // ---------------------------------------------------------------
    // Step 2: Plugin Table Loop
    // ASM: LAB_00419fce — EDI=0, EBX=DAT_005ef324
    //       LAB_00419fdf — CALL dword ptr [EDI*4 + PTR_FUN_005ef440]
    //       pcVar9++ ; repeat while pcVar9 < 5
    // On failure: assert "Unable to Attach Plugin: Index [N]" (line 0xf9 = 249)
    // ---------------------------------------------------------------
    for (int i = 0; i < NUM_PLUGINS; i++)
    {
        result = s_pluginTable[i]();
        assert(result && "Unable to Attach Plugin: Index [i]");
    }

    // ---------------------------------------------------------------
    // Step 3: Register the Compressed Keyframe Animation Scheme
    // ASM: 0041a165  CALL FUN_005a8b30
    //       0041a16a  TEST EAX,EAX
    //       0041a16c  JNZ  (continue if successful)
    // On failure: assert "Compressed keyframe scheme registration failed" (line 0x10e = 270)
    // ---------------------------------------------------------------
    result = RtAnimRegisterInterpolationScheme(
        RtCompressedKeyFrameGetCustomKeyFrameScheme()
    );
    assert(result && "Compressed keyframe scheme registration failed");

    return TRUE;
}
