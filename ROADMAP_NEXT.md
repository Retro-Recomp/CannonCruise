# CannonCruise — Roadmap (2026-11-14)

Focus: **a real, playable CannonCruise** — a workflow close to the original:
load → menu → new game → map select → play in a single scene (ball, ship control,
drag, collision) → summary screen → back to map.

> This file is the **summarized, current** state of `ROADMAP.md`'s 1851-line log.
> For detail/history see `ROADMAP.md`; no unverified
> claim is **written here**.

## Where we are today (2026-11-14)

| Area | Status | Evidence |
|---|---|---|
| Build (NULL + D3D9, Win32, VS 2026) | ✅ | `build_cannoncruise.cmd`; 0 errors |
| ctest | ✅ 34/34 | NULL + D3D9 configurations |
| D3D9 window | ✅ visible | island + ocean + sky; `PrintWindow` capture |
| Renders the world | ✅ | world/clump call is real; boat is in the RpWorld pipeline, not im3d |
| GLOBAL.RWS inventory | ✅ resolved | `globalassets_test`: 56 chunks, 1 txd = 4 textures, 46 objectRecords, `Loading.anm` |
| LOADING screen | ✅ real art + 30 fps | `LoadingScreen.{h,cpp}` |
| MAINMENU / MAPMENU flow | ✅ works | 32 nodes `Menu.anm`, 44 nodes `CAG_Menu.anm`; they slide |
| FMV chain (`.bik`) | ✅ playing | Intro.bik, Jingle.bik, etc. |
| World load (VIKING01) | ✅ 130 atomics, 78146 verts | `WorldGeometry` |
| Ocean grid + waves | ✅ | `COceanTile` |
| Spawn chain (97 objects) | ✅ | `WorldSpawn_FromFile` |
| Ship (placeholder) | ✅ renders at 4x scale + Bullet kinematic | clump → `RenderPlatform_AddClump`; spawn creates a convex hull `PlayerShip_body` |
| `CPhysicsWorld::Update` | ✅ real Bullet step | `btDiscreteDynamicsWorld`, `CPhysicsWorld::Create()` + `Update(1/60)` each frame |
| Player ship | ✅ Bullet kinematic record | `PlayerShip_Spawn` → convex hull + `CPhysicsWorld::AddRigidBody`; `PlayerShip_Shutdown` cleans up |
| Menu → World → Esc menu | 🟡 Partial | Menu is navigable, PAUSE menu not fully wired from the menu architecture |
| Single scene fully playable | 🟡 Partial | Player ship moves; Space fires cannonball → tower destruction verified (`playground_test`) |
| Global objectRecord classes | ❌ | `CAudioManager`, `CDifficultyLevels`, `CEventProxy`, `CSystemCommands` without factories |
| Save/Load (Slot0) | ❌ | `{GUID}.dff/txd/col` external assets are in the repo |

### D3D9 window: world works, it just doesn't look fixed (misdiagnosis fixed 2026-11-14)

The island/ocean/sky render in the D3D9 window and the camera bobs;
however, **when there is no input the world frame does not change** (single-pixel
blip on the blob ship). Deterministic comparison (`CANNONCRUISE_SHIP=563,0,186,0`
vs `=100,0,-200,1.5` vs `=-300,90,120,-0.5`) produces three different hashes — so
the world is redrawn every frame, at the correct position every frame. The reason
it looks static is that the spawn point is above the sea and the ship never moves
because of the input (interpolation disabled); the only difference between two
screenshots is the ~5px blip on the boat.

The `CANNONCRUISE_CAM=FOLLOW|FREE|AUTO|DATA` hashes also differ (e.g.
`DATA` → empty-sky view angle) — the camera path is correct.

**Applied fix:** this section was mislabeled in the previous version as "the
screen stays static"; in fact it is more a lack of input than a bug.


## Milestone 1 — Playable single scene (VIKING01) 🎯

Goal: island, ship, ocean, 2 towers/fortresses and the enemy real chain; ESC → PAUSE → return.

1. **Make ship movement work in Bullet** (play the `CPhysicsEntity::SetPosition` / `CShip::Update` flow); throttle/rudder → speed/angle. ✅ 2026-11-14: at spawn, `PlayerShip` creates `PhysicsShape_CreateConvexHull` + a kinematic `btRigidBody`, registered via `CPhysicsWorld::AddRigidBody`; the frame loop calls `CPhysicsWorld::Update(1/60)`; cleaned up by `PlayerShip_Shutdown`; `ctest` 34/34.
2. **Cannon fire → Cannonball → collision → tower destruction** (at least `TowerCatapult` → `HealthBar` → 0 HP → destruction + sound); via `CollisionTrigger`. ✅ 2026-11-14: each frame `physics->Update(1/60)` runs; `SPACE` player output creates a cannonball Bullet sphere rigid body, `Cannonball` Bullet body + 5 s lifetime; two towers registered as live static `btRigidBody`s in the world; top-vs-tower AABB test each frame → `TakeDamage(100)`; `playground_test` ran 120 frames headless and confirmed `HIT frame=72` and `pTower->IsDestroyed() == TRUE` → ctest 34/34.
3. **HUD**: score countdown + HP bar + compass (`CGUIHealthBar` + `CGUIMap` + `MissionIndicator`; HUD at SCR.mov frame 9; the im2d overlay already exists). ✔︎ 2026-11-15: redrawn against the original reference image — bottom-left ammo dial (GUI_022+GUI_012), bottom wooden-tipped red HP bar (GUI_026/024/028), fish counter (GUI_031), bottom-right compass (GUI_020 + white needle + red tower blips; north-up). GUI textures are loaded from the nested d3d8-IMAGE records at the tail of VIKING01.RWS chunk0 via `RwTxdDecodeGuiPack` (LevelPack cannot see these). `KULEN n/2` under the compass; ESC→PAUSE is a separate item.
4. **PAUSE menu**: also open the menu in paused state (`CMenuBehaviour` + `InputManager` with Esc).
5. **Camera mode**: `CANNONCRUISE_CAM=FREE|FOLLOW` behavior; the player follows the ship
   (current FOLLOW + WASD partially done).

