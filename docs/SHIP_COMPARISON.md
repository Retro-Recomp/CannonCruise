# Ship System — Binary vs Port Comparison (2026-11-14)

This document compares the ship sub-code of the CannonCruise.exe binary (`CC.asm`, `CC_ida.asm`) against the rewritten classes in this project. Content was manually verified through `CC_ida.asm`; the RWS contents were enumerated with the new `shipscan_test` tool.

## Finding: RWS ship assets verified

`.RWS` files were opened with `RwStudioStream`; it was verified that the ship assets are in `.dff` format. The working tree is clean.

### Test inventory (`shipscan_test.exe <file.RWS>`)

`tests/shipscan_test.cpp` + the `shipscan_test` target were added; it was seen that the ship assets are registered in every level pack.

### Ship assets found in all levels

| Asset | Levels | Type | Size |
|---|---|---|---|
| `Hugo_Ship.dff` | all | rwID_CLUMP | 172,940 B |
| `Hugo_CollHull_noinstance.dff` | all | rwID_CLUMP | 1,632 B |
| `Wheel_tag300.anm` | all | rwID_HANIMANIMATION | 2,608 B |
| `Rudder_tag400.anm` | all | rwID_HANIMANIMATION | 268 B |
| `Wing_Left_tag100.anm` / `Wing_Right_tag200.anm` | all | rwID_HANIMANIMATION | 1,348 B |
| `Gun_Barrel_tag600.anm` | all | rwID_HANIMANIMATION | 340 B |
| `Gun_Base_tag500.anm` | all | rwID_HANIMANIMATION | 556 B |
| `Viking_Destroyer.dff` | VIKING01–05 | rwID_CLUMP | 139,864 B |
| `Pirate_Destroyer.dff` | VIKING01,02,04,05 + BEAVER01–05 | rwID_CLUMP | 139,588 B |
| `Roman_Destroyer.dff` | BEAVER01–05 | rwID_CLUMP | 115,932 B |
| `Roman_Battleship.dff` | BEAVER01,04,05 | rwID_CLUMP | 266,460 B |
| `Viking_battleship.dff` | VIKING02,04,05 | rwID_CLUMP | 119,832 B |
| `Scylla_Destroyer.dff` | IDUNA01–03 + NORDIC02 + VIKING03 | rwID_CLUMP | 89,936 B |
| `Debris_Dark_02/03/04` + `Debris_Light_01/02` + `Debris_Mast_01` + `Debris_Paddle` + `Debris_Barrel` | all | rwID_CLUMP | ~1.5–3 KB |

### Hugo_Ship.dff internal structure

Read successfully with Librw `Clump::streamRead`:
- 8 atomics, 2,757 vertices, 3,533 tris, 1–2 materials (Hugo texture)
- 16-byte special preamble = `0x0002686 | 0x00000010 | 0x0002686-12 | 0x1803FFFF`
  ([u32 fileSize-6][u32 0x10][u32 fileSize-18][libid]) followed by standard
  RW 3.6 chunks: `STRUCT → FRAMELIST → GEOMETRYLIST → GEOMETRY(s) → ATOMIC(s)`

### Collision hull (`Hugo_CollHull_noinstance.dff`)

A Havok "CompressedMeshShape" binary; starts with the `Textures\0` marker.
Inside hangs a 3dsmax "User Properties" list (Mass, Friction, Elasticity,
Unyielding = 0, Simulation_Geometry = 2). Havok 2.2 binary structure; solving it
requires two-level analysis.

## What's missing in the port

### 🔴 1. Ship classes are never created at runtime

`WorldSpawn_FromFile` only registers 4 behaviors:
`CWbRenderBehaviour`, `CWbCameraBehavior`, `CAudioStreamEntity`,
`CPhysicsWorldBehaviour`. Ship is in none of the registered libraries.
No level .RWS contains a `CShipPlayer`/`CShipEnemy`/`CShip`
behavior record — ship spawning happens directly from the binary's level code
(e.g. `sub_4069A0` or similar), hard-coded C++.

**Required:** `CShip`, `CShipPlayer`, `CShipPlayerInputProxy`, `CShipEnemy` registrations
inside `GameBehaviours_Register()`; creation as real objects via
`InitShipWorld/LevelCode`; passing the clump with `CShip::Initialize(clump)`.

