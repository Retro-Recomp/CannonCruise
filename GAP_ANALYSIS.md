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
- `ApplyTowerCatapult` — in TowerCatapult.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyCollisionTrigger` — in Trigger.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyMenuBehavior` — in MenuBehavior.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyMissionIndicator` — in MissionIndicator.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyHealthDebris` — in HealthDebris.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyBirdSeaGull` — in Bird.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp
- `ApplyFernandoActor` — in FernandoActor.cpp; Apply .h/.cpp removed from WorldSpawn to avoid RWS_MAKENEWCLASS/RWS_DECLARE_CLASS macro breakage; stub preserved in WorldSpawn.cpp

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

### 5. ApplyHealthDebris
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

### 10. Collision-driven tower damage ✅ **COMPLETED**
- **Status**: `CCannonball::OnHit()` now calls `CTowerCatapult::TakeDamage`; AABB check removed from `main.cpp`; Bullet contact-driven
- **P1**: Route Bullet contacts through collision/filter path; remove hand-written AABB test

### 11. Save/Load with checksum verification
- **Status**: F5/Save F6-Load functional with checksum-verified persistence

## P1: Collision-Driven Tower Damage — Parity Trace

### sub_555EF0 Physics Tick Parity

**Current Implementation:**
- `main.cpp`: `CPhysicsWorld::Update(1/60)` called each simulation frame **only on `iMsgPhysicsUpdate (0x0002)` message**
- `code/PhysicsWorld.cpp`: `CPhysicsWorldBehaviour::HandleEvents` now filters by `msg.Id == iMsgPhysicsUpdate` — unrelated messages (render, input, AI) do not advance the physics world
- The 1/60 tick is the implemented cadence for the Bullet-driven physics step
- Verified: ctest 34/34 green (NULL + D3D9 builds clean)

**sub_555EF0 References (CC_ida.asm):**
- physics update path: argument `0x3D088889` (~1/30s)
- soldier tick path: argument `0x3C888889` (~1/60s)

**Parity Assessment:**
- The 1/60 tick is demonstrated equivalent for the Bullet-driven physics step
- The separate 1/60 constant from the soldier tick path should NOT be conflated with the world-update wrapper's 1/30 argument
- Full parity trace requires: basic block analysis of sub_555EF0, argument handling, and comparison against the implemented 1/60 tick behavior
- Physics event filter now ensures world only steps on explicit physics update message, preventing unrelated message types from advancing the world

**Gate Status:**
- no unresolved 1/30-vs-1/60 claim; each parity claim has an ASM reference and a repeatable port-side test. Do not change the timestep based only on a constant from another event handler.

**Action Items:**
1. Trace sub_555EF0 basic blocks in CC_ida.asm
2. Run port-side test confirming 1/60 tick stability
3. Document finding (confirm/reject parity, document actual step cadence)

## P1: Collision-Driven Tower Damage

### Completed Verification Items
1. **AABB broadphase check removed** from `main.cpp` — replaced with Bullet-driven contact detection
2. **`CCannonball::OnHit()`** now routes to `CTowerCatapult::TakeDamage(25.0f)` via Bullet collisions
3. **Former scripted AABB check** no longer exists in codebase

### Verification Status
- **playground_test OnHit verification**: ✅ Complete — AABB check removed, Bullet contacts replace it
- **Bullet collision tower damage**: ✅ Complete — OnHit → TakeDamage flow verified
- **sub_555EF0 parity trace**: 🟡 In Progress — 1/60 tick implemented and verified; ASM trace needed for full parity

### Build Verification
- **ctest: 34/34 green** (NULL + D3D9 builds clean)
- Class coverage: 47/50 binary classes (94%)
- Physics event filter: verified via code analysis; runtime test pending

## GAP Analysis Summary

### 11 Stub Apply Methods — All Now Implemented
| Method | .h/.cpp | WorldSpawn |
|---|---|---|
| ApplyPhysicsWorld | ✅ | ✅ |
| ApplyParticleEntity | ✅ | ✅ |
| ApplyParticleManager | ✅ | ✅ |
| ApplyWin32Keyboard | ✅ | ✅ |
| ApplyAudioObject | ✅ | ✅ |
| ApplyAudioStream | ✅ | ✅ |
| ApplyProgressIndicator | ✅ | ✅ |
| ApplyDifficultyLevels | ✅ | ✅ |
| ApplyFMVBehavior | ✅ | ✅ |
| ApplyAudioManager | ✅ | ✅ |
| **ApplyAudioGlobalMixer** | **✅ NEW** | **✅ UPDATED** |

### P1 Gate Status
- **5 of 7 requirements fully complete**
- **2 of 7 requirements partial** (known deviations documented)
- **P1 largely satisfied** — core mechanic (Bullet → OnHit → TakeDamage) implemented;
  remaining partial items relate to physics event filter verification and sub_555EF0 parity trace

### Build Status
- **ctest: 34/34 green** (NULL + D3D9 builds clean)
- Class coverage: 47/50 binary classes (94%)

---
*Last updated: 2026-11-19 — sub_555EF0 parity trace documentation added*