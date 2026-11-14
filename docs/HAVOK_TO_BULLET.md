# Havok → Bullet Migration Notes (Phase 4.6)

CannonCruisePC reverse engineering project — the portable **Bullet Physics 3.25**
was substituted for the Havok physics engine. This document collects in one place
**why**, **what** we changed and **how**, and the sources of the parameters
reverse engineered from the binary.

Related code:

| File | Role |
|---|---|
| `code/PhysicsBackend.h/.cpp` | Bullet wrapper, shape/body lifetime and input validation |
| `code/PhysicsWorld.cpp/.h` | `CPhysicsWorld`/`CPhysicsWorldBehaviour` — wrapper API bound to the backend (old names kept) |
| `code/CollisionGeometry.cpp/.h` | Clump → Bullet shape conversion path; RenderWare geometry accessors are still stubs |
| `code/PhysicsEntity.cpp`, `CollisionTrigger.*` | backend-backed body/trigger wrappers; behavior parity remains partial |
| `code/HavokMemoryManager.cpp`, `code/framework/startup/havokstart.cpp` | compatibility lifecycle/allocator code; Bullet backend objects use ordinary C++ allocation |
| `tests/physics_test.cpp` | backend behavior and invalid-input regression tests |

---

## 1. Why Bullet?

- The original engine is **Havok Physics 2004/2005** (code name "Havok Animation +
  Physics", MSVC4.2b static library). The SDK is closed/inaccessible; the repo only
  contains the game's own abstraction layer (`void*` wrappers for `hkpWorld` etc.)
  and link stubs (similar to `rwstubs.cpp`).
- Alternatives compared:
  - **Jolt** — modern and fast, but requires C++17; it does not match the project's
    C++14 standard.
  - **PhysX** — license/assembly cost and installation weight.
  - **Bullet 3.25** — MIT, C++14 compatible, common in the D3D9/OpenGL era
    (closest ecosystem for 2004-2010 game ports), lightweight
    (only LinearMath/Collision/Dynamics are compiled).

## 2. Parameters reverse engineered from the binary ("closest feel")

User directive: *first the binary's actual physics settings will be found,
then the engine will be bound with those values.* No `hkp*`/`hka*` symbols exist in
the binary (static link, no PDB) — the values were found by content analysis.

### 2.1 Game physics settings struct — `sub_55E6D0`

The Havok memory callback registration `sub_55E630` registers this function to
dword_660D58; `sub_55E680` allocates a 0x260-byte struct over dword_660920
(Havok arena context — the same as havokstart.cpp). `sub_55E6D0` fills the struct
as follows:

| Offset | Binary value | Meaning | Bullet equivalent |
|---|---|---|---|
| +0x04 | `0C11CCCCDh` = **-9.8** | gravity.y | `world->setGravity(0,-9.8,0)` |
| +0x10 | 1 | unknown byte field | — |
| +0x14 | 0x400 | unknown integer field | — |
| +0x20..28 | `0C3FA0000h` = **-500** | broadphase AABB min | documented gap (sec.3) |
| +0x30..38 | `43FA0000h` = **500** | broadphase AABB max | documented gap (sec.3) |
| +0x44 | `3F19999Ah` = **0.6** | friction | `btRigidBodyConstructionInfo.m_friction` |
| +0x48 | 1.0 | unknown | — |
| +0x4C | 4 | unknown | — |
| +0x54 | `BCF5C28Fh` = **-0.03** | unknown | — |
| +0x58 | `3E99999Ah` = **0.3** | restitution | `m_restitution` |
| +0x5C | `3A83126Fh` = **0.001** | unknown (previous 0.0163 note was incorrect) | — |
| +0x60 | `3DCCCCCDh` = **0.1** | unknown | — |
| +0x64 | 1 (byte) | unknown | — |
| +0x68 | `3D4CCCCDh` = **0.05** | unknown | — |
| +0x6C/+0x88 | `3E4CCCCDh` = **0.2** | unknown; semantic label unverified | — |
| +0x70/+0x7C | `3C23D70Ah` = **0.01** | unknown | — |
| +0x74 | `3F7FBE77h` ≈ **0.999** | unknown | — |
| +0x78/+0x84 | 1 (byte) | unknown | — |
| +0x80 | 0x14 = **20** | solver iterations | `solverInfo.m_numIterations` |
| +0x8C | `41200000h` = **10.0** | unknown | — |

Only the gravity, friction, restitution and solver iteration fields have
confident Bullet mappings. The other values are recorded as observed data;
their meanings must not be inferred from proximity to a familiar Havok setting.

### 2.2 Update flow and cadence

- IDA's `sub_43A1C0` (Ghidra `FUN_0043a1c0`) passes `0x3D088889` (about 1/30)
  to `sub_555EF0`, then calls `nullsub_11`. Pseudo-C confirms the literal
  `0.033333335` (`CannonCruise.exe.c`, `FUN_0043a1c0`).
- `sub_555EF0` forms a pair from the supplied `dt` and `1.0/dt`, then calls
  `sub_557010` followed by `sub_5572E0`. Those routines traverse simulation
  object collections and callback/cleanup paths. This establishes the call
  chain, but not that one call is exactly equivalent to one
  `hkpWorld::stepDeltaTime(dt)` operation.
- The original event handler `sub_43BB10` compares the incoming message ID with
  `dword_635C68` and calls `sub_43A1C0` only on a match. The port's
  `CPhysicsWorldBehaviour::HandleEvents` currently steps on every delivered
  message, while the runtime loop also calls `Update(1/60)` directly. This
  differs from the observed filter and can advance the port world more than
  once during a frame.
- `CSoldier::HandleEvents` (`sub_4B8710`) separately passes `0x3C888889`
  (about 1/60) to `loc_466040`. That is a different tick path and is not
  independent proof of the physics-world timestep.
- The port currently calls `CPhysicsWorld::Update(1/60)` from its runtime loop;
  Bullet requests one step for that supplied value. The current physics test
  verifies port behavior, not timing parity with the original.
- **Next:** identify the matching message's delivery cadence and trace the
  relevant simulation callees before changing the port's timestep or claiming
  parity.

### 2.3 World creation chain

- Startup creates the original world singleton through `sub_41A680` after the
  Havok initialization path. The `CPhysicsWorldBehaviour` constructor
  (`sub_43B860`, class record `sub_43B9A0`) registers the behavior; it does not
  establish the original world's update cadence. On the port side,
  `CPhysicsWorld` constructs the Bullet world; a standalone behavior/test path
  may create and own a world if the startup singleton is absent.

## 3. Havok → Bullet mapping table

| Havok (binary/API) | Bullet (backend) | Note |
|---|---|---|
| `hkpWorld` | `btDiscreteDynamicsWorld` | config+dispatcher+dbvt broadphase+sequential impulse solver |
| Original update chain (`sub_43A1C0` → `sub_555EF0` → `sub_557010`/`sub_5572E0`) | `stepSimulation(dt,1,dt)` | Intended one-step mapping; exact operation and cadence parity remain unconfirmed. |
| `hkpRigidBody` (+Cinfo) | `btRigidBody` | mass/friction/restitution from Cinfo |
| `hkpBoxShape` | `btBoxShape` | half extents |
| `hkpSphereShape` | `btSphereShape` | |
| `hkpConvexVerticesShape` (DFF collision) | `btConvexHullShape` | `bConvex=TRUE` path |
| `hkpMeshShape`/BSP? | `btBvhTriangleMeshShape` | static world; BSP risk documented |
| `hkpPhantom` (trigger) | `btGhostObject` | CF_NO_CONTACT_RESPONSE; **`btGhostPairCallback` registration required** (otherwise the overlap list stays empty — found via test) |
| `hkpConstraintSolver` default | `btSequentialImpulseConstraintSolver` | iter=20 |
| Havok memory arena (dword_660920/sub_52DC80, 0x4000, align16) | standard C++ allocation | Bullet objects do not use the Havok arena; the game-side compatibility API remains separate. |
| World settings contain AABB values ±500 | (not applied) | `btDbvtBroadphase` does not enforce these limits; the original field's runtime effect is not yet established. |

These are intended architectural correspondences, not proof of one-to-one
construction-call parity. In particular, body mass/flags, shape selection,
collision filters and per-shape material settings still need call-site
cross-checks against the original.

## 4. Code changes (what we did)

1. **`code/CMakeLists.txt`** — Bullet FetchContent (tag 3.25, shallow);
   demo/unit-test/OpenGL options off; only three sublibraries.
   Notes:
   - Bullet includes do not propagate INTERFACE to targets →
     `target_include_directories(... ${bullet3_SOURCE_DIR}/src)` added.
   - Bullet's old `cmake_minimum_required` requirement breaks on CMake 4.x →
     `CMAKE_POLICY_VERSION_MINIMUM 3.5`.
2. **`PhysicsBackend.h/.cpp` (new)** — the backend wrapper:
   - World: `PhysicsWorld_Create/Destroy/Step/Set·GetGravity`.
   - Body: `PhysicsBody_CreateBox` (half-extent Cinfo),
     `PhysicsBody_CreateTriMesh` (static mesh — MeshBundle),
     `PhysicsBody_CreateWithShape` (with external shape), Destroy.
   - Shape: `PhysicsShape_CreateBox/Sphere/ConvexHull/TriMesh` +
     `PhysicsShape_Destroy` (ShapeBundle registry — mesh lifetime tied to shape).
   - Trigger: `PhysicsTrigger_CreateBox` (ghost) + Add/RemoveTrigger
     (SensorTrigger group) + `PhysicsTrigger_GetOverlaps`.
   - State: position/rotation (column-major 3x3)/velocity/kinematic/impulse.
   - **Ownership contract**: `CreateBox/CreateTriMesh` bodies own their own
     shape (Destroy handles it automatically); `CreateWithShape`
     does not own (`userIndex=1` tag) — the caller then calls
     `PhysicsShape_Destroy`.
   - MeshBundle separation via a registry table (because `void*` can be both
     `btRigidBody*` and `MeshBundle*`).
   - Creation validates finite positions/vertices, positive dimensions,
     triangle counts and indices, and vertex stride. A zero rotation matrix is
     treated as identity; null velocity reads leave outputs unchanged.
3. **`PhysicsWorld.cpp`** — the ctor creates the world; `Update` →
   `PhysicsWorld_Step`; Add/Remove RigidBody/Phantom → delegated to the backend
   (the vector record was kept for `RigidBodyExistsInWorld`); dtor
   `PhysicsWorld_Destroy`.
4. **`CollisionGeometry.cpp`** — `CreateShapeFromClump` conversion path:
   `RpClumpForAllAtomics` → vertex/triangle collection
   (atomic-based base offset), `rpGEOMETRYTRISTRIP` → alternating winding,
   `bConvex` → ConvexHull / otherwise TriMesh. `CreateBoxShape`/
   `CreateSphereShape`/`DestroyShape` delegate to the backend. The current
   RenderWare geometry accessors return 0/NULL, so real clump conversion remains
   blocked; atomic frame transforms are also not applied.
5. **`rpworld.h` + `rwstubs.cpp`** — vertex access API declarations exist, but
   the current `rwstubs.cpp` implementations return 0/NULL:
   `RpGeometryGetNumVertices/NumTriangles/Vertices/TriangleIndices/Flags`.
   Consequently, the clump-to-shape path cannot yet consume real geometry
   through these accessors.
6. **`tests/physics_test.cpp` (new)** + `tests/CMakeLists.txt` —
   the `physics_test` target (Bullet linked).
7. **`FUN_MAP.md`** — the `FUN_0043a1c0/43aa20/43ae30/43b9a0/46f040`
   lines were updated to the backend; `FUN_0055e6d0` added.
8. **`ROADMAP.md`** — the Phase 4.6 block added.

## 5. Test coverage (`physics_test`, 7 groups / 40 checks)

| Test | Verified |
|---|---|
| TestGravityDefault | default gravity (0,-9.8,0) |
| TestFreeFall | 1 s = 60×(1/60): v≈-9.8, semi-implicit Euler position match |
| TestGroundRestAndFriction | resting band on the ground; vx=5 → stop with friction 0.6 |
| TestImpulse | central impulse → Δv = J/m (12) |
| TestTriggerOverlap | ghost trigger overlap (1 hit, correct body) |
| TestTriMeshStatic | box resting on a triangle mesh ground |
| TestInvalidInputs | rejects malformed dimensions/mesh data; null velocity read is safe; zero rotation is identity |

Running:

```sh
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release   # first setup downloads Bullet
cmake --build build -j8
ctest --test-dir build --output-on-failure  # full configured test suite
ctest --test-dir build -R "^physics_test$" --output-on-failure  # physics_test reports 40 checks
./build/CannonCruise                        # platform-dependent executable/target
```

## 6. Documented deviations and open gaps

- **Solver**: sequential-impulse (Bullet) ≠ Havok 2004 solver — one-to-one
  feel is not guaranteed; critical values were taken from the binary, fine calibration
  should be done with assets (ship weight, water thrust).
- **AABB ±500**: the broadphase clamp was not applied (Bullet wide aabb);
  the original values are known, but their effect is not. Do not add a scene
  clamp until that behavior is confirmed; the current scene includes
  coordinates outside ±500.
- **Clump atomic transform**: `CreateShapeFromClump` does not apply the atomic frame
  matrix (noinstance/single atomic assumption) — wrong positioning in multi-atomic
  DFFs.
- **Clump geometry input**: `RpGeometryGetNumVertices/NumTriangles/Vertices/
  TriangleIndices/Flags` in `rwstubs.cpp` still return 0/NULL, so the DFF
  conversion path has no real geometry until these functions are connected to
  librw. This is separate from the inline `.col` static-collision path.
- **Unmapped world fields**: `sub_55E6D0` +0x5C/+0x6C/+0x88 contain
  0.001/0.2/0.2, but their meanings are unknown. Bodies currently use
  `DISABLE_DEACTIVATION` (always active — a deliberate stability choice, not
  evidence of original behavior).
- **BSP/BSPWorld**: the binary uses `RpWorldStreamRead` + game plugin chunks;
  Bullet has no BSP convexification — revisit when assets arrive.
- **Entity/event parity**: `CPhysicsEntity` and `CollisionTrigger` are wired to
  the backend, but `CPhysicsEntity::HandleEvents` is empty and the world's
  message filter differs from `sub_43BB10`; direct runtime stepping may also
  cause extra simulation steps.
- **Static collision resource**: `CPhysicsResource::Load` still does not decode
  the inline `.col` payload and create/register its triangle-mesh shape.
- **Havok allocation parity**: the game-side lifecycle shim remains, while
  Bullet's objects use ordinary C++ allocation rather than the original arena.

## 7. What's next?

1. ~~`CPhysicsEntity` → body factory wiring~~ **done** (Phase 4.6b):
   accessors bound to the backend; AddToWorld singleton → backend record;
   test `physics_entity_test`.
2. ~~Map the `CollisionTrigger` flow to phantoms~~ **done** (Phase 4.6b):
   Initialize (clump sphere stub → AABB ghost) + PollOverlaps
   enter/leave flow.
3. ~~World singleton wiring in HavokInit~~ **done** (Phase 4.6c):
   in the binary the singleton is created by the startup init (`sub_41A680`), used by the
   behaviour — the port aligned with `CPhysicsWorld::Create()` + the `m_bOwnsWorld`
   ownership flag.
4. `CPhysicsResource::Load` → static world trimesh
   (wbID_STATICWORLDCOLLISION; IDA `sub_439460`): decode the inline `.col`
   payload and pass the validated mesh to `PhysicsShape_CreateTriMesh`.
5. Connect the RenderWare geometry accessors to real librw data so
   `CreateShapeFromClump` can build non-empty DFF collision shapes.
6. Continue runtime parity work from `ROADMAP_NEXT.md`; keep renderer, asset,
   and gameplay milestones separate from Havok-to-Bullet claims.
