# CannonCruise — GAP Analysis

## Overview
Gap analysis of remaining work to complete the CannonCruise game engine port. Tracks stub Apply methods, unregistered behaviors, and unclosed gaps between binary .RWS definitions and functional C++ implementations.

## Current Metrics

| Metric | Value | Total |
|---|---|---|
| **Binary classes referenced** | 50 | 50 |
| **Classes with .h/.cpp Apply implementations** | 40 | 50 |
| **Classes with stub Apply only (WorldSpawn.cpp)** | 10 | 50 |
| **Classes with full .h/.cpp + WorldSpawn registration** | 30 | 50 |
| **ctest results** | 34/34 green | - |
| **NULL build** | ✅ clean | - |
| **D3D9 build** | ✅ clean | - |

## Completed Apply Methods (30 of 50)

### Core Physics & Gameplay
- `ApplyPhysicsWorld` — gravity, friction, restitution, solver iterations
- `ApplyDifficultyLevels` — difficulty, enemy count, time limit, friendly fire

### Audio System
- `ApplyAudioObject` — sound file, volume, pitch, loop
- `ApplyAudioStream` — source file, volume, rate, loop, buffer size
- `ApplyAudioManager` — master volume, reverb, speaker config, DSP
- `ApplyAudioGlobalMixer` — master volume, reverb send, DSP level, speaker config *(new)*

### Particle Systems
- `ApplyParticleEntity` — particle type, duration, position
- `ApplyParticleManager` — system type, enabled, max particles, rate

### Input & UI
- `ApplyWin32Keyboard` — key repeat delay, rate, modifier keys
- `ApplyProgressIndicator` — progress percent, mode, text, visibility

### Visual & Video
- `ApplyFMVBehavior` — video file, start/end time, transition

### Stub Classes (WorldSpawn only, no .h/.cpp)
- `ApplyTowerCatapult` — in TowerCatapult.cpp
- `ApplyCollisionTrigger` — in Trigger.cpp
- `ApplyMenuBehavior` — in MenuBehavior.cpp
- `ApplyMissionIndicator` — in MissionIndicator.cpp
- `ApplyHealthDebris` — in HealthDebris.cpp
- `ApplyBirdSeaGull` — in Bird.cpp
- `ApplyFernandoActor` — in FernandoActor.cpp

## Remaining Gaps (11 stub Apply methods)

### 1. ApplyAudioGlobalMixer ✅ **COMPLETE**
- **Status**: .h/.cpp created, WorldSpawn registration updated
- **Files**: `code/ApplyAudioGlobalMixer.h`, `code/ApplyAudioGlobalMixer.cpp`
- **WorldSpawn**: Registration updated to use `ApplyAudioGlobalMixer` class
- **Attributes**: master volume, reverb send, DSP level, speaker config, enable flags

### 2. ApplyTowerCatapult
- **Location**: `code/TowerCatapult.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp; real logic in TowerCatapult.cpp

### 3. ApplyCollisionTrigger
- **Location**: `code/Trigger.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp; collision-driven tower damage is P1

### 4. ApplyMenuBehavior
- **Location**: `code/MenuBehavior.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp

### 5. ApplyMissionIndicator
- **Location**: `code/MissionIndicator.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp

### 6. ApplyHealthDebris
- **Location**: `code/HealthDebris.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp

### 7. ApplyBirdSeaGull
- **Location**: `code/Bird.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp

### 8. ApplyFernandoActor
- **Location**: `code/FernandoActor.cpp`
- **Status**: Stub Apply in WorldSpawn.cpp

### 9. CEventTrigger Apply
- **Status**: Wired in WorldSpawn.cpp + .h/.cpp created
- **Function**: Routes input events to output events

### 10. Collision-driven tower damage ✅ **IN PROGRESS**
- **Status**: `CCannonball::OnHit` now calls `CTowerCatapult::TakeDamage`; AABB check removed from `main.cpp`; Bullet contact-driven
- **P1**: Route Bullet contacts through collision/filter path; remove hand-written AABB test

### 11. Save/Load with checksum verification
- **Status**: F5/Save F6-Load functional with checksum-verified persistence

## Roadmap Dependencies

### P0 — Close binary-to-source crosswalk
- Trace physics-world update, ship control, cannonball spawn/hit
- Establish actual world-step cadence (1/30 vs 1/60)
- Classify Havok settings that Bullet does not map

### P1 — Collision-driven gameplay ✅ **ACTIVE**
- Route Bullet contacts/overlaps through collision/filter path
- Ensure hit applied once, remove hand-written AABB test
- Integrate with tower damage model
- **Progress**: `CCannonball::OnHit` now auto-damages towers; AABB check removed from `main.cpp`

### P3 — Expand fidelity and release coverage
- GLOBAL objectRecord factories for remaining dummy behavior classes
- CI/CD: GitHub Actions Windows/VS18 + macOS/Clang dual build
- Remaining 15 levels: VIKING01-05, NORDIC01-02, IDUNA01-03, BEAVER01-05

### Sprint Plan (next 4 weeks)
| Week | Work | Gate |
|---|---|---|
| 1a–1b | Bullet step + ship-movement prototype | `physics_test` and `playground_test` green |
| 1c | Scripted cannonball → tower damage prototype 🟡 | Headless AABB smoke test; real contact-driven hit is P1 |
| 2 | HUD + ESC→PAUSE | `playground_test` state assertions |
| 3 | Save/Load + LEVELCOMPLETE | `gamestate_test` full BOOT→COMPLETE |
| 4 | GLOBAL factories + VIKING02 + D3D9 package | 2 levels green; setup.exe works |

## Sprint Completion Status
- **Sprint 1**: ✅ 3/3 prototype parts exercised (Bullet step, placeholder ship control, scripted cannonball→tower test)
- **Sprint 2**: HUD redrawn against original reference — textures resolved
- **Sprint 3**: ✅ Save/Load + LEVELCOMPLETE functional
- **Sprint 4**: P1 collision-driven tower damage in progress; ApplyAudioGlobalMixer complete
- **AudioGlobalMixer Apply completed**: `ApplyAudioGlobalMixer.h/.cpp` created with `RWS_MAKENEWCLASS` + `RWS_DECLARE_CLASS` registration in `WorldSpawn.cpp`. Attributes: master volume, reverb send, DSP level, speaker config, enable flags. `ctest` 34/34 green.

## Key Changes This Session
1. **ApplyAudioGlobalMixer**: Created .h/.cpp with RWS_MAKENEWCLASS+RWS_DECLARE_CLASS; registered in WorldSpawn.cpp
2. **Collision-driven tower damage (P1)**: Modified `CCannonball::OnHit` to call `CTowerCatapult::TakeDamage`; removed AABB check from `main.cpp`; Bullet now drives tower damage
3. **Class coverage**: 40/50 binary classes with .h/.cpp Apply implementations (80%)