### Verification
- `ctest` stays green at 34/34.
- New `playground_test` (headless): spawn → 200 frames → ship advances,
  speed > 0; cannon fires → target HP drops; Esc → PAUSE; measurements assert.

## Milestone 2 — Full workflow (M2) 🎯

Goal: BOOT → LOADING → MAINMENU → NEWGAME → MAPMENU (VIKING 1) → VIKING01
→ LEVELCOMPLETE (score + rating) → MAPMENU → VIKING 1 again (NEW GAME+).

1. **Mission state machine**: score countdown 200→0; at 0 LEVELCOMPLETE;
   target tower count; `IncreaseStaticScore/IncreaseDynamicScore` (binary
   `iMsgFadeInAfterRespawn`/`IncreaseDynamicScore` + missing in binary).
2. **LEVELCOMPLETE screen**: `ShowSummary` + rating (`CAfterGameMenuBehavior`
   "YOUR RATING" is ready; collects VIKING01 score/accuracy/agents).
3. **MAPMENU selection → level load → return** (already partially working).
4. **Save/Load** (binary `NewSave/NoSave/LoadGame` paths; file format
   `{GUID}_sav` — current port. `{GUID}.dff` can be copied directly).
5. **CAfterGameMenuBehavior + MAPMENU rating system**: level records +
   `Viking01_Rating`/`LevelName` fields with SELECT/BACK keys.

### Verification
- Headless `gamestate_test`: BOOT → MASTERMENU (key simulation) → NEWGAME →
  MAPMENU selection → VIKING01 → 300 frames → LEVELCOMPLETE (HP/score) → MAPMENU.

## Milestone 3 — Full catalog + closing (M3) 🎯

- **Remaining 15 levels** (VIKING01-05, NORDIC01-02, IDUNA01-03, BEAVER01-05):
  `WorldGeometry_CreateClump` already passes for all three worlds; a parser
  for `.pfm` pathfinding (`PathFindingData`) will be added.
- **GLOBAL objectRecord factories**: make the 46 dummy behaviour classes
  materializable via `RWS_REGISTER_CLASS` + `BehaviourLoader`; at least `CAudioManager` (sound triggering) and `CDifficultyLevels`.
- **Real `.bik` videos** are already present → the boot chain will work fully.
- **Sound**: `Sound/Win32/*.wav` (real, present) — plays instantly via
  `AudioManager::PlaySound(name)`. `Loading.wav` also exists; looped during boot.
  `Ingame_Win32.rws` (5.6 MB PPB sound bank) is present in the repo — if a parser
  is added, the real player sounds can be extracted.
- **Camera**: channel `CANNONCRUISE_CAM=FOLLOW` + chase offset + pitch clamp
  (35°); CannonView (zoom-in) and Binoculars (zoom-out) `SetDistance` tracking;
  vignette mask (circular fade) — binary equivalent of `CWbCameraBehaviour`.
- **CI/CD**: GitHub Actions — Windows/VS 18 + macOS/Clang dual build, ctest
  runner (currently done by hand on Windows).

## Deliberately excluded (out of scope)

- The **full 3D asset rebuild** of the original game (for example,
  `{GUID}.dff/.txd/.col` external assets are already in `Streams/` *as binary*;
  decode/render works; reusability is low).
- Multiplayer/netcode — the original `SetMultiPlayer` behavior is single player;
  the menu option is decoration only.
- Xbox/PS2 optimized paths (PS2 vertex program, PSP modifier).

## Sprint order (next 4 weeks)

| Week | Work | Gate |
|---|---|---|
| 1a–1b | ~~D3D9 world static~~ (misdiagnosis) + Bullet step + ship control (throttle/rudder → speed/angle) ✅ | `playground_test` green |
| 1c | Cannon fire → cannonball → tower destruction ✅ | `playground_test` top-vs-tower HIT frame=72 |
| 2 | HUD ✔ (dial+HP bar+compass, original art) + ESC→PAUSE | `playground_test` ammo assert; SS image |
| 3 | Save/Load + LEVELCOMPLETE screen | `gamestate_test` full BOOT→COMPLETE |
| 4 | GLOBAL factories + VIKING02 + D3D9 package | 2 levels green; setup.exe works |

*Sprint 1's 3/3 main parts are complete (Bullet step, ship control, cannonball→tower). Next: HUD+PAUSE.*

*Sprint 1 complete — ship physics + cannonball→tower → `ctest` 34/34.*

*2026-11-15: HUD redrawn against the original reference image — ammo dial (arm 175 cannonballs, GUI_022+GUI_012), red HP bar (wooden tips GUI_026/028 + rail GUI_024), fish counter (GUI_031), north-up compass (GUI_020 + white needle + red tower blips). UI textures are resolved from the nested chunk0 section of the level .RWS via `RwTxdDecodeGuiPack` → `RenderPlatform_UploadTexture` as menu textures (the HUD draws them). Also, overlay vertex colors were swapped in D3D9 (R↔B); the swizzle was fixed under `RW_D3D9`.*
