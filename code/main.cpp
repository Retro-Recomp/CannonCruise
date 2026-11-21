// ============================================================================
// CannonCruise - Platform Smoke + RWS Pipeline Entry (main.cpp)
// Phase 4.5: Link verification (-force_load resolves all symbols).
// Phase 4.6n: Render platform frame loop smoke.
// Phase 4.6o+: Optional RWS argument -- end-to-end world pipeline:
//   RWS inventory -> WorldGeometry clump instantiate -> RenderPlatform add
//   -> StaticCollisionData (.col) trimesh shape -> frame loop
//   (clear/Begin -> world render -> Im2D frame -> End). NULL platform
//   headless (device no-op); GL3 build has real window/pixel.
//
// Usage: ./CannonCruise [world.RWS]
//
// Game's real runtime entry on Windows is startup.cpp / WinMain.
// ============================================================================

#include "CameraState.h"
#include "Ocean.h"
#include "PhysicsBackend.h"
#include "PhysicsWorld.h"
#include "TowerCatapult.h"
#include "Cannonball.h"
#include "RenderPlatform.h"
#include "Rt2dMaestro.h"
#include "Rt2dMaestroStream.h"
#include "MenuBehavior.h"
#include "AfterGameMenuBehavior.h"
#include "RwsAssets.h"   // .RWS asset path resolver (central)
#include "FmvScene.h"    // FMV scene identifier (CFMVBehavior) + target transition

// FMV story (SCR.mov/SCR1.mov) game state machine -- Phase 4.6s target.
// BOOT -> LOGO_ITE -> LOADING -> MAINMENU -> OPTIONS -> SOUND/CREDITS -> MAPMENU
// -> LEVELINTRO -> GAMEPLAY(NORMAL/CANNON/BINOCULARS) -> PAUSE -> LEVELCOMPLETE.
// Currently BOOT->MAINMENU->GAMEPLAY skeleton works; intermediate states are phased in later.
enum EGameState {
    GAMESTATE_BOOT, GAMESTATE_LOGO_ITE, GAMESTATE_LOADING,
    GAMESTATE_MAINMENU, GAMESTATE_OPTIONS, GAMESTATE_SOUND,
    GAMESTATE_CREDITS, GAMESTATE_MAPMENU, GAMESTATE_LEVELINTRO,
    GAMESTATE_GAMEPLAY, GAMESTATE_PAUSE, GAMESTATE_LEVELCOMPLETE,
    GAMESTATE_OUTRO, GAMESTATE_QUIT
};
static EGameState s_gameState = GAMESTATE_BOOT;
// Camera stays inside the map: XZ over the ocean grid (small inset),
// Y between water level and island top + margin. Computed after world load.
static RwReal s_camBoundMin[3] = {0.0f, 0.0f, 0.0f};
static RwReal s_camBoundMax[3] = {0.0f, 0.0f, 0.0f};
static RwBool s_camBoundsSet = FALSE;

static void ClampCameraToMap(RwReal camPos[3]) {
    if (s_camBoundsSet != TRUE) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (camPos[i] < s_camBoundMin[i]) camPos[i] = s_camBoundMin[i];
        if (camPos[i] > s_camBoundMax[i]) camPos[i] = s_camBoundMax[i];
    }
}
#include "RwStudioStream.h"
#include "ShowText.h"
#include "StaticCollisionData.h"
#include "WorldGeometry.h"
#include "WorldSpawn.h"
#include "RwsResources.h"
#include "LocalizationManager.h"
#include "BinkPlayer.h"  // binkw32 player - .bik playback (sub_45F350/45F270)
#include "LevelScript.h"
#include "LoadingScreen.h" // Phase 4.6ab � LOADING overlay (GLOBAL art)
#include "rwcore.h"
#include "AudioManager.h"
#include "SpeakManager.h"
#include "PlayerShip.h"   // Phase 4.6s -- placeholder ship + chase camera
#include "HUD.h"         // Sprint 2 � minimal HUD overlay
#include "SaveGameManager.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// Is FMV chain terminal target a PHASE? MainMenu/MAPMENU are already handled
// by menu/map phases; others (Nordic01/Nordic02/Iduna02/Iduna03...)
// are LEVELS -> load world to that target at chain end (binary iMsgChangeScene
// counterpart). Case-insensitive (target "MainMenu", file MAINMENU).
int IsFmvPhaseTarget(const char *pTarget) {
    if (pTarget == nullptr) {
        return 0;
    }
    char up[64] = {0};
    for (size_t i = 0; i + 1 < sizeof(up) && pTarget[i] != 0; ++i) {
        const char c = pTarget[i];
        up[i] = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
    }
    return (std::strcmp(up, "MAINMENU") == 0 ||
            std::strcmp(up, "MAPMENU") == 0)
               ? 1
               : 0;
}

/// RWS inventory + WorldGeometry clump + .col shape -- single world pipeline.
struct RwsPipeline {
    void *pClump;             ///< rw::Clump* (added to RenderPlatform)
    void *pCollisionShape;    ///< PhysicsShape* (btBvhTriangleMeshShape)
    RwUInt32 assetCount;
    RwUInt32 worldGeomBytes;
    RwUInt32 collisionBytes;
    StaticCollisionMesh collisionMesh;
};

RwBool RwsPipeline_Load(const char *pPath, RwsPipeline *pOut) {
    std::memset(pOut, 0, sizeof(*pOut));

    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    if (!pFile) {
        std::fprintf(stderr, "RWS open failed: %s\n", pPath);
        return FALSE;
    }

    RwStudioAsset asset;
    while (RwStudioFileNextAsset(pFile, &asset)) {
        ++pOut->assetCount;

        if (std::strcmp(asset.typeName, "WorldGeometry") == 0 &&
            asset.resourceLength > 0) {
            unsigned char *pBuf = static_cast<unsigned char *>(
                std::malloc(asset.resourceLength));
            if (!pBuf) {
                continue;
            }
            if (RwStudioAssetReadResource(pFile, &asset, pBuf,
                                          asset.resourceLength) ==
                asset.resourceLength) {
                WorldGeometryInfo info;
                if (WorldGeometry_Inspect(pBuf, asset.resourceLength,
                                          &info) == TRUE) {
                    pOut->pClump =
                        WorldGeometry_CreateClump(pBuf, asset.resourceLength);
                    pOut->worldGeomBytes = asset.resourceLength;
                    std::printf(
                        "WorldGeometry: %u+%u atomics, %u verts, %u tris "
                        "(%u bytes) -> clump %s\n",
                        info.atomicCount, info.atomicGroup2Count,
                        info.totalVertices + info.group2Vertices,
                        info.totalTriangles + info.group2Triangles,
                        asset.resourceLength, pOut->pClump ? "OK" : "FAIL");
                }
            }
            std::free(pBuf);
        } else if (std::strcmp(asset.typeName, "StaticCollisionData") == 0 &&
                   asset.resourceLength > 0) {
            unsigned char *pBuf = static_cast<unsigned char *>(
                std::malloc(asset.resourceLength));
            if (!pBuf) {
                continue;
            }
            if (RwStudioAssetReadResource(pFile, &asset, pBuf,
                                          asset.resourceLength) ==
                asset.resourceLength) {
                if (StaticCollisionData_Decode(pBuf, asset.resourceLength,
                                               &pOut->collisionMesh) ==
                    TRUE) {
                    pOut->pCollisionShape = StaticCollisionData_CreateShape(
                        &pOut->collisionMesh);
                    pOut->collisionBytes = asset.resourceLength;
                    std::printf(
                        "StaticCollisionData: %u verts, %u tris "
                        "(%u bytes) -> shape %s\n",
                        pOut->collisionMesh.vertexCount,
                        pOut->collisionMesh.triangleCount,
                        asset.resourceLength,
                        pOut->pCollisionShape ? "OK" : "FAIL");
                }
            }
            std::free(pBuf); // shape mesh is copied -- payload freed
        }
    }
    RwStudioFileClose(pFile);
    return TRUE;
}

void RwsPipeline_Unload(RwsPipeline *pPipe) {
    if (pPipe->pClump) {
        WorldGeometry_DestroyClump(pPipe->pClump);
        pPipe->pClump = nullptr;
    }
    if (pPipe->pCollisionShape) {
        PhysicsShape_Destroy(pPipe->pCollisionShape);
        pPipe->pCollisionShape = nullptr;
    }
}

// Phase 4.6p -- World camera pose from RWS 0x0704 CWbCameraBehavior registrations
// (VIKING01 obj[264]: id0 pos, id6 dir, id4 FOV). First props CWbCameraBehavior is applied
// (binary: CCameraState singleton).
RwBool RwsCamera_LoadPose(const RwChar *pPath, RwReal posOut[3],
                          RwReal lookOut[3]) {
    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    if (!pFile) {
        return FALSE;
    }
    CCameraState state;
    std::memset(&state, 0, sizeof(state));
    RwBool bPos = FALSE;
    RwBool bDir = FALSE;
    RwStudioAsset asset;
    while (RwStudioFileNextAsset(pFile, &asset) == TRUE) {
        const RwStudioObject *pObj = RwStudioFileGetObject(pFile);
        if (!pObj) {
            continue;
        }
        for (RwUInt32 i = 0; i < pObj->behaviourCount; ++i) {
            const RwStudioBehaviour *pBeh = &pObj->behaviours[i];
            if (std::strcmp(pBeh->className, "CWbCameraBehavior") != 0 ||
                pBeh->propCount == 0) {
                continue;
            }
            for (RwUInt32 j = 0; j < pBeh->propCount; ++j) {
                const RwStudioProperty &prop = pBeh->props[j];
                if (prop.size == 20) {
                    CCameraStateApplyVec3(&state, prop.id, prop.value);
                    if (prop.id == 0x0) {
                        bPos = TRUE;
                    } else if (prop.id == 0x6) {
                        bDir = TRUE;
                    }
                } else if (prop.size == 12) {
                    RwUInt32 raw = 0;
                    std::memcpy(&raw, prop.value, 4);
                    state.ApplyAttribute(prop.id, raw);
                }
            }
            if (bPos == TRUE && bDir == TRUE) {
                break;
            }
        }
        if (bPos == TRUE && bDir == TRUE) {
            break;
        }
    }
    RwStudioFileClose(pFile);
    if (bPos != TRUE || bDir != TRUE) {
        return FALSE;
    }
    posOut[0] = state.m_pos[0];
    posOut[1] = state.m_pos[1];
    posOut[2] = state.m_pos[2];
    // id6 (+0x24) ABSOLUTE look target (lookAt point) -- NOT a direction vector:
    // VIKING01 pos=(0.35,7,0), target=(0.85,0.85,0.85) ~= looking down at world center.
    // pos+dir would look up-to-sky instead.
    lookOut[0] = state.m_dir[0];
    lookOut[1] = state.m_dir[1];
    lookOut[2] = state.m_dir[2];
    return TRUE;
}

// --------------------------------------------------------------------------
// FMV playback -- "no-op" callback running inside BinkPlayer_Wait busy-spin.
// As verified in BinkPlayer.h, Sleep BREAKS DECODE (frames freeze) ->
// only message pumping during wait.
// Return !=0 -> stop playback (Esc/close).
//
// s_fmvArmed: disables skip on first frame to swallow keypresses leaking
// from previous phase (same logic as s_prevEsc on title card).
// --------------------------------------------------------------------------
RwBool s_fmvArmed = FALSE;
RwBool s_fmvSkip = FALSE;

int FmvSpinIdle(void) {
    RwPlatformPollEvents();
    if (RwPlatformShouldClose() == TRUE) {
        s_fmvSkip = TRUE;
        return 1;
    }
    if (s_fmvArmed == TRUE &&
        (RwPlatformKeyDown(RW_KEY_ESCAPE) || RwPlatformKeyDown('Q') ||
         RwPlatformKeyDown(' '))) {
        s_fmvSkip = TRUE;
        return 1;
    }
    return 0;
}

} // namespace

// Pause overlay: dims the frozen world and offers RESUME / QUIT TO DESKTOP.
// selection: 0 = RESUME, 1 = QUIT TO DESKTOP.
static void RenderPauseOverlay(RwInt32 selection) {
    const RwReal kScreenW = 1280.0f, kScreenH = 720.0f;
    RenderPlatform_BeginOverlay2D();
    // dim backdrop
    RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, kScreenW, kScreenH,
                                   0x90000000u);
    // parchment panel with dark border
    const RwReal pw = 400.0f, ph = 230.0f;
    const RwReal px = (kScreenW - pw) * 0.5f;
    const RwReal py = (kScreenH - ph) * 0.5f - 20.0f;
    RenderPlatform_DrawOverlayQuad(px - 5, py - 5, pw + 10, ph + 10,
                                   0xFF1A0F08u);
    RenderPlatform_DrawOverlayQuad(px, py, pw, ph, 0xFFC7A05Eu);
    // heading
    const RwReal titleW = 6 * 6 * 3; // "PAUSED" at scale 3
    ShowText_DrawBitmapText("PAUSED", px + (pw - titleW) * 0.5f, py + 22.0f,
                            3, 0x4A, 0x28, 0x10, 0xFF);
    // options
    const char *options[2] = {"RESUME", "QUIT TO DESKTOP"};
    for (int i = 0; i < 2; ++i) {
        const RwReal oy = py + 95.0f + (RwReal)i * 44.0f;
        const RwBool sel = (selection == i) ? TRUE : FALSE;
        if (sel == TRUE) {
            RenderPlatform_DrawOverlayQuad(px + 24, oy - 6, pw - 48, 30,
                                           0xFFD88A18u);
        }
        ShowText_DrawBitmapText(options[i], px + 60.0f, oy, 2,
                                sel ? 0x20u : 0x4Au,
                                sel ? 0x14u : 0x28u,
                                sel ? 0x08u : 0x10u,
                                0xFF);
        if (sel == TRUE) {
            ShowText_DrawBitmapText(">", px + 36.0f, oy, 2,
                                    0x4A, 0x28, 0x10, 0xFF);
        }
    }
    // hint line
    ShowText_DrawBitmapText("ESC - RESUME    ENTER - SELECT", px + 52.0f,
                            py + ph - 28.0f, 1, 0x6A, 0x4A, 0x28, 0xFF);
    RenderPlatform_EndOverlay2D();
}

// LEVELCOMPLETE summary: dim backdrop + parchment panel with the mission
// result. Same visual language as the pause overlay.
static void RenderLevelCompleteOverlay(RwInt32 score, RwInt32 towersDown,
                                       RwInt32 towersTotal, RwInt32 shots,
                                       RwInt32 hits) {
    const RwReal kScreenW = 1280.0f, kScreenH = 720.0f;
    RenderPlatform_BeginOverlay2D();
    RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, kScreenW, kScreenH,
                                   0x90000000u);
    const RwReal pw = 480.0f, ph = 300.0f;
    const RwReal px = (kScreenW - pw) * 0.5f;
    const RwReal py = (kScreenH - ph) * 0.5f - 20.0f;
    RenderPlatform_DrawOverlayQuad(px - 5, py - 5, pw + 10, ph + 10,
                                   0xFF1A0F08u);
    RenderPlatform_DrawOverlayQuad(px, py, pw, ph, 0xFFC7A05Eu);
    const char *title = "LEVEL COMPLETE";
    const RwReal titleW = (RwReal)std::strlen(title) * 6.0f * 3.0f;
    ShowText_DrawBitmapText(title, px + (pw - titleW) * 0.5f, py + 22.0f, 3,
                            0x4A, 0x28, 0x10, 0xFF);
    const RwInt32 accuracy = shots > 0 ? (hits * 100) / shots : 0;
    char line[64];
    const RwReal lx = px + 70.0f;
    std::snprintf(line, sizeof(line), "SCORE      %d", score);
    ShowText_DrawBitmapText(line, lx, py + 100.0f, 2, 0x4A, 0x28, 0x10, 0xFF);
    std::snprintf(line, sizeof(line), "TOWERS     %d/%d", towersDown,
                  towersTotal);
    ShowText_DrawBitmapText(line, lx, py + 140.0f, 2, 0x4A, 0x28, 0x10, 0xFF);
    std::snprintf(line, sizeof(line), "ACCURACY   %d%% (%d/%d)", accuracy,
                  hits, shots);
    ShowText_DrawBitmapText(line, lx, py + 180.0f, 2, 0x4A, 0x28, 0x10, 0xFF);
    ShowText_DrawBitmapText("ENTER - QUIT TO DESKTOP", px + 110.0f,
                            py + ph - 32.0f, 1, 0x6A, 0x4A, 0x28, 0xFF);
    RenderPlatform_EndOverlay2D();
}

