# Havok → Bullet Migration Notes (Phase 4.6)

CannonCruisePC reverse engineering project — the portable **Bullet Physics 3.25**
was substituted for the Havok physics engine. This document collects in one place
**why**, **what** we changed and **how**, and the sources of the parameters
reverse engineered from the binary.

Related code:

| File | Role |
|---|---|
| `code/PhysicsBackend.h/.cpp` | Bullet wrapper (new — the backend itself) |
| `code/PhysicsWorld.cpp/.h` | `CPhysicsWorld`/`CPhysicsWorldBehaviour` — wrapper API bound to the backend (old names kept) |
| `code/CollisionGeometry.cpp/.h` | Clump → Bullet shape converter (`FUN_0046f040`) |
| `code/PhysicsEntity.cpp`, `CollisionTrigger.*` | thin wrappers (awaiting wiring) |
| `code/HavokMemoryManager.cpp`, `code/framework/startup/havokstart.cpp` | the game's Havok lifecycle port — **preserved** (the HavokInit call paths are on the game side; the arena is now standard malloc) |
| `tests/physics_test.cpp` | parameter validation test |

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
| +0x14 | 0x400 | capacity? | — (unused) |
| +0x20..28 | `0C3FA0000h` = **-500** | broadphase AABB min | documented gap (sec.3) |
| +0x30..38 | `43FA0000h` = **500** | broadphase AABB max | documented gap (sec.3) |
| +0x44 | `3F19999Ah` = **0.6** | friction | `btRigidBodyConstructionInfo.m_friction` |
| +0x48 | 1.0 | unknown (damping?) | — |
| +0x4C | 4 | unknown | — |
| +0x58 | `3E99999Ah` = **0.3** | restitution | `m_restitution` |
| +0x5C | `3A83126Fh` ≈ 0.0163 | ~1/61 — may be deactivation time | — |
| +0x6C/+0x88 | 0.2 | threshold (deactivation?) | — |
| +0x80 | 0x14 = **20** | solver iterations | `solverInfo.m_numIterations` |
| +0x8C | 10.0 | may be speed limit | — |

### 2.2 Fixed time step — 1/60

- `CPhysicsWorld::Update` (`FUN_0043a040`) in the binary calls
  `hkpWorld::stepDeltaTime`; **the constant 1/60 s** is passed everywhere in the
  game code (`PhysicsWorld.h` contract, `HandleEvents`).
- Independent evidence: `CSoldier::HandleEvents` (`sub_4B8710+7E`)
  makes the physics call with `push 3C888889h` (=1/60).
- Backend: `stepSimulation(dt, /*maxSubSteps=*/1, /*fixedTimeStep=*/dt)`
  — exactly one fixed step per call (imitating hkpWorld::stepDeltaTime).

### 2.3 World creation chain

- `CPhysicsWorldBehaviour` ctor (`sub_43B860` → RWS class record
  `sub_43B9A0`, string "CPhysicsWorldBehaviour", size 0x20) sets up the game's
  world singleton; HavokInit (`sub_434FB0`) prepares the arena/context.
  On the port side: behaviour ctor → `CPhysicsWorld` ctor →
  `PhysicsWorld_Create()` (gravity/iter with the values above).

## 3. Havok → Bullet mapping table

| Havok (binary/API) | Bullet (backend) | Note |
|---|---|---|
| `hkpWorld` | `btDiscreteDynamicsWorld` | config+dispatcher+dbvt broadphase+sequential impulse solver |
| `hkpWorld::stepDeltaTime(dt)` | `stepSimulation(dt,1,dt)` | fixed step |
| `hkpRigidBody` (+Cinfo) | `btRigidBody` | mass/friction/restitution from Cinfo |
| `hkpBoxShape` | `btBoxShape` | half extents |
| `hkpSphereShape` | `btSphereShape` | |
| `hkpConvexVerticesShape` (DFF collision) | `btConvexHullShape` | `bConvex=TRUE` path |
| `hkpMeshShape`/BSP? | `btBvhTriangleMeshShape` | static world; BSP risk documented |
| `hkpPhantom` (trigger) | `btGhostObject` | CF_NO_CONTACT_RESPONSE; **`btGhostPairCallback` registration required** (otherwise the overlap list stays empty — found via test) |
| `hkpConstraintSolver` default | `btSequentialImpulseConstraintSolver` | iter=20 |
| Havok memory arena (dword_660920/sub_52DC80, 0x4000, align16) | standard `new`/`malloc` | the arena is gone; the `HavokMemoryManager` game-side API was kept but the backend does not allocate |
| `hkpWorldCinfo::m_broadPhaseBounds` ±500 | (not applied) | Bullet uses a wide aabb — documented deviation |

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
3. **`PhysicsWorld.cpp`** — the ctor creates the world; `Update` →
   `PhysicsWorld_Step`; Add/Remove RigidBody/Phantom → delegated to the backend
   (the vector record was kept for `RigidBodyExistsInWorld`); dtor
   `PhysicsWorld_Destroy`.