### 🔴 2. The real Hugo_Ship.dff is never loaded

`RwsPipeline_Load` only processes the `WorldGeometry` and `StaticCollisionData` assets.
The ship assets are listed under a specific contract:

```
[ASSETRECORD] rwID='rwID_CLUMP' src='X:\GFX\Ships\...\Hugo_Ship.dff' resource=172940
```

**Required:** a filter for the `rwID_CLUMP` assets inside `*.RWS` — the same decomposition path as with `WorldGeometry` can be used:
1. Read the payload with `RwStudioAssetReadResource`
2. Hidden `RpReadClump(stream)`-style stream open (`rw::Clump::streamRead`)
3. Pass it with `CShipPlayer::Initialize(clump)`

### 🔴 3. The `PlayerShip.cpp` placeholder is not real

In the screenshot, an `X` shape with rudder wings, a toy box; 13 vertices /
13 tris placeholder. The real Hugo model has 8 atomics, 2,757 tris, 2 materials.

**Required:** remove the placeholder and instantiate the real model with `RwsShip_LoadClump`.

### 🟠 4. `CShip::Initialize` / `CShipPlayer::UpdatePlayer` are only skeletons

Binary `FUN_004a7430` (CShip ctor), `FUN_004a76d0` (CShip::Update),
`FUN_004a7bc0` (CShip::TakeDamage) are fully running code; the port stubs.
CShipPlayer does not include the `sub_4AE700` (event wiring) and `sub_4AF3C0`
(per-frame) implementations.

### 🟠 5. `CShipEnemy::UpdateAI` does not exist, `FUN_004aa2e0` is entirely skipped

In parallel with 2000+ bytes of binary code: `sub_4AB200` (guard/protect + waypoint
search), `sub_4AB520` (no group = deals cycle), `sub_49C940` (HUD push),
`sub_4ACB40` (find/search HP), `sub_4AC920` (change state). In the port this function
pages are entirely missing.

### 🟠 6. `.anm` (HANIMANIMATION) is not parsed

`Wheel_tag300.anm`, `Rudder_tag400.anm`, `Wing_Left/Right_*.anm`, `Gun_*.anm`
are never read; there is no code that plays the
`Rudder`/`PADDLE` traps during the physics frames.

### 🟡 7. The collision hull is not solved

`Hugo_CollHull.dff` is a Havok binary; in Havok 2.2's `bhkCompressedMeshShape`
format (~1.6 KB); the textbook way to solve these is Havok 2.2 binary
decompiling VR or imitating a rock-capsule for a Bullet bot.

### 🟡 8. `CPhysicsWorld::Update` and `btDiscreteDynamicsWorld` are bound, but the Bullet step caller is not connected to CShip's loop

## Binary function addresses of interest (CShip behavior)

| Binary Name | Function |
|---|---|
| `sub_4A5F80` | CMsg apply (27-way switch; physics health) |
| `sub_4A99E0` | CShipEnemy HE iMsgRunningTick |
| `sub_4AB200` | Enemy AI main update |
| `sub_4AB520` | Group sync (burst/release) |
| `sub_4AC920` | AI state change (PATROL → HUNT) |
| `sub_4ACB40` | Find friendly ship |
| `sub_4AC740` | AI waiting for menu/selection |
| `sub_4A99E0` | Enemy weapon menu |
| `sub_4AE6C0` | CShipPlayer class registration (RWS `CClassFactory register`) |
| `sub_4AE700` | CShipPlayer ctor + event wiring |
| `sub_4AF3C0` | CShipPlayer per-frame |
| `sub_4B2FE0` | CShipPlayerInputProxy::Update (W/S/A/D) |

## Conclusion

While the port writes the `CShipPlayer`/`CShipEnemy` templates piece by piece, the binary
wires this directly into its own framework (e.g. the `EntityMessageProcessor`
target is a central system that makes the game loop call Tick). Current level:

- **Placeholder delivery rate**: the game window renders the island + objects,
  a player ship is not at world access — nothing appears as "ship" inside the game.
- **Playability**: the Paddle button's match is not hull-based; the brake/rudders are
  axis-by-axis independent.
- **Lack of debug evidence**: only class registrations were written for CShipPlayer/CShipEnemy;
  none of the behavior exists.