// Shot statistics for the LEVELCOMPLETE summary.
static RwInt32 s_shotsFired = 0;
static RwInt32 s_shotsHit = 0;

int main(int argc, char **argv) {
    // Diagnostics: stdout UNBUFFERED -- stdio buffer flushed only on exit;
    // otherwise last output was lost on crash (e.g. 0xC0000005); needed to
    // see which step we were at.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("CannonCruise: link verification OK\n");

    // Engine boot -- keep the failing stage visible and avoid continuing after
    // a render-device open failure.
    if (RwEngineInit(0, 0, 0) != TRUE) {
        std::fprintf(stderr, "CannonCruise: engine init failed\n");
        return 2;
    }
    if (RwEngineOpen() != TRUE) {
        std::fprintf(stderr, "CannonCruise: engine open failed\n");
        RwEngineTerm();
        return 2;
    }
    if (RwEngineStart() != TRUE) {
        std::fprintf(stderr, "CannonCruise: engine start failed\n");
        RwEngineClose();
        RwEngineTerm();
        return 2;
    }

    // Localization Manager (sub_4918C0 / sub_4923A0: lang_<lang>.txt + win_<lang>.txd)
    // Screen resolution - FMV quad also uses this (.bik 640x448 ->
    // fits full screen; original game draws 640x448 raster to screen
    // with same layout).
    const RwInt32 kScreenW = 1280;
    const RwInt32 kScreenH = 720;
    if (RenderPlatform_Initialize(kScreenW, kScreenH) != TRUE) {
        std::fprintf(stderr, "CannonCruise: render platform init failed\n");
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        return 2;
    }

    // Localization Manager (sub_4918C0 / sub_4923A0: lang_<dil>.txt + win_<dil>.txd)
    CLocalizationManager locMgr;
    locMgr.LoadDefinitions();
    locMgr.SetLanguage(locMgr.GetCurrentLanguage());

    // Audio & Speech subsystems (Phase 4.6z + Turkish dubbing)
    CAudioManager audioMgr;
    CSpeakManager speakMgr;
    speakMgr.SetLanguage("Tyr");

    // World: if no argument, like game flow MAINMENU.RWS (binary:
    // intro -> menu). Note: menu RWS resources are NOT embedded (resLen=0,
    // resource path X:\RwStudio\...\{GUID}.dff -- build-output reference,
    // we don't have it) -> empty clump falls to first working world (VIKING01).
    const char *pWorldPath = (argc > 1) ? argv[1] : nullptr;
    // 4.6r -- boot flow: argumentless run uses entry chain (INTRO->menu)
    // overlaying the world; if world arg given, goes directly to
    // interactive world (entry chain skipped).
    const RwBool bBootFlow = (argc <= 1) ? TRUE : FALSE;
    // CANNONCRUISE_SCREENSHOT=<path.png> -- save first overlay frame as PNG
    // (intro frame; CANNONCRUISE_INTRO_FRAMES=0 for menu frame). One-time only.
    const char *pShotPath = std::getenv("CANNONCRUISE_SCREENSHOT");
    // CANNONCRUISE_SCREENSHOT_MAP=<path.png> -- MAPMENU frame.
    const char *pMapShotPath = std::getenv("CANNONCRUISE_SCREENSHOT_MAP");
    // World path updated by MAPMENU selection (setup block uses it).
    // 512: RwsAssets_Resolve returns 512-length paths -- no truncation.
    char mapLevelPath[512] = {0};

    RwsPipeline pipeline = {};
    RwBool bWorldLoaded = FALSE;
    // Resolved path lives in permanent buffer -- pWorldPath pointer must stay
    // valid throughout loop.
    static char resolvedWorldPath[512] = {0};
    if (pWorldPath != nullptr) {
        // Load world textures first (materials resolved at stream time).
        RenderPlatform_LoadWorldDictionary(pWorldPath);
        bWorldLoaded = RwsPipeline_Load(pWorldPath, &pipeline);
    } else {
        // RwsAssets_Resolve resolves relative depth (build layout) + env override
        // centrally -- no per-call prefix list needed anymore.
        static const char *kBootWorlds[] = {
            "Streams/MAINMENU.RWS", "Streams/VIKING01.RWS",
        };
        for (RwUInt32 i = 0; i < 2 && bWorldLoaded == FALSE; ++i) {
            if (!RwsAssets_Resolve(kBootWorlds[i], resolvedWorldPath,
                                   sizeof(resolvedWorldPath))) {
                continue; // file not present at this location
            }
            RenderPlatform_LoadWorldDictionary(resolvedWorldPath);
            if (RwsPipeline_Load(resolvedWorldPath, &pipeline) == TRUE &&
                pipeline.pClump != nullptr) {
                pWorldPath = resolvedWorldPath;
                bWorldLoaded = TRUE;
                break;
            }
            RwsPipeline_Unload(&pipeline); // no clump (menu: external dff)
            RenderPlatform_ClearWorldDictionary();
        }
    }
    // Note: world setup (AddClump/spawn/camera/ocean) moved AFTER boot-flow
    // -- MAPMENU selection set up with single load (reload + RemoveClump +
    // double-spawn not needed). (4.6r-4 MAPMENU)
    COcean *pOcean = nullptr; // quit-cleanup notification (setup below)
    // Sprint 1 -- test towers for the player's cannon + cannonball records
    std::vector<CTowerCatapult *> s_towers;
    std::vector<RwBool> s_towerIsMission; // parallel to s_towers
    std::vector<RpClump *> s_towerClumps;  // visible model (NULL: none)
    std::vector<const void *> s_towerDamaged; // RwsResource* "_Damaged"
    std::vector<std::vector<RwReal> > s_towerMatrices; // record transforms
    RpClump *s_pShipClump = nullptr;       // Hugo_Ship model (NULL: placeholder)
    // CShipEnemy ships: sail between their start point and their CWaypoint
    // (ping-pong), take cannonball hits and sink.
    struct EnemyShipRuntime {
        RpClump *clump;
        void *animator;          // paddle/rudder clips (NULL: none)
        RwInt32 clipPaddleL, clipPaddleR, clipRudder;
        RwReal scale;            // record matrix scale
        RwReal x, y, z, yaw;     // current pose (y = last wave height)
        RwReal homeX, homeZ;     // start point
        RwBool hasWaypoint;
        RwReal wpX, wpZ;
        RwBool toWaypoint;       // current leg
        RwReal hp;
        RwReal sinkTime;         // > 0 while sinking
        RwBool gone;             // sunk below the surface: no longer drawn
        RwChar spawnEvent[64];   // hidden until this script event ("" = at start)
        RwChar deathEvent[64];   // script event sent when sunk
    };
    std::vector<EnemyShipRuntime> s_enemies;
    // An enemy takes part once its spawn event (if any) has been sent.
    auto enemyPresent = [](const EnemyShipRuntime &e) -> RwBool {
        return (e.gone != TRUE &&
                (e.spawnEvent[0] == 0 || LevelScript_WasSent(e.spawnEvent) == TRUE))
                   ? TRUE : FALSE;
    };
    // CFishGreen collectibles (tutorial fishing; +health, fish counter).
    struct FishRuntime {
        RpClump *clump;
        RwReal x, z;
        RwChar spawnEvent[64];
        RwChar pickupEvent[64];
        RwBool collected;
    };
    std::vector<FishRuntime> s_fishes;
    RwInt32 s_enemiesSunk = 0;
    // Hugo_Ship HAnim clips driven as poses (see RpClumpPortAnimator*).
    void *s_pShipAnimator = nullptr;
    RwInt32 s_shipClipWingL = -1, s_shipClipWingR = -1, s_shipClipRudder = -1,
            s_shipClipWheel = -1, s_shipClipGun = -1;
    std::vector<CCannonball *> s_cannonballs;

    // 4.6r -- MAINMENU menu phase (entry chain: GLOBAL load -> FMV
    // chain (JINGLE) -> MAINMENU -> world). INTRO1-3/FMV {GUID} files not
    // available -> skip; MAINMENU external {GUID}.dff missing (empty clump)
    // but embedded Menu.anm stream rendered via Rt2dMaestro + command sequences walked.
    // CANNONCRUISE_MENU_FRAMES: frame cap (0=skip menu, -1=unlimited).
    RwBool bQuitApp = FALSE;
    // 4.6r-5 -- GLOBAL.RWS (binary sub_4A2010: GLOBAL -> JINGLE).
    //  - Embedded texture dictionary (via RwStudio parser; embedded
    //    asset NOT at 0xe8) decode + upload -- names are logged.
    //  - Global objectRecords (CAudioManager, CDifficultyLevels, CEventProxy
    //    CSystemCommands ... + rwID_2DMAESTRO) -- no registered factory;
    //    unknown classes are skipped (no registration) per binary behavior.
    //  - Image_Win32.rws (5.6 MB UI pack) -- format not solved, no decode;
    //    menu font/atlas comes from MAINMENU txd (4.6r). If decode is added
    //    it will be wired in here.
    {
        char globalPath[512] = {0};
        if (RwsAssets_Resolve("Streams/GLOBAL.RWS", globalPath,
                              sizeof(globalPath)) == TRUE) {
            const RwUInt32 gCount = MenuTextures_UploadEmbeddedFromRws(globalPath);
            std::printf("GLOBAL: %u embedded textures uploaded\n",
                        (unsigned)gCount);
        } else {
            std::printf("GLOBAL: Streams/GLOBAL.RWS not found -> skipped\n");
        }
    }
    if (bBootFlow == TRUE) {
        s_gameState = GAMESTATE_LOGO_ITE;
        // 4.6r-6 -- FMV scene phase (binary sub_4A2010: GLOBAL -> JINGLE; JINGLE
        // target MainMenu -> menu phase). Old assumption "INTRO1->2->3"
        // WRONG: INTRO1/2 not referenced by name in binary (only INTRO3
        // exists); INTRO1->Nordic01, INTRO2->Nordic02 = pre-level cuts,
        // INTRO3->MAPMENU. Full transition graph extracted in FmvScene.h.
        //
        // Every .RWS FMV descriptor is CFMVBehavior (sub_45F6D0 Register,
        // sub_45F950 apply) -> FMV name + TARGET SCENE (FmvScene module).
        // Video path ".\Video\Win32\<name>.bik" (case0 / loc_45FA0F); .bik and
        // external {GUID} txd/dff/col/pfm not available -> FMV skip and GO TO
        // TARGET SCENE -- Nordic connection is chain target following.
        int introCap = 180; // ~3 sn @60fps; CANNONCRUISE_INTRO_FRAMES
        const char *introEnv = std::getenv("CANNONCRUISE_INTRO_FRAMES");
        if (introEnv != nullptr) {
            introCap = std::atoi(introEnv);
        }
        // CANNONCRUISE_FMV: starting descriptor (default JINGLE =
        // binary startup). E.g. Streams/INTRO1.RWS -> target Nordic01.
        char fmvRel[512] = {0};
        const char *fmvEnv = std::getenv("CANNONCRUISE_FMV");
        std::snprintf(fmvRel, sizeof(fmvRel), "%s",
                      (fmvEnv != nullptr) ? fmvEnv : "Streams/JINGLE.RWS");
        // Chain terminal target + resolved next path (level start).
        char fmvTerminalTarget[64] = {0};
        char fmvNextRel[512] = {0};
        for (RwUInt32 ii = 0; ii < 8 && bQuitApp == FALSE; ++ii) {
            // FMV descriptor: CFMVBehavior -> FMV name + target scene
            FmvSceneDescriptor fmv;
            if (FmvScene_Load(fmvRel, &fmv) != 1) {
                std::printf("FMV: %s -> not a descriptor (level/world) -> "
                            "end of chain\n",
                            fmvRel);
                break;
            }
            char video[512] = {0};
            const int hasVideo =
                FmvScene_VideoPath(fmv.behaviour, video, sizeof(video));
            // Resolve .bik real path (CWD + asset root candidates) - used at
            // playback time; FmvScene_VideoExists scans the same order.
            char videoFull[512] = {0};
            const int videoOk =
                FmvScene_VideoResolve(fmv.behaviour, videoFull,
                                      sizeof(videoFull));
            char nextRel[512] = {0};
            const int hasNext =
                FmvScene_NextScenePath(fmv.target, nextRel, sizeof(nextRel));
            // target connection visible here: target -> Streams/<TARGET>.RWS
            std::printf("FMV[%u]: %s fmv='%s' target='%s'%s%s\n",
                        (unsigned)ii, fmv.guid, fmv.behaviour, fmv.target,
                        (hasNext == 1) ? " → " : "",
                        (hasNext == 1) ? nextRel : "(no scene)");
            if (hasVideo == 1) {
                std::printf("  video: %s (%s)\n", video,
                            (videoOk == 1) ? "play" : "none -> skip");
            }
            // title card loop
            // ------------------------------------------------------------------
            // FMV VIDEO PLAYBACK (binkw32) - binary sub_45F350 / sub_45F270.
            // If .bik exists, REAL video played instead of title card; after
            // frame ends (or skipped via Esc) chain continues to TARGET SCENE
            // (JINGLE->MainMenu, INTRO1->Nordic01, OUTRO3->Credits).
            //
            // Loop mirrors binary: while (BinkWait()) busy-spin ->
            // BinkDoFrame -> BinkCopyToBuffer(mode 0x80000003) -> BinkNextFrame.
            // BinkWait limits frame rate to 30 fps; Sleep NOT added during
            // wait (breaks decode) - only messages pumped.
            // ------------------------------------------------------------------
            RwBool bFmvPlayed = FALSE;
            if (videoOk == 1) {
                void *pBink = BinkPlayer_Open(videoFull);
                if (pBink != nullptr) {
                    const int vw = BinkPlayer_Width(pBink);
                    const int vh = BinkPlayer_Height(pBink);
                    std::printf("  play: %s (%dx%d, %d frames) %s\n", videoFull,
                                vw, vh, BinkPlayer_TotalFrames(pBink),
                                BinkPlayer_LastError());
                    std::vector<unsigned char> fmvPixels((size_t)vw *
                                                         (size_t)vh * 4u);
                    const int fmvPitch = vw * 4;
                    int fmvShown = 0;
                    s_fmvArmed = FALSE; // skip disabled until first frame
                    s_fmvSkip = FALSE;
                    for (;;) {
                        // wait until frame ready (busy-spin + message pumping;
                        // FmvSpinIdle breaks on close/Esc)
                        BinkPlayer_Wait(pBink, FmvSpinIdle);
                        if (s_fmvSkip == TRUE) {
                            break;
                        }
                        if (BinkPlayer_IsFinished(pBink) == 1) {
                            break;
                        }
                        if (BinkPlayer_DecodeRGBA(pBink, fmvPixels.data(),
                                                  fmvPitch) != 1) {
                            std::printf("  video frame error (%s)\n",
                                        BinkPlayer_LastError());
                            break;
                        }
                        s_fmvArmed = TRUE;
                        if (RenderPlatform_UploadTexture(
                                "FMV", vw, vh, fmvPixels.data(),
                                fmvPitch) != TRUE) {
                            break;
                        }
                        if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
                            break;
                        }
                        // video overlay - ZTEST off, fullscreen quad
                        // (640x448 .bik matches screen resolution; original
                        // game draws 640x448 raster to screen)
                        RenderPlatform_BeginOverlay2D();
                        RenderPlatform_DrawTexturedQuad(
                            0.0f, 0.0f, (RwReal)kScreenW, (RwReal)kScreenH,
                            "FMV");
                        RenderPlatform_EndOverlay2D();
                        // Frame 30 (~1 s): the first frames can still show
                        // stale render-target memory before the video texture
                        // is drawn.
                        if (pShotPath != nullptr && fmvShown == 30) {
                            RenderPlatform_RequestScreenshot(pShotPath);
                        }
                        RenderPlatform_EndFrame();
                        bFmvPlayed = TRUE;
                        ++fmvShown;
                        BinkPlayer_Next(pBink);
                    }
                    BinkPlayer_Close(pBink);
                    std::printf("  video finished: %d frames%s played\n", fmvShown,
                                (s_fmvSkip == TRUE) ? " (skipped)" : "");
                    RenderPlatform_ClearTextures(); // release FMV texture
                } else {
                    std::printf("  video failed to open: %s (%s)\n", videoFull,
                                BinkPlayer_LastError());
                }
            }
            if (bFmvPlayed == FALSE) {

            static RwBool s_prevSkip = FALSE;
            // Consume a key already held on phase entry to avoid skipping
            // before the opening card is visible.
            RwPlatformPollEvents();
            s_prevSkip = (RwBool)(RwPlatformKeyDown(RW_KEY_ENTER) ||
                                  RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                                  RwPlatformKeyDown('Q') ||
                                  RwPlatformKeyDown(' '));
            for (int fr = 0;; ++fr) {
                if (introCap >= 0 && fr >= introCap) {
                    break;
                }
                RwPlatformPollEvents();
                if (RwPlatformShouldClose() == TRUE) {
                    bQuitApp = TRUE;
                    break;
                }
                const RwBool skip = RwPlatformKeyDown(RW_KEY_ENTER) ||
                                    RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                                    RwPlatformKeyDown('Q') ||
                                    RwPlatformKeyDown(' ');
                if (skip == TRUE && s_prevSkip == FALSE) {
                    s_prevSkip = skip;
                    break;
                }
                s_prevSkip = skip;
                if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
                    break;
                }
                if (bWorldLoaded == TRUE && pipeline.pClump != nullptr) {
                    RenderPlatform_RenderScene();
                }
                // Branded fallback for an unavailable FMV: keep the opening
                // cinematic rather than exposing asset diagnostics.
                RenderPlatform_BeginOverlay2D();
                RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, (RwReal)kScreenW,
                                               (RwReal)kScreenH, 0xFF07151Du);
                RenderPlatform_DrawOverlayQuad(0.0f, 0.0f, (RwReal)kScreenW,
                                               8.0f, 0xFF18333Au);
                RenderPlatform_DrawOverlayQuad(
                    0.0f, (RwReal)kScreenH - 8.0f, (RwReal)kScreenW, 8.0f,
                    0xFF18333Au);
                RenderPlatform_DrawOverlayQuad(150.0f, 122.0f, 980.0f, 2.0f,
                                               0xFFB88A49u);
                RenderPlatform_DrawOverlayQuad(150.0f, 596.0f, 980.0f, 2.0f,
                                               0xFFB88A49u);
                RenderPlatform_DrawOverlayQuad(150.0f, 122.0f, 2.0f, 476.0f,
                                               0xFFB88A49u);
                RenderPlatform_DrawOverlayQuad(1128.0f, 122.0f, 2.0f, 476.0f,
                                               0xFFB88A49u);

                const char *title = "CANNONCRUISE";
                const RwReal titleScale = 7.0f;
                const RwReal titleWidth =
                    (RwReal)std::strlen(title) * 6.0f * titleScale;
                ShowText_DrawBitmapText(title, (kScreenW - titleWidth) * 0.5f,
                                        254.0f, (RwInt32)titleScale, 235, 211,
                                        158, 255);
                const char *subtitle = "A VOYAGE THROUGH THE NORTHERN SEAS";
                const RwReal subtitleScale = 3.0f;
                const RwReal subtitleWidth =
                    (RwReal)std::strlen(subtitle) * 6.0f * subtitleScale;
                ShowText_DrawBitmapText(
                    subtitle, (kScreenW - subtitleWidth) * 0.5f, 338.0f,
                    (RwInt32)subtitleScale, 177, 194, 193, 255);
                RenderPlatform_DrawOverlayQuad(430.0f, 394.0f, 420.0f, 1.0f,
                                               0xFF42605Du);
                const char *prompt = "ENTER / SPACE / ESC TO CONTINUE";
                const RwReal promptScale = 2.0f;
                const RwReal promptWidth =
                    (RwReal)std::strlen(prompt) * 6.0f * promptScale;
                ShowText_DrawBitmapText(
                    prompt, (kScreenW - promptWidth) * 0.5f, 524.0f,
                    (RwInt32)promptScale, 204, 190, 150, 255);
                RenderPlatform_EndOverlay2D();
                if (pShotPath != nullptr && fr == 1) {
                    RenderPlatform_RequestScreenshot(pShotPath);
                }
                RenderPlatform_EndFrame();
            }
            // advance chain: is target scene a NEXT FMV descriptor?
            // (JINGLE->MainMenu, INTRO1->Nordic01, OUTRO3->Credits chain)
            } // if (bFmvPlayed == FALSE) - title card

            std::snprintf(fmvTerminalTarget, sizeof(fmvTerminalTarget), "%s",
                          fmv.target);
            fmvNextRel[0] = 0;
            if (hasNext != 1) {
                break; // target not resolved -> chain end
            }
            std::snprintf(fmvNextRel, sizeof(fmvNextRel), "%s", nextRel);
            std::snprintf(fmvRel, sizeof(fmvRel), "%s", nextRel);
        }
        if (bQuitApp == TRUE) {
            delete pOcean;
            pOcean = nullptr;
            RwsPipeline_Unload(&pipeline);
            RenderPlatform_Terminate();
            RwEngineStop();
            RwEngineClose();
            RwEngineTerm();
            return 0;
        }

        // Chain terminal -> TARGET LEVEL (Nordic01/Nordic02/Iduna02...):
        // bind world to that level -- intro's Nordic connection is REALIZED.
        // Phase targets (MainMenu/MAPMENU) already handled by menu/map phases
        // below -> untouched. Loading pattern same as MAPMENU
        // (preserve predecessor for fallback).
        if (bQuitApp == FALSE && fmvNextRel[0] != 0 &&
            IsFmvPhaseTarget(fmvTerminalTarget) == 0) {
            char prevLevel[512] = {0};
            if (pWorldPath != nullptr) {
                std::snprintf(prevLevel, sizeof(prevLevel), "%s", pWorldPath);
            }
            RwsPipeline_Unload(&pipeline);
            bWorldLoaded = FALSE;
            RenderPlatform_LoadWorldDictionary(fmvNextRel);
            if (RwsPipeline_Load(fmvNextRel, &pipeline) == TRUE &&
                pipeline.pClump != nullptr) {
                std::snprintf(mapLevelPath, sizeof(mapLevelPath), "%s",
                              fmvNextRel);
                pWorldPath = mapLevelPath;
                bWorldLoaded = TRUE;
                std::printf("FMV: target level '%s' loaded (%s)\n",
                            fmvTerminalTarget, fmvNextRel);
            } else {
                RwsPipeline_Unload(&pipeline);
                RenderPlatform_ClearWorldDictionary();
                if (prevLevel[0] != 0) {
                    RenderPlatform_LoadWorldDictionary(prevLevel);
                    if (RwsPipeline_Load(prevLevel, &pipeline) == TRUE &&
                        pipeline.pClump != nullptr) {
                        bWorldLoaded = TRUE;
                    }
                }
                std::printf("FMV: target level failed to load ('%s')\n",
                            fmvTerminalTarget);
            }
        }

        int menuCap = -1; // -1 = unlimited (wait Enter/Esc); test env override
        const char *menuEnv = std::getenv("CANNONCRUISE_MENU_FRAMES");
        if (menuEnv != nullptr) {
            menuCap = std::atoi(menuEnv);
        }
        RwBool bMenuBackedOut = FALSE; // MAIN-Esc exit (map not opened)
        if (menuCap != 0) {
            // Relative depth centrally resolved (RwsAssets_Resolve) -> single candidate;
            // previous 3-prefix loop removes redundant repetition. Loop
            // structure kept (continue agreement if missing).
            char menuPath[512] = {0};
            RwsAssets_Resolve("Streams/MAINMENU.RWS", menuPath,
                              sizeof(menuPath));
            const char *kMenuFiles[] = {menuPath};
            for (RwUInt32 mi = 0; mi < 1; ++mi) {
                // Platform: single-path maestro load (0x0716-walk deduplicated).
                Rt2dMaestro *pMaestro =
                    Rt2dMaestro_LoadFromRwsFile(kMenuFiles[mi]);
                if (pMaestro == nullptr) {
                    continue;
                }
                // Storyboard: menu state lives in CMenuBehavior (MAINMENU/
                // OPTIONS/SOUND/PAUSE share same Menu.anm).
                CMenuBehavior menu;
                menu.AttachMaestro(pMaestro);
                pMaestro = menu.GetMaestro();
                // embedded menu textures (TexDictionary -> GL; Menu_074 ground,
                // Menu_076 logo) -- inside RWS, not external file.
                menu.LoadTextures(kMenuFiles[mi]);
                s_gameState = GAMESTATE_MAINMENU;
                audioMgr.PlayStream("MenuMus", TRUE);
                std::printf("Menu: nodes=%u commands=%d canvas=%.0fx%.0f\n",
                            pMaestro->info.nodeCount, pMaestro->commandCount,
                            (double)pMaestro->info.canvasWidth,
                            (double)pMaestro->info.canvasHeight);
                for (RwInt32 ci = 0; ci < pMaestro->commandCount; ++ci) {
                    std::printf("  [%d] %s\n", ci,
                                pMaestro->info.nodes[pMaestro->commands[ci]]
                                    .name);
                }
                // menu loop: select (W/S/up/down), confirm (Enter/Space), back
                // (Esc/Q) -- auto-confirm at frame cap (NewSave flow).
                int menuFrame = 0;
                static RwBool s_prevDown = FALSE, s_prevUp = FALSE,
                              s_prevEnter = FALSE, s_prevEsc = FALSE,
                              s_prevLeft = FALSE, s_prevRight = FALSE;
                // Phase entry: consume held keys ONCE (Enter/Space leaking
                // from intro shouldn't confirm menu unseen).
                RwPlatformPollEvents();
                s_prevDown = RwPlatformKeyDown('S') ||
                               RwPlatformKeyDown(RW_KEY_DOWN);
                s_prevUp = RwPlatformKeyDown('W') ||
                           RwPlatformKeyDown(RW_KEY_UP);
                s_prevLeft = RwPlatformKeyDown('A') ||
                             RwPlatformKeyDown(RW_KEY_LEFT);
                s_prevRight = RwPlatformKeyDown('D') ||
                              RwPlatformKeyDown(RW_KEY_RIGHT);
                s_prevEnter = RwPlatformKeyDown(' ') ||
                              RwPlatformKeyDown(RW_KEY_ENTER);
                s_prevEsc = RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                            RwPlatformKeyDown('Q');
                for (;;) {
                    ++menuFrame;
                    if (menuCap > 0 && menuFrame > menuCap) {
                        break; // auto-confirm -> go to world
                    }
                    RwPlatformPollEvents();
                    if (RwPlatformShouldClose() == TRUE) {
                        bQuitApp = TRUE;
                        break;
                    }
                    const RwBool down =
                        RwPlatformKeyDown('S') ||
                        RwPlatformKeyDown(RW_KEY_DOWN);
                    const RwBool up =
                        RwPlatformKeyDown('W') ||
                        RwPlatformKeyDown(RW_KEY_UP);
                    const RwBool left =
                        RwPlatformKeyDown('A') ||
                        RwPlatformKeyDown(RW_KEY_LEFT);
                    const RwBool right =
                        RwPlatformKeyDown('D') ||
                        RwPlatformKeyDown(RW_KEY_RIGHT);
                    const RwBool enter =
                        RwPlatformKeyDown(' ') ||
                        RwPlatformKeyDown(RW_KEY_ENTER);
                    const RwBool esc = RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                                       RwPlatformKeyDown('Q');
                    if (down == TRUE && s_prevDown == FALSE) {
                        menu.MoveNext();
                        audioMgr.PlaySound("Audio_MenuSelect");
                    }
                    if (up == TRUE && s_prevUp == FALSE) {
                        menu.MovePrev();
                        audioMgr.PlaySound("Audio_MenuSelect");
                    }
                    if (left == TRUE && s_prevLeft == FALSE) {
                        menu.AdjustLeft();
                    }
                    if (right == TRUE && s_prevRight == FALSE) {
                        menu.AdjustRight();
                    }
                    if (esc == TRUE && s_prevEsc == FALSE) {
                        s_prevDown = down; s_prevUp = up; s_prevEnter = enter;
                        s_prevEsc = esc;
                        s_prevLeft = left; s_prevRight = right;
                        if (menu.Back() == TRUE) {
                            bMenuBackedOut = TRUE;
                            break; // MAIN -> exit loop
                        }
                    }
                    if (enter == TRUE && s_prevEnter == FALSE) {
                        s_prevDown = down; s_prevUp = up; s_prevEnter = enter;
                        s_prevEsc = esc;
                        s_prevLeft = left; s_prevRight = right;
                        if (menu.Confirm() == TRUE) {
                            break; // terminal action (NEW/LOAD/QUIT/CREDITS)
                        }
                    }
                    s_prevDown = down; s_prevUp = up; s_prevEnter = enter;
                    s_prevEsc = esc;
                    s_prevLeft = left; s_prevRight = right;
                    menu.Update(1.0f / 60.0f);
                    if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
                        break;
                    }
                    if (bWorldLoaded == TRUE && pipeline.pClump != nullptr) {
                        RenderPlatform_RenderScene();
                    }
                    menu.Render();
                    if (pShotPath != nullptr && menuFrame == 2) {
                        RenderPlatform_RequestScreenshot(pShotPath);
                    }
                    RenderPlatform_EndFrame();
                }
                // maestro owned by menu (dtor destroys it)
                pMaestro = nullptr;
                // storyboard terminal action: NEW/LOAD -> continue to world,
                // QUIT -> exit, CREDITS -> log (CREDITS.RWS phase pending)
                const CMenuBehavior::EMenuAction act =
                    menu.TakePendingAction();
                if (act == CMenuBehavior::MENUACTION_QUITGAME) {
                    std::printf("Menu: action QUITGAME\n");
                    bQuitApp = TRUE;
                } else if (act == CMenuBehavior::MENUACTION_CREDITS) {
                    // Load Credits.RWS; afterwards return to MainMenu.
                    char credPath[512] = {0};
                    if (FmvScene_NextScenePath("Credits", credPath, sizeof(credPath)) == 1) {
                        std::printf("Menu: action CREDITS -> loading %s\n", credPath);
                        // Load RWS file; textures/visuals loaded via
                        // MenuTextures_LoadFromRws then return to MainMenu.
                        RwsAssets_Resolve("Streams/CREDITS.RWS", credPath,
                                          sizeof(credPath));
                        // Set game state to return to MainMenu.
                        s_gameState = GAMESTATE_CREDITS;
                    } else {
                        std::printf("Menu: action CREDITS (no path found)\n");
                    }
                } else if (act == CMenuBehavior::MENUACTION_NEWGAME) {
                    std::printf("Menu: action NEWGAME\n");
                } else if (act == CMenuBehavior::MENUACTION_LOADGAME) {
                    std::printf("Menu: action LOADGAME\n");
                }
                RenderPlatform_ClearTextures(); // free menu textures
                // 4.6r-4 -- MAPMENU phase (CAG_Menu.anm + level selection).
                // After NEW/LOAD (also at auto-cap); skipped on Esc-backout.
                const RwBool wantMap =
                    (RwBool)(act == CMenuBehavior::MENUACTION_NEWGAME ||
                             act == CMenuBehavior::MENUACTION_LOADGAME ||
                             (act == CMenuBehavior::MENUACTION_NONE &&
                              bMenuBackedOut == FALSE && menuCap > 0));
                if (wantMap == TRUE && bQuitApp == FALSE) {
                    s_gameState = GAMESTATE_MAPMENU;
                    int mapCap = -1;
                    const char *mapEnv =
                        std::getenv("CANNONCRUISE_MAPMENU_FRAMES");
                    if (mapEnv != nullptr) {
                        mapCap = std::atoi(mapEnv);
                    }
                    // Test: automatic level selection (non-interactive reload path).
                    int mapAutoSel = -1;
                    const char *mapSelEnv =
                        std::getenv("CANNONCRUISE_MAPMENU_SELECT");
                    if (mapSelEnv != nullptr) {
                        mapAutoSel = std::atoi(mapSelEnv);
                    }
                    static char mapPath[512] = {0};
                    RwsAssets_Resolve("Streams/MAPMENU.RWS", mapPath,
                                      sizeof(mapPath));
                    const char *kMapFiles[] = {mapPath};
                    for (RwUInt32 gi = 0; gi < 1; ++gi) {
                        // Platform: single-path maestro load.
                        Rt2dMaestro *pMapMaestro =
                            Rt2dMaestro_LoadFromRwsFile(kMapFiles[gi]);
                        if (pMapMaestro == nullptr) {
                            continue;
                        }
                        CAfterGameMenuBehavior mapmenu;
                        mapmenu.AttachMaestro(pMapMaestro);
                        pMapMaestro = nullptr;
                        MenuTextures_LoadFromRws(kMapFiles[gi]);
                        std::printf("MapMenu: levels=%d\n",
                                    (int)mapmenu.GetLevelCount());
                        for (RwInt32 li = 0;
                             li < mapmenu.GetLevelCount(); ++li) {
                            std::printf("  [%d] %s (%s)\n", (int)li,
                                        mapmenu.GetLevelDisplay(li),
                                        mapmenu.GetLevelFile(li));
                        }
                        int mapFrame = 0;
                        static RwBool s_prevMapDown = FALSE,
                                      s_prevMapUp = FALSE,
                                      s_prevMapEnter = FALSE,
                                      s_prevMapEsc = FALSE;
                        // Phase entry: consume held keys ONCE (Enter from menu
                        // confirm shouldn't skip past map unseen -- "direct viking
                        // 1" bug).
                        RwPlatformPollEvents();
                        s_prevMapDown = RwPlatformKeyDown('S') ||
                                        RwPlatformKeyDown(RW_KEY_DOWN);
                        s_prevMapUp = RwPlatformKeyDown('W') ||
                                      RwPlatformKeyDown(RW_KEY_UP);
                        s_prevMapEnter = RwPlatformKeyDown(' ') ||
                                         RwPlatformKeyDown(RW_KEY_ENTER);
                        s_prevMapEsc = RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                                       RwPlatformKeyDown('Q');
                        for (;;) {
                            ++mapFrame;
                            if (mapAutoSel >= 0 &&
                                mapAutoSel < mapmenu.GetLevelCount()) {
                                // automatic selection (selection starts at 0)
                                for (int k = 0; k < mapAutoSel; ++k) {
                                    mapmenu.MoveNext();
                                }
                                mapmenu.Confirm();
                                break;
                            }
                            if (mapCap > 0 && mapFrame > mapCap) {
                                mapmenu.Confirm(); // auto-confirm (first level)
                                break;
                            }
                            RwPlatformPollEvents();
                            if (RwPlatformShouldClose() == TRUE) {
                                bQuitApp = TRUE;
                                break;
                            }
                            const RwBool down =
                                RwPlatformKeyDown('S') ||
                                RwPlatformKeyDown(RW_KEY_DOWN);
                            const RwBool up =
                                RwPlatformKeyDown('W') ||
                                RwPlatformKeyDown(RW_KEY_UP);
                            const RwBool enter =
                                RwPlatformKeyDown(' ') ||
                                RwPlatformKeyDown(RW_KEY_ENTER);
                            const RwBool esc =
                                RwPlatformKeyDown(RW_KEY_ESCAPE) ||
                                RwPlatformKeyDown('Q');
                            if (down == TRUE && s_prevMapDown == FALSE) {
                                mapmenu.MoveNext();
                            }
                            if (up == TRUE && s_prevMapUp == FALSE) {
                                mapmenu.MovePrev();
                            }
                            if (esc == TRUE && s_prevMapEsc == FALSE) {
                                s_prevMapDown = down; s_prevMapUp = up;
                                s_prevMapEnter = enter; s_prevMapEsc = esc;
                                if (mapmenu.Back() == TRUE) {
                                    break; // no selection -> fallback world
                                }
                            }
                            if (enter == TRUE && s_prevMapEnter == FALSE) {
                                s_prevMapDown = down; s_prevMapUp = up;
                                s_prevMapEnter = enter; s_prevMapEsc = esc;
                                if (mapmenu.Confirm() == TRUE) {
                                    break; // MapMenuEnter
                                }
                            }
                            s_prevMapDown = down; s_prevMapUp = up;
                            s_prevMapEnter = enter; s_prevMapEsc = esc;
                            mapmenu.Update(1.0f / 60.0f);
                            if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
                                break;
                            }
                            if (bWorldLoaded == TRUE &&
                                pipeline.pClump != nullptr) {
                                RenderPlatform_RenderScene();
                            }
                            mapmenu.Render();
                            if (pMapShotPath != nullptr && mapFrame == 2) {
                                RenderPlatform_RequestScreenshot(
                                    pMapShotPath);
                            }
                            RenderPlatform_EndFrame();
                        }
                        // Selected level -> switch world pipeline
                        // (setup block runs AFTER -- single load).
                        const RwInt32 sel = mapmenu.TakeSelectedLevel();
                        if (sel >= 0 && bQuitApp == FALSE) {
                            const char *core = mapmenu.GetLevelFile(sel);
                            if (core != nullptr) {
                                char levelName[128];
                                char levelPath[512] = {0};
                                std::snprintf(levelName, sizeof(levelName),
                                              "Streams/%s.RWS", core);
                                // Relative depth centrally resolved
                                // (no manual prefix scan)
                                RwsAssets_Resolve(levelName, levelPath,
                                                  sizeof(levelPath));
                                if (levelPath[0] != 0) {
                                    // same file -> no reload
                                    const char *curBase = pWorldPath;
                                    if (curBase != nullptr) {
                                        const char *slash =
                                            std::strrchr(curBase, '/');
                                        curBase = (slash != nullptr)
                                                      ? slash + 1
                                                      : curBase;
                                    }
                                    const char *newSlash =
                                        std::strrchr(levelPath, '/');
                                    const char *newBase =
                                        (newSlash != nullptr) ? newSlash + 1
                                                              : levelPath;
                                    if (curBase == nullptr ||
                                        std::strcmp(curBase, newBase) != 0) {
                                        // 512: same size as resolved path buffer
                                        // -> no truncation.
                                        char prevPath[512] = {0};
                                        if (pWorldPath != nullptr) {
                                            std::snprintf(
                                                prevPath, sizeof(prevPath),
                                                "%s", pWorldPath);
                                        }
                                        RwsPipeline_Unload(&pipeline);
                                        bWorldLoaded = FALSE;
                                        RenderPlatform_LoadWorldDictionary(
                                            levelPath);
                                        if (RwsPipeline_Load(
                                                levelPath,
                                                &pipeline) == TRUE &&
                                            pipeline.pClump != nullptr) {
                                            std::snprintf(
                                                mapLevelPath,
                                                sizeof(mapLevelPath), "%s",
                                                levelPath);
                                            pWorldPath = mapLevelPath;
                                            bWorldLoaded = TRUE;
                                            std::printf(
                                                "MapMenu: level '%s' "
                                                "loaded\n",
                                                mapmenu.GetLevelDisplay(
                                                    sel));
                                        } else {
                                            RwsPipeline_Unload(&pipeline);
                                            RenderPlatform_ClearWorldDictionary();
                                            if (prevPath[0] != 0) {
                                                RenderPlatform_LoadWorldDictionary(
                                                    prevPath);
                                            }
                                            if (prevPath[0] != 0 &&
                                                RwsPipeline_Load(
                                                    prevPath,
                                                    &pipeline) == TRUE &&
                                                pipeline.pClump != nullptr) {
                                                bWorldLoaded = TRUE;
                                            }
                                            std::printf(
                                                "MapMenu: level load "
                                                "failed, keep '%s'\n",
                                                (pWorldPath != nullptr)
                                                    ? pWorldPath
                                                    : "(none)");
                                        }
                                    } else {
                                        std::printf(
                                            "MapMenu: level '%s' already "
                                            "loaded\n",
                                            mapmenu.GetLevelDisplay(sel));
                                    }
                                }
                            }
                        }
                        RenderPlatform_ClearTextures(); // map textures
                        break; // first valid map file
                    }
                }
                break; // first valid menu file
            }
        }
        if (bQuitApp == TRUE) {
            delete pOcean;
            pOcean = nullptr;
            RwsPipeline_Unload(&pipeline);
            RenderPlatform_Terminate();
            RwEngineStop();
            RwEngineClose();
            RwEngineTerm();
            return 0;
        }
    }

    // Phase 4.6ab -- LOADING overlay (GLOBAL art). World is loaded but not yet
    // AddClump'ed; the user is shown parchment + Hugo + "LOADING".
    // NOTE: in the librw win skeleton the window size = 640x480 (buffer matches);
    // draw space is also given as 640x480 (FMV/menu render at 1280x720 and get
    // cropped -- would break).
    {
        const char *loadEnv = std::getenv("CANNONCRUISE_LOADING_FRAMES");
        const int loadFrames =
            (loadEnv != nullptr) ? std::atoi(loadEnv) : 100;
        if (loadFrames > 0) {
            if (LoadingScreen_Play(loadFrames, 640, 480) == FALSE) {
                delete pOcean;
                pOcean = nullptr;
                RwsPipeline_Unload(&pipeline);
                RenderPlatform_Terminate();
                RwEngineStop();
                RwEngineClose();
                RwEngineTerm();
                return 0;
            }
        }
    }

    // World setup (after boot-flow -- MAPMENU selection set up with single
    // load). 4.6q camera + 4.6p spawn/ocean carried over.
    // 4.6q: camera state lives throughout loop (freecam WASD/QE + mouse look).
    RwReal camPos[3] = {0.0f, 180.0f, 420.0f};
    RwReal camLook[3] = {0.0f, 0.0f, 0.0f};
    RwReal camYaw = 0.0f;   // rad -- mouse/arrow key pitch/yaw
    RwReal camPitch = 0.0f;
    RwReal camSpan = 400.0f; // world scale -- freecam speed base
    // Sized for the real Hugo_Ship (about 4.6 m long, 5.8 m across the wings).
    const RwReal kFollowCameraDistance = 10.0f;
    const RwReal kFollowCameraHeight = 4.5f;
    if (bWorldLoaded == TRUE && pipeline.pClump != nullptr) {
        const char *bgmTrack = "BeaverM";
        if (pWorldPath != nullptr) {
            if (std::strstr(pWorldPath, "VIKING") || std::strstr(pWorldPath, "Viking")) bgmTrack = "VikingM";
            else if (std::strstr(pWorldPath, "NORDIC") || std::strstr(pWorldPath, "Nordic")) bgmTrack = "NordicM";
            else if (std::strstr(pWorldPath, "IDUNA") || std::strstr(pWorldPath, "Iduna")) bgmTrack = "IdunaM";
            else if (std::strstr(pWorldPath, "BEAVER") || std::strstr(pWorldPath, "Beaver")) bgmTrack = "BeaverM";
        }
        audioMgr.PlayStream(bgmTrack, TRUE);
        speakMgr.PlaySpeak(1, 0); // Introductory speech line

        if (RenderPlatform_AddClump(pipeline.pClump) != TRUE) {
            std::fprintf(stderr, "CannonCruise: AddClump failed\n");
        }
        // Phase 4.6p -- spawn chain: 0x0704 registrations -> CClassFactory
        // create + apply (known classes; unknown silently skipped).
        if (GameBehaviours_Register() == TRUE) {
            WorldSpawnStats spawnStats;
            const RwInt32 spawned =
                WorldSpawn_FromFile(pWorldPath, &spawnStats);
            std::printf("WorldSpawn: records=%u created=%u skipped=%u "
                        "applied=%d\n",
                        spawnStats.objectRecords, spawnStats.instancesCreated,
                        spawnStats.instancesSkipped, (int)spawned);
        }
        // Phase 4.6q -- camera auto-framing: overview from world bbox
        // (0x0704 camera data is a chase-rig -- without follow target,
        // not world coordinates; CANNONCRUISE_CAM=DATA for data camera).
        const char *camMode = std::getenv("CANNONCRUISE_CAM");
        if (camMode != nullptr && std::strcmp(camMode, "DATA") == 0) {
            if (RwsCamera_LoadPose((pWorldPath != nullptr) ? pWorldPath : "",
                                   camPos, camLook) == TRUE) {
                std::printf("CannonCruise: camera DATA pos=(%.2f, %.2f, "
                            "%.2f) look=(%.2f, %.2f, %.2f)\n",
                            (double)camPos[0], (double)camPos[1],
                            (double)camPos[2], (double)camLook[0],
                            (double)camLook[1], (double)camLook[2]);
            }
        } else {
            WorldGeometryBounds bb;
            if (WorldGeometry_GetLastBounds(&bb) == TRUE) {
                const RwReal cx = (bb.min[0] + bb.max[0]) * 0.5f;
                const RwReal cy = (bb.min[1] + bb.max[1]) * 0.5f;
                const RwReal cz = (bb.min[2] + bb.max[2]) * 0.5f;
                const RwReal dx = bb.max[0] - bb.min[0];
                const RwReal dz = bb.max[2] - bb.min[2];
                const RwReal span = (dx > dz) ? dx : dz;
                // look at center (known-good direction), distance span*0.62:
                // island dominates view -- ground lookAt was empty frame
                // 4.6q rev: lower+close (island was thin sliver at horizon)
                camPos[0] = cx;
                camPos[1] = cy + span * 0.14f;
                camPos[2] = cz + span * 0.38f;
                camLook[0] = cx;
                camLook[1] = cy;
                camLook[2] = cz;
                camSpan = span; // freecam speed base
                // Diagnostics: CANNONCRUISE_LOOKAT=x,y,z[,dist] -> override
                {
                    const char *lookEnv =
                        std::getenv("CANNONCRUISE_LOOKAT");
                    if (lookEnv != nullptr) {
                        float lx = cx, ly = cy, lz = cz, dd = -1.0f;
                        if (std::sscanf(lookEnv, "%f,%f,%f,%f", &lx, &ly,
                                        &lz, &dd) >= 3) {
                            camLook[0] = lx;
                            camLook[1] = ly;
                            camLook[2] = lz;
                            if (dd > 0.0f) {
                                RwReal f[3] = {lx - camPos[0], ly - camPos[1],
                                               lz - camPos[2]};
                                RwReal fl = std::sqrt(f[0] * f[0] +
                                                      f[1] * f[1] +
                                                      f[2] * f[2]);
                                if (fl > 1e-4f) {
                                    camPos[0] = lx - f[0] / fl * dd;
                                    camPos[1] = ly - f[1] / fl * dd;
                                    camPos[2] = lz - f[2] / fl * dd;
                                }
                            }
                        }
                    }
                }
                std::printf("CannonCruise: camera AUTO bbox "
                            "(%.0f..%.0f, %.0f..%.0f, %.0f..%.0f) "
                            "pos=(%.1f, %.1f, %.1f)\n",
                            (double)bb.min[0], (double)bb.max[0],
                            (double)bb.min[1], (double)bb.max[1],
                            (double)bb.min[2], (double)bb.max[2],
                            (double)camPos[0], (double)camPos[1],
                            (double)camPos[2]);
            }
        }
        ClampCameraToMap(camPos);
        RenderPlatform_SetCameraPose(camPos, camLook);
        // mouse look init: yaw/pitch derived from (look-pos) direction
        {
            RwReal f[3] = {camLook[0] - camPos[0], camLook[1] - camPos[1],
                           camLook[2] - camPos[2]};
            const RwReal len = std::sqrt(f[0] * f[0] + f[1] * f[1] +
                                         f[2] * f[2]);
            if (len > 1e-4f) {
                camYaw = std::atan2(f[0] / len, f[2] / len);
                camPitch = std::asin(f[1] / len);
            }
        }
        RwPlatformSetMouseCapture(TRUE); // cursor hidden -- mouse look
        // Phase 4.6p (wire-in) -- ocean: 5x5 tile grid covering bbox XZ
        // (tile = span/2 -> total span*2.5; y=0 water level, VIKING min.y=-9).
        {
            WorldGeometryBounds bb;
            if (WorldGeometry_GetLastBounds(&bb) == TRUE) {
                const RwReal cx = (bb.min[0] + bb.max[0]) * 0.5f;
                const RwReal cz = (bb.min[2] + bb.max[2]) * 0.5f;
                const RwReal dx = bb.max[0] - bb.min[0];
                const RwReal dz = bb.max[2] - bb.min[2];
                const RwReal span = (dx > dz) ? dx : dz;
                const RwReal tileSize = span * 0.5f;
                pOcean = new COcean;
                pOcean->Initialize(5, 5, tileSize,
                                   cx - 2.5f * tileSize,
                                   cz - 2.5f * tileSize);
                std::printf("CannonCruise: ocean 5x5 tiles tileSize=%.0f "
                            "origin=(%.0f, %.0f)\n",
                            (double)tileSize,
                            (double)(cx - 2.5f * tileSize),
                            (double)(cz - 2.5f * tileSize));
                // Camera bounds: XZ edge = ocean grid (inner margin),
                // Y = sea surface..island top + margin.
                {
                    const RwReal half = span * 1.25f - 2.0f;
                    s_camBoundMin[0] = cx - half;
                    s_camBoundMax[0] = cx + half;
                    s_camBoundMin[1] = 1.5f;
                    s_camBoundMax[1] = bb.max[1] + 40.0f;
                    s_camBoundMin[2] = cz - half;
                    s_camBoundMax[2] = cz + half;
                    s_camBoundsSet = TRUE;
                    if (PlayerShip_SetMapBounds(
                            s_camBoundMin[0], s_camBoundMax[0],
                            s_camBoundMin[2], s_camBoundMax[2]) != TRUE) {
                        std::fprintf(stderr,
                                     "CannonCruise: invalid ocean bounds for "
                                     "player ship\n");
                    }
                }
            }
        }
        // Phase 4.6s -- ship follow: placeholder boat at sea, facing the
        // island centre. Override with CANNONCRUISE_SHIP=x,y,z,yaw.
        {
            WorldGeometryBounds bb;
            if (WorldGeometry_GetLastBounds(&bb) == TRUE) {
                const RwReal cx = (bb.min[0] + bb.max[0]) * 0.5f;
                const RwReal cz = (bb.min[2] + bb.max[2]) * 0.5f;
                const RwReal dx = bb.max[0] - bb.min[0];
                const RwReal dz = bb.max[2] - bb.min[2];
                const RwReal span = (dx > dz) ? dx : dz;
                RwV3d shipPos = {cx + span * 0.04f, 0.0f, cz + span * 0.42f};
                RwReal shipYaw = std::atan2(cx - shipPos.x, cz - shipPos.z);
                // Original start transform from the level's CShipPlayer
                // record (CSystemCommands matrix): position + heading (at).
                const WorldSpawnPlayerShip *pDataShip = WorldSpawn_GetPlayerShip();
                if (pDataShip != nullptr && pDataShip->valid == TRUE) {
                    shipPos.x = pDataShip->matrix[12];
                    shipPos.y = 0.0f;
                    shipPos.z = pDataShip->matrix[14];
                    shipYaw = std::atan2(pDataShip->matrix[8],
                                         pDataShip->matrix[10]);
                    std::printf("CannonCruise: player start from level data "
                                "(%.1f, %.1f) yaw=%.2f\n",
                                (double)shipPos.x, (double)shipPos.z,
                                (double)shipYaw);
                }
                const char *shipEnv = std::getenv("CANNONCRUISE_SHIP");
                if (shipEnv != nullptr) {
                    float sx = 0.0f, sy = 0.0f, sz = 0.0f, syaw = 0.0f;
                    if (std::sscanf(shipEnv, "%f,%f,%f,%f", &sx, &sy, &sz,
                                    &syaw) >= 3) {
                        shipPos.x = sx;
                        shipPos.y = sy;
                        shipPos.z = sz;
                        shipYaw = syaw;
                    }
                }
                PlayerShip_Spawn(&shipPos, shipYaw);
                // Real ship model (Hugo_Ship) from the level; the hand-built
                // placeholder is only drawn when the level has none.
                if (pDataShip != nullptr && pDataShip->pModel != nullptr &&
                    s_pShipClump == nullptr) {
                    const RwsResource *pRes =
                        static_cast<const RwsResource *>(pDataShip->pModel);
                    s_pShipClump = RpClumpStreamReadMemory(pRes->pData, pRes->size);
                    // Not added to the world: drawn explicitly after the
                    // ocean each frame (RenderPlatform_RenderClump).
                    if (s_pShipClump != nullptr) {
                        std::printf("CannonCruise: player ship model %s "
                                    "(%d atomics)\n", pRes->name,
                                    (int)RpClumpPortCountAtomics(s_pShipClump));
                        s_pShipAnimator = RpClumpPortAnimatorCreate(s_pShipClump);
                        for (RwUInt32 ci = 0;
                             s_pShipAnimator != nullptr && ci < pDataShip->clipCount; ++ci) {
                            const RwsResource *pAnim = static_cast<const RwsResource *>(
                                pDataShip->clips[ci].pAnim);
                            const RwInt32 node = pDataShip->clips[ci].nodeId;
                            const RwInt32 idx = RpClumpPortAnimatorAddClip(
                                s_pShipAnimator, pAnim->pData, pAnim->size, node);
                            // Node tags of Hugo_Ship: 100/200 wings, 300 wheel,
                            // 400 rudder, 500 gun base (+600 barrel).
                            if (node == 100) s_shipClipWingL = idx;
                            else if (node == 200) s_shipClipWingR = idx;
                            else if (node == 300) s_shipClipWheel = idx;
                            else if (node == 400) s_shipClipRudder = idx;
                            else if (node == 500) s_shipClipGun = idx;
                            std::printf("  clip %s -> node %d: %s\n", pAnim->name,
                                        (int)node, idx >= 0 ? "ok" : "FAILED");
                        }
                        RwV3d bmin, bmax;
                        if (RpClumpPortGetLocalBounds(s_pShipClump, &bmin, &bmax) == TRUE) {
                            std::printf("  ship bounds x[%.2f..%.2f] y[%.2f..%.2f] z[%.2f..%.2f]\n",
                                        bmin.x, bmax.x, bmin.y, bmax.y, bmin.z, bmax.z);
                        }
                    }
                }
                PlayerShipState spawnedShip;
                if (PlayerShip_GetState(&spawnedShip) == TRUE) {
                    shipPos = spawnedShip.pos;
                    shipYaw = spawnedShip.yaw;
                }
                RWS::HUD_Initialize();
                if (camMode == nullptr || std::strcmp(camMode, "FOLLOW") == 0) {
                    const RwReal ang = shipYaw + 3.14159265f;
                    camPos[0] =
                        shipPos.x + std::sin(ang) * kFollowCameraDistance;
                    camPos[1] = shipPos.y + kFollowCameraHeight;
                    camPos[2] =
                        shipPos.z + std::cos(ang) * kFollowCameraDistance;
                    camLook[0] =
                        shipPos.x + std::sin(shipYaw) * 2.0f;
                    camLook[1] = shipPos.y + 1.5f;
                    camLook[2] =
                        shipPos.z + std::cos(shipYaw) * 2.0f;
                    ClampCameraToMap(camPos);
                    RenderPlatform_SetCameraPose(camPos, camLook);
                }
                std::printf("CannonCruise: player ship placeholder spawn "
                            "(%.1f, %.1f, %.1f) yaw=%.2f\n",
                            (double)shipPos.x, (double)shipPos.y,
                            (double)shipPos.z, (double)shipYaw);
                // Towers: placed from the level's CTowerCatapult records
                // (CSystemCommands matrix). Towers carrying a mission death
                // event (id3 "MissionTowerNDead") are the mission targets; if a
                // level has towers but none carry an event, all of them count.
                const WorldSpawnTower *pDataTowers = nullptr;
                const RwUInt32 dataTowerCount = WorldSpawn_GetTowers(&pDataTowers);
                RwInt32 missionTowerCount = 0;
                for (RwUInt32 ti = 0; ti < dataTowerCount; ++ti) {
                    if (pDataTowers[ti].missionEvent[0] != '\0') {
                        ++missionTowerCount;
                    }
                }
                for (RwUInt32 ti = 0; ti < dataTowerCount; ++ti) {
                    const WorldSpawnTower &dt = pDataTowers[ti];
                    CTowerCatapult *pT = new CTowerCatapult();
                    pT->SetWorldPosition(dt.pos);
                    s_towers.push_back(pT);
                    // Visible tower model at the record's transform.
                    RpClump *pTowerClump = nullptr;
                    if (dt.pModel != nullptr) {
                        const RwsResource *pRes =
                            static_cast<const RwsResource *>(dt.pModel);
                        pTowerClump = RpClumpStreamReadMemory(pRes->pData,
                                                              pRes->size);
                        if (pTowerClump != nullptr) {
                            RpClumpPortSetMatrix(pTowerClump, dt.matrix);
                            RenderPlatform_AddClump(pTowerClump);
                        }
                    }
                    s_towerClumps.push_back(pTowerClump);
                    s_towerDamaged.push_back(dt.pDamaged);
                    s_towerMatrices.push_back(std::vector<RwReal>(dt.matrix, dt.matrix + 16));
                    const RwBool bMission =
                        (missionTowerCount == 0 || dt.missionEvent[0] != '\0')
                            ? TRUE : FALSE;
                    s_towerIsMission.push_back(bMission);
                    std::printf("CannonCruise: tower #%u at (%.1f,%.1f,%.1f)%s %s\n",
                                ti, dt.pos.x, dt.pos.y, dt.pos.z,
                                bMission == TRUE ? " [mission]" : "",
                                dt.missionEvent);
                }
                if (dataTowerCount > 0) {
                    RWS::HUD_SetTowersTotal(missionTowerCount > 0
                                                ? missionTowerCount
                                                : (RwInt32)dataTowerCount);
                }
                // Enemy ships (CShipEnemy records): their own model at the
                // record's transform. They are drawn after the ocean like the
                // player ship and ride the waves; AI/sailing is not ported yet.
                {
                    const WorldSpawnEnemyShip *pEnemies = nullptr;
                    const RwUInt32 enemyCount = WorldSpawn_GetEnemyShips(&pEnemies);
                    RwUInt32 shown = 0, routed = 0;
                    for (RwUInt32 ei = 0; ei < enemyCount; ++ei) {
                        const RwsResource *pRes =
                            static_cast<const RwsResource *>(pEnemies[ei].pModel);
                        if (pRes == nullptr) {
                            continue;
                        }
                        RpClump *pEnemy = RpClumpStreamReadMemory(pRes->pData, pRes->size);
                        if (pEnemy == nullptr) {
                            continue;
                        }
                        const WorldSpawnEnemyShip &es = pEnemies[ei];
                        RpClumpPortSetMatrix(pEnemy, es.matrix);
                        EnemyShipRuntime er;
                        std::memset(&er, 0, sizeof(er));
                        er.clump = pEnemy;
                        er.clipPaddleL = er.clipPaddleR = er.clipRudder = -1;
                        er.scale = std::sqrt(es.matrix[0] * es.matrix[0] +
                                             es.matrix[1] * es.matrix[1] +
                                             es.matrix[2] * es.matrix[2]);
                        if (er.scale <= 0.0f) er.scale = 1.0f;
                        er.x = er.homeX = es.matrix[12];
                        er.y = es.matrix[13];
                        er.z = er.homeZ = es.matrix[14];
                        // Forward = matrix "at" row (same convention as the player).
                        er.yaw = std::atan2(es.matrix[8], es.matrix[10]);
                        er.hasWaypoint = es.hasWaypoint;
                        er.wpX = es.waypoint[0];
                        er.wpZ = es.waypoint[2];
                        er.toWaypoint = TRUE;
                        er.hp = 150.0f; // two 100-damage hits, like the towers
                        std::snprintf(er.spawnEvent, sizeof(er.spawnEvent), "%s", es.spawnEvent);
                        std::snprintf(er.deathEvent, sizeof(er.deathEvent), "%s", es.deathEvent);
                        LevelScript_RegisterProducer(es.deathEvent);
                        if (es.clipCount > 0) {
                            er.animator = RpClumpPortAnimatorCreate(pEnemy);
                        }
                        for (RwUInt32 ci = 0; er.animator != nullptr && ci < es.clipCount; ++ci) {
                            const RwsResource *pAnim =
                                static_cast<const RwsResource *>(es.clips[ci].pAnim);
                            const RwInt32 idx = RpClumpPortAnimatorAddClip(
                                er.animator, pAnim->pData, pAnim->size, es.clips[ci].nodeId);
                            if (std::strstr(pAnim->name, "Paddle_Left")) er.clipPaddleL = idx;
                            else if (std::strstr(pAnim->name, "Paddle_Right")) er.clipPaddleR = idx;
                            else if (std::strstr(pAnim->name, "Rudder")) er.clipRudder = idx;
                        }
                        s_enemies.push_back(er);
                        if (es.hasWaypoint == TRUE) ++routed;
                        ++shown;
                    }
                    std::printf("CannonCruise: %u/%u enemy ships with models, "
                                "%u on waypoint routes\n",
                                (unsigned)shown, (unsigned)enemyCount, (unsigned)routed);
                }
                // Fish (CFishGreen): appear on their spawn event, collected
                // by sailing into them or shooting next to them.
                {
                    const WorldSpawnFish *pFish = nullptr;
                    const RwUInt32 fishCount = WorldSpawn_GetFish(&pFish);
                    for (RwUInt32 fi = 0; fi < fishCount; ++fi) {
                        FishRuntime fr;
                        std::memset(&fr, 0, sizeof(fr));
                        const RwsResource *pRes = static_cast<const RwsResource *>(pFish[fi].pModel);
                        if (pRes != nullptr) {
                            fr.clump = RpClumpStreamReadMemory(pRes->pData, pRes->size);
                        }
                        fr.x = pFish[fi].pos[0];
                        fr.z = pFish[fi].pos[2];
                        std::snprintf(fr.spawnEvent, sizeof(fr.spawnEvent), "%s", pFish[fi].spawnEvent);
                        std::snprintf(fr.pickupEvent, sizeof(fr.pickupEvent), "%s", pFish[fi].pickupEvent);
                        LevelScript_RegisterProducer(fr.pickupEvent);
                        s_fishes.push_back(fr);
                    }
                    std::printf("CannonCruise: %u fish, %u level script records%s\n",
                                (unsigned)fishCount, (unsigned)LevelScript_RecordCount(),
                                LevelScript_HasLevelExit() == TRUE ? " (level exit)" : "");
                    LevelScript_DumpTriggers();
                    LevelScript_Start();
                }
                // Fallback for levels without tower records (IDUNA/NORDIC
                // missions are not tower-based): keep the two placeholder
                // towers derived from the world bounds so the level stays
                // playable until those mission types are ported.
                // Levels whose script has its own exit (NORDIC tutorial and
                // destroyer missions) need no placeholder towers.
                if (dataTowerCount == 0 && LevelScript_HasLevelExit() == TRUE) {
                    RWS::HUD_SetTowersTotal(0);
                } else if (dataTowerCount == 0) {
                    WorldGeometryBounds bb2;
                    if (WorldGeometry_GetLastBounds(&bb2) == TRUE) {
                        const RwReal tcx = (bb2.min[0] + bb2.max[0]) * 0.5f;
                        const RwReal tcz = (bb2.min[2] + bb2.max[2]) * 0.5f;
                        const RwReal span = (bb2.max[0] - bb2.min[0]) > (bb2.max[2] - bb2.min[2])
                            ? (bb2.max[0] - bb2.min[0])
                            : (bb2.max[2] - bb2.min[2]);
                        RwV3d t1 = {tcx + span * 0.18f, 0.0f, tcz + span * 0.10f};
                        RwV3d t2 = {tcx - span * 0.18f, 0.0f, tcz + span * 0.05f};
                        CTowerCatapult *pT1 = new CTowerCatapult();
                        CTowerCatapult *pT2 = new CTowerCatapult();
                        pT1->SetWorldPosition(t1);
                        pT2->SetWorldPosition(t2);
                        s_towers.push_back(pT1);
                        s_towers.push_back(pT2);
                        s_towerIsMission.push_back(TRUE);
                        s_towerIsMission.push_back(TRUE);
                        for (int k = 0; k < 2; ++k) {
                            s_towerClumps.push_back(nullptr);
                            s_towerDamaged.push_back(nullptr);
                            s_towerMatrices.push_back(std::vector<RwReal>());
                        }
                        RWS::HUD_SetTowersTotal(2);
                        std::printf("CannonCruise: no tower records; spawned 2 placeholder towers at "
                                    "(%.1f,%.1f,%.1f) and (%.1f,%.1f,%.1f)\n",
                                    t1.x, t1.y, t1.z, t2.x, t2.y, t2.z);
                    }
                }
            }
        }
    }
    // Geometry/material diagnostics (root cause of colorlessness).
    if (std::getenv("CANNONCRUISE_GEODIAG") != nullptr) {
        WorldGeometry_Diagnose(pipeline.pClump, 400);
    }

    // Frame loop -- if world loaded, run until window close (unlimited;
    // CANNONCRUISE_FRAMES env for test cap, e.g. 300), else 3 frames

    // platform smoke.
    int frameCap = -1; // -1 = unlimited
    const char *capEnv = std::getenv("CANNONCRUISE_FRAMES");
    if (capEnv != nullptr) {
        frameCap = std::atoi(capEnv);
    }
    const int kFrames = (bWorldLoaded == TRUE) ? frameCap : 3;
    int framesRendered = 0;
    s_gameState = GAMESTATE_GAMEPLAY; // video cutscene: NORMAL camera mode
    for (int frame = 0; kFrames < 0 || frame < kFrames; ++frame) {
        if (bQuitApp == TRUE) {
            std::printf("Pause: quit requested; leaving game loop\n");
            break;
        }
        RwPlatformPollEvents(); // GLFW event pump (keys + mouse delta)
        if (RwPlatformShouldClose() == TRUE) {
            break;
        }
        if (RenderPlatform_BeginFrame(nullptr) != TRUE) {
            std::fprintf(stderr, "CannonCruise: BeginFrame %d failed\n",
                         frame);
            break;
        }
        if (bWorldLoaded == TRUE && pipeline.pClump != nullptr) {
            // ESC toggles PAUSE (edge-triggered). Paused = world frozen,
            // overlay menu on screen; ESC/Enter-on-RESUME resumes.
            static RwBool s_escPrev = FALSE;
            static RwInt32 s_pauseSelection = 0; // 0=RESUME, 1=QUIT TO DESKTOP
            const RwBool escDown =
                RwPlatformKeyDown(RW_KEY_ESCAPE) ? TRUE : FALSE;
            const char *pauseAtEnv = std::getenv("CANNONCRUISE_PAUSE_AT");
            if (pauseAtEnv != nullptr && frame == std::atoi(pauseAtEnv) &&
                s_gameState == GAMESTATE_GAMEPLAY) {
                s_gameState = GAMESTATE_PAUSE;
                s_pauseSelection = 0;
                std::printf("Pause: entered (CANNONCRUISE_PAUSE_AT=%d)\n",
                            frame);
            }
            if (escDown == TRUE && s_escPrev == FALSE) {
                if (s_gameState == GAMESTATE_GAMEPLAY) {
                    s_gameState = GAMESTATE_PAUSE;
                    s_pauseSelection = 0;
                    std::printf("Pause: entered\n");
                } else if (s_gameState == GAMESTATE_PAUSE) {
                    s_gameState = GAMESTATE_GAMEPLAY;
                    std::printf("Pause: resumed (ESC)\n");
                }
            }
            s_escPrev = escDown;
            // Save/Load debug controls (F5=Save, F6=Load)
            static RwBool s_f5Prev = FALSE;
            static RwBool s_f6Prev = FALSE;
            const RwBool f5Down = RwPlatformKeyDown(RW_KEY_F5) ? TRUE : FALSE;
            const RwBool f6Down = RwPlatformKeyDown(RW_KEY_F6) ? TRUE : FALSE;
            const RwBool f5Pressed = (f5Down == TRUE && s_f5Prev == FALSE) ? TRUE : FALSE;
            const RwBool f6Pressed = (f6Down == TRUE && s_f6Prev == FALSE) ? TRUE : FALSE;
            s_f5Prev = f5Down;
            s_f6Prev = f6Down;
            // F5 = Save  (writes .\\CannonCruise.sav)
            if (f5Pressed == TRUE) {
                FILE* pFile = fopen(".\\CannonCruise.sav", "wb");
                if (pFile) {
                    // Write SSaveData: LevelUnlocked, HighScore, Coins, Checksum
                    RwUInt32 levelUnlocked = 1;       // bit 0 = VIKING01 unlocked, etc.
                    RwUInt32 highScore = static_cast<RwUInt32>(RWS::HUD_GetScore());
                    RwUInt32 coins = 0;              // TODO: track coins
                    RwUInt32 checksum = levelUnlocked ^ highScore ^ coins ^ 0x53415645u;
                    SSaveGameData saveData = { levelUnlocked, highScore, coins, checksum };
                    fwrite(&saveData, sizeof(SSaveGameData), 1, pFile);
                    fclose(pFile);
                    std::printf("Save: game saved to .\\CannonCruise.sav\n");
                } else {
                    std::printf("Save: failed to open file\n");
                }
            }
            // F6 = Load
            if (f6Pressed == TRUE) {
                FILE* pFile = fopen(".\\CannonCruise.sav", "rb");
                if (pFile) {
                    SSaveGameData saveData;
                    if (fread(&saveData, sizeof(SSaveGameData), 1, pFile) == 1) {
                        // Validate checksum
                        RwUInt32 expectedChecksum = saveData.m_uLevelUnlocked ^ saveData.m_uHighScore ^ saveData.m_uCoins ^ 0x53415645u;
                        if (saveData.m_uChecksum == expectedChecksum) {
                            // Apply saved state
                            // Level unlocked: set bitflags for levels
                            std::printf("Load: game loaded from .\\CannonCruise.sav\n"
                                        "  LevelUnlocked=%u  HighScore=%u  Coins=%u\n",
                                        saveData.m_uLevelUnlocked,
                                        saveData.m_uHighScore,
                                        saveData.m_uCoins);
                        } else {
                            std::printf("Load: invalid checksum - using defaults\n");
                        }
                    } else {
                        std::printf("Load: failed to read file\n");
                    }
                    fclose(pFile);
                } else {
                    std::printf("Load: no save file found\n");
                }
            }
            if (s_gameState == GAMESTATE_PAUSE) {
                // W/S or Up/Down to navigate, Enter to confirm.
                static RwBool s_upPrev = FALSE, s_downPrev = FALSE,
                              s_enterPrev = FALSE;
                const RwBool up = RwPlatformKeyDown('W') ||
                                  RwPlatformKeyDown(RW_KEY_UP);
                const RwBool down = RwPlatformKeyDown('S') ||
                                    RwPlatformKeyDown(RW_KEY_DOWN);
                const RwBool enter = RwPlatformKeyDown(RW_KEY_ENTER) ||
                                     RwPlatformKeyDown(' ');
                if (down == TRUE && s_downPrev == FALSE) {
                    s_pauseSelection = (s_pauseSelection + 1) % 2;
                }
                if (up == TRUE && s_upPrev == FALSE) {
                    s_pauseSelection = (s_pauseSelection + 1) % 2;
                }
                if (enter == TRUE && s_enterPrev == FALSE) {
                    if (s_pauseSelection == 0) {
                        s_gameState = GAMESTATE_GAMEPLAY;
                        std::printf("Pause: resumed (RESUME)\n");
                    } else {
                        std::printf("Pause: QUIT TO DESKTOP selected\n");
                        bQuitApp = TRUE;
                    }
                }
                s_upPrev = up;
                s_downPrev = down;
                s_enterPrev = enter;
            }
            if (s_gameState == GAMESTATE_LEVELCOMPLETE) {
                // Starts TRUE so a key still held when the level ends (e.g.
                // the fire key) does not dismiss the summary immediately.
                static RwBool s_lcPrev = TRUE;
                const RwBool confirm = RwPlatformKeyDown(RW_KEY_ENTER) ||
                                       RwPlatformKeyDown(' ');
                if (confirm == TRUE && s_lcPrev == FALSE) {
                    std::printf("LevelComplete: QUIT TO DESKTOP selected\n");
                    bQuitApp = TRUE;
                }
                s_lcPrev = confirm;
            }
            // World is frozen while paused and on the summary screen.
            const RwBool bSimActive =
                (s_gameState == GAMESTATE_PAUSE ||
                 s_gameState == GAMESTATE_LEVELCOMPLETE) ? FALSE : TRUE;
            // The port advances Bullet by a fixed 1/60 s per active frame.
            // This does not enforce 60 FPS or establish original cadence parity.
            {
                CPhysicsWorld *pWorld = CPhysicsWorld::GetSingletonPtr();
                if (pWorld != nullptr && bSimActive == TRUE) {
                    pWorld->Update(1.0f / 60.0f);
                }
            }
            if (std::getenv("CANNONCRUISE_NO_WORLD") == nullptr) {
                RenderPlatform_RenderScene();
            }
            // Sprint 1 -- player fires cannon with ' '; ship sails with arrows;
            // cannonball is simulated with Bullet. A tower hit takes damage.
            // CANNONCRUISE_AUTOFIRE=N fires every N frames (headless hit tests).
            static const char *s_autoFireEnv = std::getenv("CANNONCRUISE_AUTOFIRE");
            const int autoFireEvery = s_autoFireEnv ? std::atoi(s_autoFireEnv) : 0;
            const RwBool bAutoFire =
                (autoFireEvery > 0 && frame % autoFireEvery == 0) ? TRUE : FALSE;
            if (bSimActive == TRUE && (RwPlatformKeyDown(' ') || bAutoFire == TRUE)) {
                if (RWS::HUD_GetAmmo() <= 0) {
                    RWS::HUD_FlashNoAmmo();
                } else {
                PlayerShipState ss;
                if (PlayerShip_GetState(&ss) == TRUE) {
                    RwReal aimYaw = ss.yaw;
                    RwReal aimDist = 0.0f;
                    // CANNONCRUISE_AUTOAIM=1 (test only): aim at the nearest
                    // alive mission tower instead of the bow direction.
                    // =ENEMY aims at the nearest afloat enemy ship (with lead).
                    static const char *s_autoAimEnv = std::getenv("CANNONCRUISE_AUTOAIM");
                    static const RwBool s_autoAim = s_autoAimEnv != nullptr ? TRUE : FALSE;
                    static const RwBool s_autoAimEnemy =
                        (s_autoAimEnv != nullptr && std::strcmp(s_autoAimEnv, "ENEMY") == 0)
                            ? TRUE : FALSE;
                    RwReal aimDh = -0.5f; // target centre height - muzzle height
                    if (s_autoAimEnemy == TRUE) {
                        RwReal bestD2 = -1.0f;
                        for (size_t ei = 0; ei < s_enemies.size(); ++ei) {
                            const EnemyShipRuntime &e = s_enemies[ei];
                            if (enemyPresent(e) != TRUE || e.sinkTime > 0.0f) continue;
                            RwReal tx = e.x, tz = e.z;
                            RwReal ddx = tx - ss.pos.x, ddz = tz - ss.pos.z;
                            if (e.hasWaypoint == TRUE) {
                                const RwReal lead = std::sqrt(ddx * ddx + ddz * ddz) / 55.0f * 5.0f;
                                tx += std::sin(e.yaw) * lead;
                                tz += std::cos(e.yaw) * lead;
                                ddx = tx - ss.pos.x;
                                ddz = tz - ss.pos.z;
                            }
                            const RwReal d2 = ddx * ddx + ddz * ddz;
                            if (bestD2 < 0.0f || d2 < bestD2) {
                                bestD2 = d2;
                                aimYaw = std::atan2(ddx, ddz);
                                aimDist = std::sqrt(d2);
                                aimDh = (e.y + 1.5f) - (ss.pos.y + 2.5f);
                            }
                        }
                    } else if (s_autoAim == TRUE) {
                        RwReal bestD2 = -1.0f;
                        for (size_t ti = 0; ti < s_towers.size(); ++ti) {
                            const RwBool bMission = (ti < s_towerIsMission.size())
                                                        ? s_towerIsMission[ti] : TRUE;
                            if (bMission != TRUE || s_towers[ti]->IsDestroyed()) {
                                continue;
                            }
                            const RwV3d &tp = s_towers[ti]->GetWorldPosition();
                            const RwReal ddx = tp.x - ss.pos.x, ddz = tp.z - ss.pos.z;
                            const RwReal d2 = ddx * ddx + ddz * ddz;
                            if (bestD2 < 0.0f || d2 < bestD2) {
                                bestD2 = d2;
                                aimYaw = std::atan2(ddx, ddz);
                                aimDist = std::sqrt(d2);
                            }
                        }
                    }
                    // Elevation as tan(angle): flat shot (~3.4 deg) so the ball
                    // stays at tower height (towers are ~8 m tall) out to
                    // ~55 m; autoaim solves for the target distance.
                    RwReal aimTan = 0.06f;
                    if (s_autoAim == TRUE && aimDist > 0.0f) {
                        const RwReal kG = 9.8f, kV = 55.0f;
                        aimTan = kG * aimDist / (2.0f * kV * kV) + aimDh / aimDist;
                        if (aimTan < 0.0f) aimTan = 0.0f;
                    }
                    RwV3d fwd = {std::sin(aimYaw), aimTan, std::cos(aimYaw)};
                    RwReal len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
                    if (len > 0.0f) {
                        fwd.x /= len; fwd.y /= len; fwd.z /= len;
                    }
                    // Spawn ahead of the bow: the kinematic hull reaches
                    // 4.6 * kShipScale(0.5) = 2.3 m from the centre; a ball spawned
                    // inside it gets trapped and rides along with the ship.
                    const RwReal kMuzzleOffset = 4.0f;
                    RwV3d firePos = {ss.pos.x + fwd.x * kMuzzleOffset,
                                     ss.pos.y + 2.5f,
                                     ss.pos.z + fwd.z * kMuzzleOffset};
                    SCannonballSpawnData spawnData;
                    std::memset(&spawnData, 0, sizeof(spawnData));
                    spawnData.m_Pos = firePos;
                    spawnData.m_Vel = {fwd.x * 55.0f, fwd.y * 55.0f, fwd.z * 55.0f};
                    spawnData.m_Type = CANNONBALL_STANDARD;
                    spawnData.m_uOwnerID = 1;
                    CCannonball *pBall = new CCannonball();
                    if (pBall != nullptr) {
                        pBall->Spawn(spawnData);
                        s_cannonballs.push_back(pBall);
                        RWS::HUD_UseAmmo(1);
                        ++s_shotsFired;
                        std::printf("CannonCruise: fired cannonball from "
                                    "(%.1f,%.1f,%.1f) vel=(%.1f,%.1f,%.1f)%s\n",
                                    firePos.x, firePos.y, firePos.z,
                                    spawnData.m_Vel.x, spawnData.m_Vel.y,
                                    spawnData.m_Vel.z,
                                    spawnData.m_uOwnerID == 1 ? "" : "");
                    }
                }
                audioMgr.PlaySound("Audio_CannonFire");
                } // ammo > 0
            }
// Real collision-driven tower damage via Bullet contact detection.
// When a cannonball collides with a tower, the physics world reports a contact;
// we verify overlap and apply damage through the established TakeDamage/OnHit path.
// This replaces the original hand-written AABB broadphase check with world-guided hits.

// When both towers are destroyed, transition to LEVELCOMPLETE.
if (bSimActive == TRUE) {
    const RwReal ballRadius = 0.8f;
    const RwReal towerHalf = CTowerCatapult::GetHalfExtents().y;
    const RwReal towerHalfSpace = CTowerCatapult::GetHalfExtents().x;
    const RwReal towerCenterY = CTowerCatapult::GetCenterOffsetY();
    // Bullet keeps the ball outside the tower's box (it bounces off the
    // surface), so treat resting contact within this slop as a hit.
    const RwReal kContactSlop = 0.3f;
    for (size_t ti = 0; ti < s_towers.size(); ++ti) {
        CTowerCatapult *pT = s_towers[ti];
        if (pT->IsDestroyed()) continue;
        const RwV3d &tp = pT->GetWorldPosition();
        for (size_t bi = 0; bi < s_cannonballs.size(); ++bi) {
            CCannonball *pB = s_cannonballs[bi];
            void *pBody = pB->GetRigidBody();
            if (pBody == nullptr) continue;
            RwV3d bp;
            PhysicsBody_GetPosition(pBody, &bp.x, &bp.y, &bp.z);
            const RwReal dx = bp.x - tp.x;
            const RwReal dy = bp.y - (tp.y + towerCenterY);
            const RwReal dz = bp.z - tp.z;
            const RwReal ax = std::fabs(dx) - (towerHalfSpace + ballRadius);
            const RwReal ay = std::fabs(dy) - (towerHalf + ballRadius);
            const RwReal az = std::fabs(dz) - (towerHalfSpace + ballRadius);
            if (ax < kContactSlop && ay < kContactSlop && az < kContactSlop) {
                // Damage already applied via Bullet contact -- skip duplicate.
                bool alreadyDamaged = FALSE;
                for (size_t di = 0; di < s_cannonballs.size(); ++di) {
                    if (di == bi) continue;
                    void *otherBody = s_cannonballs[di]->GetRigidBody();
                    if (otherBody == nullptr) continue;
                    RwV3d otherbp;
                    PhysicsBody_GetPosition(otherBody, &otherbp.x, &otherbp.y, &otherbp.z);
                    const RwReal oax = std::fabs(otherbp.x - tp.x) - (towerHalfSpace + ballRadius);
                    const RwReal oay = std::fabs(otherbp.y - (tp.y + towerCenterY)) - (towerHalf + ballRadius);
                    const RwReal oaz = std::fabs(otherbp.z - tp.z) - (towerHalfSpace + ballRadius);
                    if (oax < kContactSlop && oay < kContactSlop && oaz < kContactSlop) {
                        alreadyDamaged = TRUE;
                        break;
                    }
                }
                if (!alreadyDamaged) {
                    std::printf("CannonCruise: HIT tower #%zu at (%.1f,%.1f,%.1f) (ball=(%.1f,%.1f,%.1f)\n",
                        ti, tp.x, tp.y, tp.z, bp.x, bp.y, bp.z);
                    pT->TakeDamage(100.0f); // 100 damage -- 50 left of 150 HP
                    ++s_shotsHit;
                    if (pT->IsDestroyed()) {
                        const RwBool bMission = (ti < s_towerIsMission.size())
                                                    ? s_towerIsMission[ti] : TRUE;
                        RWS::HUD_RegisterTowerDestroyed(bMission);
                        // Swap to the damaged model (Viking_firetower_Damaged).
                        if (ti < s_towerClumps.size() && s_towerClumps[ti] != nullptr) {
                            RenderPlatform_RemoveClump(s_towerClumps[ti]);
                            RpClumpDestroy(s_towerClumps[ti]);
                            s_towerClumps[ti] = nullptr;
                            const RwsResource *pDmg = static_cast<const RwsResource *>(
                                s_towerDamaged[ti]);
                            if (pDmg != nullptr && s_towerMatrices[ti].size() == 16) {
                                RpClump *pDmgClump =
                                    RpClumpStreamReadMemory(pDmg->pData, pDmg->size);
                                if (pDmgClump != nullptr) {
                                    RpClumpPortSetMatrix(pDmgClump, &s_towerMatrices[ti][0]);
                                    RenderPlatform_AddClump(pDmgClump);
                                    s_towerClumps[ti] = pDmgClump;
                                }
                            }
                        }
                        std::printf("CannonCruise: tower #%zu DESTROYED%s; score tick\n",
                                    ti, bMission == TRUE ? " (mission)" : "");
                    }
                    // Cannonball is destroyed after dealing damage
                    pB->OnHit(pT, 0);
                }
            }
        }
    }
    // All mission towers down -> LEVELCOMPLETE (summary screen state).
    if (s_gameState == GAMESTATE_GAMEPLAY && !s_towers.empty()) {
        RwBool bAllDown = TRUE;
        for (size_t ti = 0; ti < s_towers.size(); ++ti) {
            const RwBool bMission = (ti < s_towerIsMission.size())
                                        ? s_towerIsMission[ti] : TRUE;
            if (bMission == TRUE && !s_towers[ti]->IsDestroyed()) {
                bAllDown = FALSE;
                break;
            }
        }
        if (bAllDown == TRUE) {
            s_gameState = GAMESTATE_LEVELCOMPLETE;
            std::printf("CannonCruise: all mission towers destroyed -> LEVELCOMPLETE (score=%d)\n",
                        RWS::HUD_GetScore());
        }
    }
}
            // Enemy ships: a cannonball inside the hull's rough capsule hits;
            // the second hit sinks the ship (+score).
            if (bSimActive == TRUE) {
                for (size_t ei = 0; ei < s_enemies.size(); ++ei) {
                    EnemyShipRuntime &e = s_enemies[ei];
                    if (enemyPresent(e) != TRUE || e.sinkTime > 0.0f) {
                        continue;
                    }
                    const RwReal kHitRadius = 4.5f * e.scale;
                    for (size_t bi = 0; bi < s_cannonballs.size(); ++bi) {
                        CCannonball *pB = s_cannonballs[bi];
                        void *pBody = pB->GetRigidBody();
                        if (pBody == nullptr || !pB->IsAlive()) continue;
                        RwV3d bp;
                        PhysicsBody_GetPosition(pBody, &bp.x, &bp.y, &bp.z);
                        const RwReal dx = bp.x - e.x, dz = bp.z - e.z;
                        const RwReal dy = bp.y - (e.y + 1.5f);
                        if (dx * dx + dz * dz > kHitRadius * kHitRadius ||
                            std::fabs(dy) > 3.5f) {
                            continue;
                        }
                        e.hp -= 100.0f;
                        ++s_shotsHit;
                        std::printf("CannonCruise: HIT enemy ship #%zu at (%.1f,%.1f) hp=%.0f\n",
                                    ei, e.x, e.z, e.hp);
                        if (e.hp <= 0.0f) {
                            e.sinkTime = 1.0f / 60.0f;
                            ++s_enemiesSunk;
                            RWS::HUD_AddScore(250);
                            LevelScript_Send(e.deathEvent);
                            std::printf("CannonCruise: enemy ship #%zu SUNK (%d sunk)\n",
                                        ei, (int)s_enemiesSunk);
                        }
                        pB->OnHit(nullptr, 0);
                        break;
                    }
                }
            }
            // Sprint 1 -- remove dead cannonballs from the list
            if (bSimActive == TRUE) {
                for (auto it = s_cannonballs.begin();
                     it != s_cannonballs.end();) {
                    if (!(*it)->IsAlive()) {
                        delete *it;
                        it = s_cannonballs.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            if (bSimActive == TRUE && RwPlatformKeyDown('T')) {
                speakMgr.PlaySpeak(2, 0);
            } // world->render() -- between Begin/End
            // Phase 4.6s -- chase cam is the default when a world loads
            // (CANNONCRUISE_CAM=FREE opts out to freecam; =DATA keeps the
            // binary-extracted chase-rig pose). The FOLLOW branch below
            // re-derives camPos/camLook from the player ship each frame.
            const char *camFollowEnv = std::getenv("CANNONCRUISE_CAM");
            const RwBool bFollowCam =
                (camFollowEnv == nullptr ||
                 std::strcmp(camFollowEnv, "FOLLOW") == 0)
                    ? TRUE
                    : FALSE;
            static RwReal followOrbit = 0.0f; // mouse orbit around ship (rad)
            // Phase 4.6q -- freecam: mouse look + WASD (in view plane) +
            // Q/E (vertical) + arrow keys (yaw/pitch). Null platform no key/
            // mouse -> no-op. View derived from yaw/pitch.
            if (bSimActive != TRUE) {
                // paused: camera frozen in place
            } else if (bFollowCam == TRUE) {
                // Ship: W/S throttle, A/D rudder; mouse X camera orbit (clamped).
                const RwReal kDt = 1.0f / 60.0f;
                RwReal throttle = 0.0f;
                if (RwPlatformKeyDown('W')) {
                    throttle += 1.0f;
                }
                if (RwPlatformKeyDown('S')) {
                    throttle -= 0.5f;
                }
                // rudder +1 turns the ship to starboard (screen right) in
                // PlayerShip_Update, so A (port) is -1 and D (starboard) +1.
                RwReal rudder = 0.0f;
                if (RwPlatformKeyDown('A')) {
                    rudder -= 1.0f;
                }
                if (RwPlatformKeyDown('D')) {
                    rudder += 1.0f;
                }
                int mdx = 0, mdy = 0;
                if (RwPlatformMouseDelta(&mdx, &mdy) == TRUE) {
                    followOrbit += (RwReal)mdx * 0.0035f;
                }
                followOrbit *= 0.94f; // clamp -- turns behind the ship
                // CANNONCRUISE_DRIVE=throttle,rudder (headless tests): fixed input.
                static const char *s_driveEnv = std::getenv("CANNONCRUISE_DRIVE");
                if (s_driveEnv != nullptr) {
                    float dThrottle = 0.0f, dRudder = 0.0f;
                    if (std::sscanf(s_driveEnv, "%f,%f", &dThrottle, &dRudder) >= 1) {
                        throttle = dThrottle;
                        rudder = dRudder;
                    }
                }
                // CANNONCRUISE_AUTOPILOT=1 (tests): steer for the script's
                // current goal marker.
                static const RwBool s_autoPilot =
                    std::getenv("CANNONCRUISE_AUTOPILOT") != nullptr ? TRUE : FALSE;
                RwReal apX = 0.0f, apZ = 0.0f;
                PlayerShipState apShip;
                RwBool apHaveGoal = (s_autoPilot == TRUE && PlayerShip_GetState(&apShip) == TRUE)
                                        ? LevelScript_GetGoal(&apX, &apZ) : FALSE;
                // A visible fish is collected by sailing into it.
                for (size_t fi = 0; s_autoPilot == TRUE && fi < s_fishes.size(); ++fi) {
                    const FishRuntime &f = s_fishes[fi];
                    if (f.collected != TRUE && f.spawnEvent[0] != 0 &&
                        LevelScript_WasSent(f.spawnEvent) == TRUE) {
                        apX = f.x;
                        apZ = f.z;
                        apHaveGoal = TRUE;
                        break;
                    }
                }
                if (apHaveGoal == TRUE) {
                    RwReal dyaw = std::atan2(apX - apShip.pos.x, apZ - apShip.pos.z) - apShip.yaw;
                    while (dyaw > 3.14159265f) dyaw -= 6.2831853f;
                    while (dyaw < -3.14159265f) dyaw += 6.2831853f;
                    rudder = -dyaw * 2.0f; // rudder +1 turns to starboard (yaw decreases)
                    if (rudder > 1.0f) rudder = 1.0f;
                    if (rudder < -1.0f) rudder = -1.0f;
                    throttle = std::fabs(dyaw) > 1.2f ? 0.3f : 1.0f;
                }
                // Ride the same wave phase the ocean is drawn with this frame
                // (the ocean adds kDt to its time later in the frame).
                const RwReal waveT = (pOcean != nullptr)
                                         ? pOcean->GetWaveTime() + kDt
                                         : (RwReal)frame * kDt;
                PlayerShip_Update(kDt, waveT, throttle, rudder);
                PlayerShipState shipState;
                if (PlayerShip_GetState(&shipState) == TRUE) {
                    const RwReal ang =
                        shipState.yaw + 3.14159265f + followOrbit;
                    // Camera above-behind, tied rigidly to the ship: looking
                    // down over the stern at the deck and bow (reference view).
                    camPos[0] =
                        shipState.pos.x + std::sin(ang) * kFollowCameraDistance;
                    camPos[1] = shipState.pos.y + kFollowCameraHeight;
                    camPos[2] =
                        shipState.pos.z + std::cos(ang) * kFollowCameraDistance;
                    ClampCameraToMap(camPos);
                    // aim at the deck, slightly ahead (over the bow)
                    camLook[0] = shipState.pos.x + std::sin(shipState.yaw) * 2.0f;
                    camLook[1] = shipState.pos.y + 1.5f;
                    camLook[2] = shipState.pos.z + std::cos(shipState.yaw) * 2.0f;
                    if (frame <= 3 && std::getenv("CANNONCRUISE_CAM_DEBUG") != nullptr) {
                        std::fprintf(stderr, "FOLLOWDBG frame=%d ship=(%.1f,%.1f,%.1f) cam=(%.1f,%.1f,%.1f)\n",
                                    frame, shipState.pos.x, shipState.pos.y, shipState.pos.z,
                                    camPos[0], camPos[1], camPos[2]);
                    }
                    RenderPlatform_SetCameraPose(camPos, camLook);
                }
            } else {
                const RwReal kDt = 1.0f / 60.0f;
                const RwReal speed = camSpan * 0.10f * kDt;
                const RwReal turn = 1.8f * kDt; // arrow key rad/s
                const RwReal sens = 0.0035f;    // mouse radians/pixel
                RwBool lookChanged = FALSE;
                if (RwPlatformKeyDown(RW_KEY_LEFT)) {
                    camYaw += turn;
                    lookChanged = TRUE;
                }
                if (RwPlatformKeyDown(RW_KEY_RIGHT)) {
                    camYaw -= turn;
                    lookChanged = TRUE;
                }
                if (RwPlatformKeyDown(RW_KEY_UP)) {
                    camPitch += turn;
                    lookChanged = TRUE;
                }
                if (RwPlatformKeyDown(RW_KEY_DOWN)) {
                    camPitch -= turn;
                    lookChanged = TRUE;
                }
                int mdx = 0;
                int mdy = 0;
                if (RwPlatformMouseDelta(&mdx, &mdy) == TRUE) {
                    camYaw += (RwReal)mdx * sens;
                    camPitch -= (RwReal)mdy * sens; // drag up = look up
                    lookChanged = TRUE;
                }
                const RwReal kMaxPitch = 1.55f; // ~89°
                if (camPitch > kMaxPitch) {
                    camPitch = kMaxPitch;
                }
                if (camPitch < -kMaxPitch) {
                    camPitch = -kMaxPitch;
                }
                // view: fwd = (sin(yaw)cos(p), sin(p), cos(yaw)cos(p))
                const RwReal cp = std::cos(camPitch);
                const RwReal fwd[3] = {std::sin(camYaw) * cp,
                                       std::sin(camPitch),
                                       std::cos(camYaw) * cp};
                const RwReal right[3] = {std::cos(camYaw), 0.0f,
                                         -std::sin(camYaw)}; // yaw+90°
                RwReal delta[3] = {0.0f, 0.0f, 0.0f};
                if (RwPlatformKeyDown('W')) {
                    delta[0] += fwd[0] * speed;
                    delta[1] += fwd[1] * speed;
                    delta[2] += fwd[2] * speed;
                }
                if (RwPlatformKeyDown('S')) {
                    delta[0] -= fwd[0] * speed;
                    delta[1] -= fwd[1] * speed;
                    delta[2] -= fwd[2] * speed;
                }
                if (RwPlatformKeyDown('A')) {
                    delta[0] -= right[0] * speed;
                    delta[2] -= right[2] * speed;
                }
                if (RwPlatformKeyDown('D')) {
                    delta[0] += right[0] * speed;
                    delta[2] += right[2] * speed;
                }
                if (RwPlatformKeyDown('Q')) {
                    delta[1] -= speed;
                }
                if (RwPlatformKeyDown('E')) {
                    delta[1] += speed;
                }
                const RwBool moved =
                    (delta[0] != 0.0f || delta[1] != 0.0f || delta[2] != 0.0f)
                        ? TRUE
                        : FALSE;
                if (moved == TRUE) {
                    camPos[0] += delta[0];
                    camPos[1] += delta[1];
                    camPos[2] += delta[2];
                    ClampCameraToMap(camPos);
                }
                if (lookChanged == TRUE || moved == TRUE) {
                    camLook[0] = camPos[0] + fwd[0];
                    camLook[1] = camPos[1] + fwd[1];
                    camLook[2] = camPos[2] + fwd[2];
                    RenderPlatform_SetCameraPose(camPos, camLook);
                }
            }
            if (pOcean != nullptr) {
                if (bSimActive == TRUE) {
                    // COcean::Update takes a delta and accumulates it; passing
                    // the absolute time made the water run ahead of the ship's
                    // bob, so wave crests covered the hull.
                    pOcean->Update(1.0f / 60.0f);
                }
                pOcean->Render(); // between Begin/End
            }
            // Enemy ships: bob on the wave at their position, drawn after the
            // ocean (same reason as the player ship).
            for (size_t ei = 0; ei < s_enemies.size(); ++ei) {
                EnemyShipRuntime &e = s_enemies[ei];
                if (enemyPresent(e) != TRUE) {
                    continue;
                }
                const RwReal kEnemyDt = 1.0f / 60.0f;
                RwReal steer = 0.0f;
                if (bSimActive == TRUE && e.sinkTime <= 0.0f && e.hasWaypoint == TRUE) {
                    // Sail the current leg; turn round near its end point.
                    const RwReal tx = e.toWaypoint == TRUE ? e.wpX : e.homeX;
                    const RwReal tz = e.toWaypoint == TRUE ? e.wpZ : e.homeZ;
                    const RwReal dx = tx - e.x, dz = tz - e.z;
                    if (dx * dx + dz * dz < 8.0f * 8.0f) {
                        e.toWaypoint = e.toWaypoint == TRUE ? FALSE : TRUE;
                    }
                    RwReal dyaw = std::atan2(dx, dz) - e.yaw;
                    while (dyaw > 3.14159265f) dyaw -= 6.2831853f;
                    while (dyaw < -3.14159265f) dyaw += 6.2831853f;
                    const RwReal kTurn = 0.5f * kEnemyDt; // rad per frame
                    steer = dyaw > kTurn ? 1.0f : (dyaw < -kTurn ? -1.0f : dyaw / kTurn);
                    e.yaw += steer * kTurn;
                    // Slow down while turning hard.
                    const RwReal kSpeed = 5.0f * (1.0f - 0.6f * std::fabs(steer));
                    e.x += std::sin(e.yaw) * kSpeed * kEnemyDt;
                    e.z += std::cos(e.yaw) * kSpeed * kEnemyDt;
                }
                e.y = (pOcean != nullptr) ? pOcean->CalculateWaterHeight(e.x, e.z) + 0.8f
                                          : 0.8f;
                RwReal roll = 0.0f;
                if (e.sinkTime > 0.0f) {
                    if (bSimActive == TRUE) e.sinkTime += kEnemyDt;
                    e.y -= e.sinkTime * e.sinkTime * 0.8f;
                    roll = (e.sinkTime < 1.2f ? e.sinkTime : 1.2f) * 0.5f;
                    if (e.sinkTime > 5.0f) {
                        e.gone = TRUE;
                        continue;
                    }
                }
                // Rotation: yaw about Y, then roll about the bow axis (sinking).
                const RwReal sy = std::sin(e.yaw), cy = std::cos(e.yaw);
                const RwReal sr = std::sin(roll), cr = std::cos(roll);
                const RwReal s = e.scale;
                // right = (cy,0,-sy), up = (0,1,0), at = (sy,0,cy); roll about at.
                const RwReal rx = cy * cr, ry = sr, rz = -sy * cr;
                const RwReal ux = -cy * sr, uy = cr, uz = sy * sr;
                const RwReal m[16] = {rx * s, ry * s, rz * s, 0.0f,
                                      ux * s, uy * s, uz * s, 0.0f,
                                      sy * s, 0.0f, cy * s, 0.0f,
                                      e.x, e.y, e.z, 1.0f};
                RpClumpPortSetMatrix(e.clump, m);
                if (e.animator != nullptr) {
                    // Paddles row while sailing (pose ranges loop at 0.4 s
                    // like Hugo_Ship's clips); the rudder follows the steer.
                    static RwReal s_paddleT = 0.0f;
                    if (ei == 0 && bSimActive == TRUE) {
                        s_paddleT += kEnemyDt;
                    }
                    const RwBool bRowing = (e.hasWaypoint == TRUE && e.sinkTime <= 0.0f);
                    const RwReal paddleT = bRowing ? std::fmod(s_paddleT * 0.5f, 0.4f) : 0.0f;
                    RpClumpPortAnimatorSetPose(e.animator, e.clipPaddleL, paddleT);
                    RpClumpPortAnimatorSetPose(e.animator, e.clipPaddleR, paddleT);
                    RpClumpPortAnimatorSetPose(e.animator, e.clipRudder, 0.2f + steer * 0.18f);
                    RpClumpPortAnimatorUpdate(e.animator);
                }
                RenderPlatform_RenderClump(e.clump);
            }
            // Keep the hull above the water mesh in the depth buffer.
            if (s_pShipClump != nullptr) {
                // Real model follows the simulated ship: forward = (sin yaw, 0, cos yaw).
                PlayerShipState shipNow;
                if (PlayerShip_GetState(&shipNow) == TRUE) {
                    const RwReal sy = std::sin(shipNow.yaw), cy = std::cos(shipNow.yaw);
                    const RwReal m[16] = {cy, 0.0f, -sy, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          sy, 0.0f, cy, 0.0f,
                                          shipNow.pos.x, shipNow.pos.y,
                                          shipNow.pos.z, 1.0f};
                    RpClumpPortSetMatrix(s_pShipClump, m);
                    if (s_pShipAnimator != nullptr) {
                        // Clips are 0.4 s pose ranges (0.4 wraps to 0, so stay
                        // inside 0.02..0.38):
                        //  wings  0 = spread (at rest) .. folded back (full speed)
                        //  rudder/wheel 0.2 = centred, +-0.18 = full rudder
                        //  gun    0.2 = pointing forward
                        RwReal speedNorm = std::fabs(shipNow.speed) / 18.0f;
                        if (speedNorm > 1.0f) speedNorm = 1.0f;
                        const RwReal wingT = 0.02f + speedNorm * 0.36f;
                        const RwReal rudderT = 0.2f + shipNow.rudder * 0.18f;
                        RpClumpPortAnimatorSetPose(s_pShipAnimator, s_shipClipWingL, wingT);
                        RpClumpPortAnimatorSetPose(s_pShipAnimator, s_shipClipWingR, wingT);
                        RpClumpPortAnimatorSetPose(s_pShipAnimator, s_shipClipRudder, rudderT);
                        RpClumpPortAnimatorSetPose(s_pShipAnimator, s_shipClipWheel, rudderT);
                        RpClumpPortAnimatorSetPose(s_pShipAnimator, s_shipClipGun, 0.2f);
                        RpClumpPortAnimatorUpdate(s_pShipAnimator);
                    }
                    // After the ocean: the water mesh drawn later used to paint
                    // over the low hull on some wave phases.
                    RenderPlatform_RenderClump(s_pShipClump);
                }
            } else {
                PlayerShip_Render(); // placeholder hull (GL3/D3D9: im3d)
            }
            // Level script (tutorial texts, goal markers, triggers) and fish.
            {
                PlayerShipState ss;
                const RwBool bHaveShip = PlayerShip_GetState(&ss);
                if (bSimActive == TRUE && bHaveShip == TRUE) {
                    static RwBool s_nextPrev = FALSE;
                    const RwBool nextDown = RwPlatformKeyDown(RW_KEY_ENTER);
                    const RwBool nextEdge = (nextDown == TRUE && s_nextPrev != TRUE) ? TRUE : FALSE;
                    s_nextPrev = nextDown;
                    LevelScript_Update(1.0f / 60.0f, ss.pos.x, ss.pos.z, nextEdge);
                    // Fish: sail into one, or land a cannonball next to it.
                    for (size_t fi = 0; fi < s_fishes.size(); ++fi) {
                        FishRuntime &f = s_fishes[fi];
                        if (f.collected == TRUE ||
                            (f.spawnEvent[0] != 0 && LevelScript_WasSent(f.spawnEvent) != TRUE)) {
                            continue;
                        }
                        RwBool got = FALSE;
                        const RwReal sdx = ss.pos.x - f.x, sdz = ss.pos.z - f.z;
                        if (sdx * sdx + sdz * sdz < 6.0f * 6.0f) {
                            got = TRUE;
                        }
                        for (size_t bi = 0; got != TRUE && bi < s_cannonballs.size(); ++bi) {
                            void *pBody = s_cannonballs[bi]->GetRigidBody();
                            if (pBody == nullptr || !s_cannonballs[bi]->IsAlive()) continue;
                            RwV3d bp;
                            PhysicsBody_GetPosition(pBody, &bp.x, &bp.y, &bp.z);
                            const RwReal bdx = bp.x - f.x, bdz = bp.z - f.z;
                            if (bdx * bdx + bdz * bdz < 4.0f * 4.0f && bp.y < 3.0f) {
                                got = TRUE;
                                s_cannonballs[bi]->OnHit(nullptr, 1);
                            }
                        }
                        if (got == TRUE) {
                            f.collected = TRUE;
                            RWS::HUD_AddFish(1);
                            const RwReal h = RWS::HUD_GetHealth() + 0.1f;
                            RWS::HUD_SetHealth(h > 1.0f ? 1.0f : h);
                            RWS::HUD_AddScore(50);
                            LevelScript_Send(f.pickupEvent);
                            std::printf("CannonCruise: fish #%zu collected\n", fi);
                        }
                    }
                    char nextLevel[64];
                    if (s_gameState == GAMESTATE_GAMEPLAY &&
                        LevelScript_LevelExitReached(nextLevel, sizeof(nextLevel)) == TRUE) {
                        s_gameState = GAMESTATE_LEVELCOMPLETE;
                        std::printf("CannonCruise: level script exit (%s) -> LEVELCOMPLETE (score=%d)\n",
                                    nextLevel, RWS::HUD_GetScore());
                    }
                }
                RwReal gx = 0.0f, gz = 0.0f;
                if (bHaveShip == TRUE && LevelScript_GetGoal(&gx, &gz) == TRUE) {
                    RWS::HUD_SetGoalBlip(TRUE, gx - ss.pos.x, gz - ss.pos.z);
                } else {
                    RWS::HUD_SetGoalBlip(FALSE, 0.0f, 0.0f);
                }
            }
            // Fish models bob on the water until collected.
            for (size_t fi = 0; fi < s_fishes.size(); ++fi) {
                FishRuntime &f = s_fishes[fi];
                if (f.clump == nullptr || f.collected == TRUE ||
                    (f.spawnEvent[0] != 0 && LevelScript_WasSent(f.spawnEvent) != TRUE)) {
                    continue;
                }
                const RwReal fy = (pOcean != nullptr) ? pOcean->CalculateWaterHeight(f.x, f.z) : 0.0f;
                const RwReal m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, f.x, fy + 0.2f, f.z, 1};
                RpClumpPortSetMatrix(f.clump, m);
                RenderPlatform_RenderClump(f.clump);
            }
            // Sprint 2 � minimal HUD overlay: player HP + tower sayac + skor.
            {
                PlayerShipState ss;
                if (PlayerShip_GetState(&ss) == TRUE) {
                    RWS::HUD_SetShipHeading(ss.yaw);
                    for (size_t ti = 0; ti < s_towers.size(); ++ti) {
                        CTowerCatapult *pT = s_towers[ti];
                        const RwV3d &tp = pT->GetWorldPosition();
                        RWS::HUD_SetTowerBlip(
                            (RwInt32)ti, tp.x - ss.pos.x, tp.z - ss.pos.z,
                            pT->IsDestroyed() ? FALSE : TRUE);
                    }
                }
                RWS::HUD_Render();
                LevelScript_Render();
            }
            // Pause overlay (frozen world, dim, two options)
            if (s_gameState == GAMESTATE_PAUSE) {
                RenderPauseOverlay(s_pauseSelection);
            }
            // LEVELCOMPLETE: show summary screen with score/accuracy/rating
            else if (s_gameState == GAMESTATE_LEVELCOMPLETE) {
                static CAfterGameMenuBehavior afterGame;
                static RwBool bAfterInit = FALSE;
                if (!bAfterInit) {
                    afterGame.AttachMaestro(nullptr);
                    bAfterInit = TRUE;
                }
                // Read current game metrics from HUD
                RwUInt32 score = static_cast<RwUInt32>(RWS::HUD_GetScore());
                const RwUInt32 accuracy = s_shotsFired > 0
                    ? (RwUInt32)((s_shotsHit * 100) / s_shotsFired) : 0u;
                RwUInt32 rating = 0;   // TODO: original star-rating rule unknown
                afterGame.ShowSummary(score, accuracy, rating);
                RenderLevelCompleteOverlay(RWS::HUD_GetScore(),
                                           RWS::HUD_GetTowersDestroyed(),
                                           RWS::HUD_GetTowersTotal(),
                                           s_shotsFired, s_shotsHit);
            }
        }
        if (RenderPlatform_RenderTestQuad(0.0f, 0.0f, 0.25f, 0.25f,
                                          0xFF40A0FF) != TRUE) {
            std::fprintf(stderr, "CannonCruise: TestQuad %d failed\n",
                         frame);
            break;
        }
        // World frame capture (if phase pulls didn't consume; one-time only).
        const char *shotFrameEnv = std::getenv("CANNONCRUISE_SHOT_FRAME");
        const int shotFrame =
            (shotFrameEnv != nullptr) ? std::atoi(shotFrameEnv) : 2;
        if (pShotPath != nullptr && frame == shotFrame) {
            RenderPlatform_RequestScreenshot(pShotPath);
        }
        RenderPlatform_EndFrame();
        ++framesRendered;
    }

    if (kFrames < 0) {
        std::printf(
            "CannonCruise: %d frames OK (unlimited; world=%p camera=%p "
            "clump=%p shape=%p assets=%u)\n",
            framesRendered, RenderPlatform_GetWorld(), RenderPlatform_GetCamera(),
            pipeline.pClump, pipeline.pCollisionShape,
            (bWorldLoaded == TRUE) ? pipeline.assetCount : 0u);
    } else {
        std::printf(
            "CannonCruise: %d/%d frames OK (world=%p camera=%p clump=%p "
            "shape=%p assets=%u)\n",
            framesRendered, kFrames, RenderPlatform_GetWorld(),
            RenderPlatform_GetCamera(), pipeline.pClump,
            pipeline.pCollisionShape,
            (bWorldLoaded == TRUE) ? pipeline.assetCount : 0u);
    }

    audioMgr.StopAllVoices();
    speakMgr.StopSpeak();
    for (auto *t : s_towers) { delete t; }
    s_towers.clear();
    s_towerIsMission.clear();
    for (auto *b : s_cannonballs) { delete b; }
    s_cannonballs.clear();
    PlayerShip_Shutdown(); // PhysicsWorld dtor assert'i i�in rigid body temizligi
    delete pOcean; // COcean::Shutdown + tile cleanup
    RwPlatformSetMouseCapture(FALSE); // cursor back

    RwsPipeline_Unload(&pipeline);
    RenderPlatform_Terminate();
    RwEngineStop();
    RwEngineClose();
    RwEngineTerm();

    // Window close on world path (ShouldClose) counts as success.
    return (framesRendered > 0) ? 0 : 1;
}
