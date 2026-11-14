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
| D3D9 runtime | ✅ | Build script had copied Windows DLLs beside the EXE, shadowing the system runtime; copying only `binkw32.dll` restores HAL detection. Clean-output run reached the main menu. |
| Renders the world | ✅ | world/clump call is real; boat is in the RpWorld pipeline, not im3d |
| GLOBAL.RWS inventory | ✅ resolved | `globalassets_test`: 56 chunks, 1 txd = 4 textures, 46 objectRecords, `Loading.anm` |
| LOADING screen | ✅ real art + 30 fps | `LoadingScreen.{h,cpp}` |
| MAINMENU / MAPMENU flow | ✅ works | 32 nodes `Menu.anm`, 44 nodes `CAG_Menu.anm`; they slide |
| FMV chain (`.bik`) | ✅ playing | Intro.bik, Jingle.bik, etc. |
| World load (VIKING01) | ✅ 130 atomics, 78146 verts | `WorldGeometry` |
| Ocean grid + waves | ✅ | `COceanTile` |
| Spawn chain (97 objects) | ✅ | `WorldSpawn_FromFile` |
| Ship (placeholder) | 🟡 independent wood texture, shaped longboat, cannon, mast/rigging + ocean-grid bounds | Hand-built curved hull, deck, rails, benches, cannon, mast and rigging use generated `CC_ShipHull`; no level texture atlas is sampled |
| `CPhysicsWorld::Update` | ✅ real Bullet step | `btDiscreteDynamicsWorld`, `CPhysicsWorld::Create()` + `Update(1/60)` each frame |
| Player ship | 🟡 bounded movement prototype | `PlayerShip_Update` manually integrates throttle/rudder; steering is speed-dependent and the hull center is clamped inside the water grid; Bullet mirrors the transform |
| Menu → World → Esc menu | 🟡 Partial | Menu is navigable, PAUSE menu not fully wired from the menu architecture |
| Single scene fully playable | 🟡 Partial | Player ship moves; projectile-to-tower damage is a test prototype, not yet driven by a real collision callback |
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

## ASM comparison (2026-11-14)

The `.asm` files are disassembler listings of the original PE, not original
source code. This is an initial focused cross-check of physics and gameplay
paths, not an exhaustive class-by-class audit. It records only behavior visible
in the listed instructions; a passing build or port-side test proves that
port's behavior, not parity with the original.