4. **`CollisionGeometry.cpp`** — real implementation of `CreateShapeFromClump`:
   `RpClumpForAllAtomics` → vertex/triangle collection
   (atomic-based base offset), `rpGEOMETRYTRISTRIP` → alternating winding,
   `bConvex` → ConvexHull / otherwise TriMesh. `CreateBoxShape`/
   `CreateSphereShape`/`DestroyShape` delegated to the backend.
5. **`rpworld.h` + `rwstubs.cpp`** — vertex access API added (stub:
   0/NULL; **real data will flow with librw integration**):
   `RpGeometryGetNumVertices/NumTriangles/Vertices/TriangleIndices/Flags`.
6. **`tests/physics_test.cpp` (new)** + `tests/CMakeLists.txt` —
   the `physics_test` target (Bullet linked).
7. **`FUN_MAP.md`** — the `FUN_0043a040/43aa20/43ae30/43b9a0/46f040`
   lines were updated to the backend; `FUN_0055e6d0` added.
8. **`ROADMAP.md`** — the Phase 4.6 block added.

## 5. Test coverage (`physics_test`, 7/7)

| Test | Verified |
|---|---|
| TestGravityDefault | default gravity (0,-9.8,0) |
| TestFreeFall | 1 s = 60×(1/60): v≈-9.8, semi-implicit Euler position match |
| TestGroundRestAndFriction | resting band on the ground; vx=5 → stop with friction 0.6 |
| TestImpulse | central impulse → Δv = J/m (12) |
| TestTriggerOverlap | ghost trigger overlap (1 hit, correct body) |
| TestTriMeshStatic | box resting on a triangle mesh ground |

Running:

```sh
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release   # first setup downloads Bullet
cmake --build build -j8
ctest --test-dir build --output-on-failure  # 7/7
./build/CannonCruise                        # link verification OK
```

## 6. Documented deviations and open gaps

- **Solver**: sequential-impulse (Bullet) ≠ Havok 2004 solver — one-to-one
  feel is not guaranteed; critical values were taken from the binary, fine calibration
  should be done with assets (ship weight, water thrust).
- **AABB ±500**: the broadphase clamp was not applied (Bullet wide aabb);
  it can be added as an in-game clamp when needed.
- **Clump atomic transform**: `CreateShapeFromClump` does not apply the atomic frame
  matrix (noinstance/single atomic assumption) — wrong positioning in multi-atomic
  DFFs.
- **Deactivation fields**: `sub_55E6D0` +0x5C/+0x6C/+0x88 (0.0163/0.2/0.2)
  are not yet mapped to Bullet deactivation settings; bodies currently use
  `DISABLE_DEACTIVATION` (always active — deviation from the original, deliberate
  for stability).
- **BSP/BSPWorld**: the binary uses `RpWorldStreamRead` + game plugin chunks;
  Bullet has no BSP convexification — revisit when assets arrive.
- **Entity wiring**: `CPhysicsEntity` (FUN_00436480/437410/437c60)
  is still not bound to the backend (next work item).
- **HavokInit**: the `sub_434FB0` lifecycle was ported (havokstart.cpp) but
  the physics world singleton wiring is homework.
- **Sound `.rws` / Rt2dMaestro**: outside physics — librw work (see ROADMAP).

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
   (wbID_STATICWORLDCOLLISION; IDA `sub_439460`) — ~~counter/assert ported;
   stream parse~~ **rwstudio container parse active** (Phase 4.6e —
   finds the `.col` record of RwStudioStream). Remaining: .col inline data layout
   → `PhysicsShape_CreateTriMesh`.
5. ~~Gate 4: real `.rws` asset + librw integration~~ **opened**
   (Phase 4.6d/4.6e): librw NULL platform engine + StreamFile shim
   (`rw_librw.cpp`), `rwlibrw_test` + **rwstudio container parser**
   (`RwStudioStream`) inventories real `Streams/*.RWS` files
   (MAINMENU 5 assets/4 objects, GLOBAL 7/48, VIKING01 119/497; the embedded txd
   payload was verified — ctest 10/10). Remaining: render platform (GL3/D3D9)
   + scene-in-scene test; txd payload decode (d3d8 IMAGE) and the 0x0704
   property schema; then `RpGeometryGetVertices`/
   `RpClumpGetBoundingSphere` will return real data.