| Area | Original ASM evidence | Current implementation | Assessment |
|---|---|---|---|
| Target | IDA identifies a 32-bit PE for 80386 in [`CC_ida.asm`](./CC_ida.asm). | The documented Windows target is Win32/x86. Runtime graphics/physics use librw/Bullet rather than the original SDKs. | Architecture matches; SDK ABI and behavior do not follow from that. |
| Physics update | [`sub_43A1C0`](./CC_ida.asm#L81809) calls `sub_555EF0` with `0x3D088889` (about 1/30), then calls `nullsub_11`. [`sub_555EF0`](./CC_ida.asm#L534569) transforms that argument and calls further internal routines. Separately, [`sub_4B8710`](./CC_ida.asm#L266418) passes `0x3C888889` (about 1/60) to another routine on a `CSoldier` running-tick path. | [`main.cpp`](./code/main.cpp) calls `CPhysicsWorld::Update(1/60)`; the Bullet backend advances one step for the supplied `dt`. | The current 1/60 tick is not yet demonstrated equivalent. Trace `sub_555EF0` and event cadence before choosing a matching step; the separate 1/60 constant is not by itself proof of the physics-world step. |
| Physics event filter | [`sub_43BB10`](./CC_ida.asm#L84599) compares the incoming message ID with `dword_635C68` and updates the world only on a match. | [`CPhysicsWorldBehaviour::HandleEvents`](./code/PhysicsWorld.cpp) steps on every message; the runtime loop also updates the world directly. | Event semantics differ. Identify the message contract and test that unrelated events never advance physics. |
| World parameters | [`sub_55E6D0`](./CC_ida.asm#L548319) initializes Havok world settings, including gravity, friction, restitution, solver iterations and ±500 bounds. | [`PhysicsBackend.cpp`](./code/PhysicsBackend.cpp) maps gravity/friction/restitution/iterations to Bullet; its DBVT broadphase does not apply the original bounds as a clamp. | Partial parameter mapping; solver and bounds are known behavioral differences until measured. |
| Ship controls/rendering | No original ship-control function has yet been cross-walked to the port in this review. | [`PlayerShip.cpp`](./code/PlayerShip.cpp) uses a hand-authored low-poly longboat hull/deck, cannon and mast rigging with a generated `CC_ShipHull` material, manual throttle/rudder integration, ocean-edge clamping and a kinematic Bullet body. | Useful playable scaffold, not established original ship behavior or model parity; island collision and original model integration remain open. |
| Cannonball hit | The corresponding original spawn/hit instruction paths still need a function-by-function cross-check. | [`playground_test.cpp`](./tests/playground_test.cpp) computes an AABB overlap itself, then explicitly calls `TakeDamage` and `OnHit`; it does not wait for a Bullet contact callback. | Scripted hit test passes, but authentic projectile collision/damage flow remains open. |

Two address/evidence cautions for follow-up: earlier source/migration notes
labeled `CPhysicsWorld::Update` as `FUN_0043a040`, while the inspected Ghidra
and IDA listings label the update routine `FUN_0043a1c0` / `sub_43A1C0`.
Reconcile the symbol map before using the address elsewhere. Earlier notes also
used the soldier tick's 1/60 value as evidence for physics timing; it must not
be conflated with the world-update wrapper's 1/30 argument.

## ASM-driven roadmap

1. **P0 — Close the binary-to-source crosswalk.** Record original address,
   caller/event, arguments, side effects, matching source method and confidence
   for physics-world update, ship control and cannonball spawn/hit. Trace
   `sub_555EF0`, establish the actual world-step cadence, and classify the
   Havok settings that Bullet does not currently map.
   **Gate:** no unresolved 1/30-vs-1/60 claim; each parity claim has an ASM
   reference and a repeatable port-side test. Do not change the timestep based
   only on a constant from another event handler.
2. **P1 — Replace scripted hit detection with collision-driven gameplay.**
   Route Bullet contacts/overlaps through the game's collision/filter and
   entity-event path into `CCannonball::OnHit` and `CTowerCatapult::TakeDamage`;
   ensure a hit is applied once, and remove the test's hand-written AABB hit
   as the proof of gameplay collision.
   **Gate:** hit, miss, water/solid impact, damage, projectile removal and
   cleanup are covered without directly calling `OnHit` from the test.
3. **P1 — Cross-check player-ship behavior against the binary.** Trace the
   original control, movement, transform and spawn paths before replacing the
   placeholder's speed/turn constants and kinematic updates.
   **Gate:** controlled trajectories and model/transform behavior are compared
   under identical input and timestep; remaining differences are listed.
4. **P2 — Finish the game-state loop.** Complete pause/resume, mission score,
   level-complete/summary, return-to-map and save/load transitions already
   outlined below.
   **Gate:** an automated BOOT → level → completion → map run checks both
   state transitions and persisted state.
5. **P3 — Expand fidelity and release coverage.** Resolve missing global
   object factories, remaining levels, sound/video paths and reproducible
   Windows CI; keep the macOS port as its separate backend/ABI roadmap.
   **Gate:** each shipped level has a load/play/exit smoke test and known
   backend differences are documented.

The physics backend details and open mapping assumptions are also recorded in
[`docs/HAVOK_TO_BULLET.md`](./docs/HAVOK_TO_BULLET.md).

Until P0–P1 are closed, describe the current result as a working renderer and
gameplay prototype—not a behaviorally equivalent reconstruction.


## Milestone 1 — Playable single scene (VIKING01) 🎯

Goal: island, ship, ocean, 2 towers/fortresses and the enemy real chain; ESC → PAUSE → return.

1. **Player-ship movement prototype** (original `CPhysicsEntity::SetPosition` / `CShip::Update` behavior still needs cross-check); throttle/rudder → speed/angle. 🟡 `PlayerShip_Update` manually integrates the transform, prevents turning while stopped, clamps the hull inside the ocean grid and mirrors it to a kinematic Bullet body; `playground_test` verifies movement/bounds, not Bullet-driven movement or parity with the original. Island collision and the original ship model remain open.
2. **Cannon fire → Cannonball → collision → tower destruction** (at least `TowerCatapult` → `HealthBar` → 0 HP → destruction + sound); via `CollisionTrigger`. 🟡 Prototype: the Bullet body and scripted AABB-to-damage path are exercised by `playground_test`; the test explicitly calls `TakeDamage`/`OnHit`, so it does not verify real collision callbacks or ASM parity.
3. **HUD**: score countdown + HP bar + compass (`CGUIHealthBar` + `CGUIMap` + `MissionIndicator`; HUD at SCR.mov frame 9; the im2d overlay already exists). 🟡 The reference layout is drawn with the bottom-left ammo dial (GUI_022+GUI_012), wooden-tipped HP bar (GUI_026/024/028), fish counter (GUI_031), bottom-right compass (GUI_020 + needle + tower blips), score and tower count; it scales to the initialized raster and falls back per missing texture. GUI textures are loaded from the nested d3d8-IMAGE records at the tail of VIKING01.RWS chunk0 via `RwTxdDecodeGuiPack` (LevelPack cannot see these). Live HP is not connected because the player has no damage model, and no fish pickup producer currently calls `HUD_AddFish`; those gameplay integrations remain open. `KULEN n/2` under the compass; ESC→PAUSE is a separate item.
4. **PAUSE menu**: also open the menu in paused state (`CMenuBehavior` + `InputManager` with Esc). ✔ 2026-11-15: in-game ESC toggles PAUSE (edge-triggered) — world frozen (physics/ship input/ocean update/camera), dim overlay drawn over the frozen frame with parchment panel: `RESUME` (ESC) and `QUIT TO DESKTOP` (Enter). W/S or Up/Down navigate, Enter confirms, ESC resumes. `CANNONCRUISE_PAUSE_AT` / `CANNONCRUISE_SHOT_FRAME` env vars force the state for headless screenshot verification.
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
| 1a–1b | ~~D3D9 world static~~ (misdiagnosis) + Bullet step + placeholder/manual ship-movement prototype 🟡 | `physics_test` and `playground_test` green; no reference-trajectory parity claim |
| 1c | Scripted cannonball → tower damage prototype 🟡 | Headless AABB smoke test; real contact-driven hit is still P1 |
| 2 | HUD 🟡 (score + dial + HP bar + compass) + ESC→PAUSE ✔ | `playground_test` state assertions; live HP/fish producers remain open |
| 3 | Save/Load + LEVELCOMPLETE screen | `gamestate_test` full BOOT→COMPLETE |
| 4 | GLOBAL factories + VIKING02 + D3D9 package | 2 levels green; setup.exe works |

* Sprint 1's 3/3 prototype parts are exercised (Bullet step, placeholder ship control, scripted cannonball→tower). Real collision-driven tower damage remains P1; then continue HUD+PAUSE.*

* Sprint 1 prototype complete — placeholder ship physics + scripted cannonball→tower test → `ctest` 34/34.*

*2026-11-15: HUD redrawn against the original reference image — ammo dial (arm 175 cannonballs, GUI_022+GUI_012), red HP bar (wooden tips GUI_026/028 + rail GUI_024), fish counter (GUI_031), north-up compass (GUI_020 + white needle + red tower blips). UI textures are resolved from the nested chunk0 section of the level .RWS via `RwTxdDecodeGuiPack` → `RenderPlatform_UploadTexture` as menu textures (the HUD draws them). Also, overlay vertex colors were swapped in D3D9 (R↔B); the swizzle was fixed under `RW_D3D9`.*
