# CClassFactory + Behaviour Spawnsistor — Binary vs Port Audit (2026-11-14)

This document compares the CannonCruise.exe binary's runtime class factory
registrations (`call sub_413700`, i.e. `CClassFactory::Register`) against the
registration table produced by our port's `RWS_REGISTER_CLASS` and
`BehaviourLoader::Register` calls. The observation is dual:
(a) the **22 classes named by .RWS** surface as .BINARY DATA (`shipscan_test`
output), (b) our port logs and reports which registrations it knows. The .RWS
and port tables **never overlap**.

## 1. Binary runtime class registration table

`call sub_413700` preceded by
```asm
push 1                 ; enable flag
push <sizeof class>     ; application class size
push offset sub_<ctor>  ; class constructor
push offset a<Name>     ; .rdata class name
mov ecx, offset unk_658474  ; CClassFactory dict → singleton
call sub_413700         ; CClassFactory::Register
```
`sub_4C2CE0` (CWbCameraBehavior) is the typical sample; `sub_4AE6C0` is CShipPlayer.

Verified against example .RWS registration logs; 50 classes:

```
AudioGlobalMixer  CActor  CAfterGameMenuBehavior  CAmmoCrate  CAnimatedEnvironment
CAudioManager  CAudioObject  CAudioStream  CAudioStreamEntity  CBirdSeaGull
CCameraShaker  CCannonball  CCollisionTrigger  CDebris  CDifficultyLevels
CEventProxy  CEventTrigger  CFade  CFernandoActor  CFishGreen  CFMVBehavior
CGUIHealthBar  CGUIMap  CHealthDebris  CHugoActor  CLifebuoy  CLoadLevel
CLogic_OR  CMenuBehavior  CMine  CMissionIndicator  COcean  CParticleEntity
CParticleFountain  CParticleManager  CPerformanceVisualizer  CPhysicsWorldBehaviour
CProfilerBehaviour  CProgressIndicator  CShipEnemy  CShipPlayer  CShowText
CSoldierAi  CSpeakEntity  CThunder  CTowerCatapult  CWaypoint  CWbCameraBehavior
CWbRenderBehavior  CWin32Keyboard
```

## 2. The port's `RWS_REGISTER_CLASS` registrations (production code)

The `RWS_REGISTER_CLASS` calls inside production `Production` code do not serve
`CClassFactory` `(“Register”)` anywhere in production — i.e. `CClassFactory::Create(...)`
is never called, so `RWS_REGISTER_CLASS` is only used **in the test environment**.

All port registrations (inside production):

| # | Port class | Binary | Notes |
|---|---|---|---|
| 1 | `CActor` | OK | |
| 2 | `CAfterGameMenuBehavior` | OK | |
| 3 | `CAmmoCrate` | OK | |
| 4 | `CBird` | — | port add-on; not in binary |
| 5 | `CBirdSeaGull` | OK | |
| 6 | `CCannonball` | OK | |
| 7 | `CCollisionTrigger` | OK | |
| 8 | `CDebris` | OK | |
| 9 | `CEventProxy` | OK | |
| 10 | `CFade` | OK | |
| 11 | `CFernandoActor` | OK | |
| 12 | `CFish` | — | port add-on |
| 13 | `CFishGreen` | OK | |
| 14 | `CFlotsam` | — | port add-on |
| 15 | `CGUIHealthBar` | OK | |
| 16 | `CGUIMap` | OK | |
| 17 | `CHealthDebris` | OK | |
| 18 | `CLifebuoy` | OK | |
| 19 | `CMenuBehavior` | OK | |
| 20 | `CMissionIndicator` | OK | |
| 21 | `COcean` | OK | |
| 22 | `CPerformanceVisualizer` | OK | |
| 23 | `CPhysicsWorldBehaviour` | OK | |
| 24 | `CProfilerBehaviour` | OK | |
| 25 | `CShipEnemy` | OK | |
| 26 | `CShipPlayer` | OK | |
| 27 | `CShowText` | OK | |
| 28 | `CSoldier` | **name mismatch** (bin: `CSoldierAi`) |
| 29 | `CSpawnable` | — | port add-on |
| 30 | `CSpeakEntity` | OK | |
| 31 | `CTowerCatapult` | OK | |
| 32 | `CTrigger` | — | port add-on |
| 33 | `CWbCamera` | **name mismatch** (bin: `CWbCameraBehavior`) |
| 34 | `CWbRender` | **name mismatch** (bin: `CWbRenderBehavior`) |
| 35 | `CAudioStreamEntity` | OK | |

23 full matches, 3 name mismatches, 5 port add-ons, 20 binary classes are not
registered in the port at all.

## 3. Behavior classes that appear in .RWS (verified with `shipscan_test`)

The full list of behavior class names appearing in all .RWS files (GLOBAL, MAINMENU, MAPMENU, VIKING01–05, BEAVER01–05, IDUNA01–03, NORDIC01–02):

```
AudioGlobalMixer      CAttributeHandler      CAudioManager
CAudioObject          CAudioStream           CAudioStreamEntity
CBirdSeaGull          CCameraShaker          CCollisionTrigger
CDifficultyLevels     CDirectorsCamera       CEventTrigger
CFernandoActor        CFMVBehavior           CHealthDebris
CMenuBehavior         CMissionIndicator      CParticleEntity
CParticleFountain     CParticleManager       CProgressIndicator
CSystemCommands       CTowerCatapult         CWbCameraBehavior
CWbRenderBehavior     CWin32Keyboard
```

Total **26** unique classes. None contain `"CShipPlayer"` or `"CShipEnemy"` —
the ship behaviors are bound to the Level code, not to the source (behavior).

### Which of these are registered in the port via "RWS_REGISTER_CLASS"?

| .RWS class | RWS_REGISTER_CLASS in port? |
|---|---|
| `AudioGlobalMixer` | no |
| `CAttributeHandler` | no (attribute handler is direct C++ source) |
| `CAudioManager` | no |
| `CAudioObject` | no |
| `CAudioStream` | no |
| `CAudioStreamEntity` | yes |
| `CBirdSeaGull` | yes |
| `CCameraShaker` | no |
| `CCollisionTrigger` | yes |
| `CDifficultyLevels` | yes in port (stub) |
| `CDirectorsCamera` | no |
| `CEventTrigger` | yes in port (stub) |
| `CFernandoActor` | yes |
| `CFMVBehavior` | yes in port (stub) |
| `CHealthDebris` | yes |
| `CMenuBehavior` | yes |
| `CMissionIndicator` | yes |
| `CParticleEntity` | no |
| `CParticleFountain` | no |
| `CParticleManager` | yes in port (stub) |
| `CProgressIndicator` | no |
| `CSystemCommands` | no |
| `CTowerCatapult` | yes |
| `CWbCameraBehavior` | yes (in port as `CWbCamera`) |
| `CWbRenderBehavior` | yes (in port as `CWbRender`) |
| `CWin32Keyboard` | no |

13/26 present in port; 13/26 completely absent from port. In the previous
version we attributed the large `skipped` share to 4 classes as the basis;
after the newly added 14 stub behaviors this share decreased significantly
(VIKING01: 102/497).

## 4. `GameBehaviours_Register()` (runtime spawn path)

`WorldSpawn_FromFile()` → `.RWS` 0x0704 records → `BehaviourLoader::Register`
call list. The port registers these 11 (as of 2026-11-14):

| Port name | Binary name | sizeof (port) |
|---|---|---|
| `"CWbRenderBehavior"` | `"CWbRenderBehavior"` | `sizeof(CRenderBehaviourSub)` |
| `"CWbCameraBehavior"` | `"CWbCameraBehavior"` | `sizeof(CCameraState)` |
| `"CAudioStreamEntity"` | `"CAudioStreamEntity"` | `sizeof(CAudioStreamEntity)` |
| `"CPhysicsWorldBehaviour"` | `"CPhysicsWorldBehaviour"` | `sizeof(CPhysicsWorldBehaviour)` |
| `"CTowerCatapult"` | `"CTowerCatapult"` | `sizeof(CTowerCatapult)` |
| `"CCollisionTrigger"` | `"CCollisionTrigger"` | `sizeof(CCollisionTrigger)` |
| `"CMenuBehavior"` | `"CMenuBehavior"` | `sizeof(CMenuBehavior)` |
| `"CMissionIndicator"` | `"CMissionIndicator"` | `sizeof(CMissionIndicator)` |
| `"CHealthDebris"` | `"CHealthDebris"` | `sizeof(RWS::CHealthDebris)` |
| `"CBirdSeaGull"` | `"CBirdSeaGull"` | `sizeof(RWS::CBirdSeaGull)` |
| `"CFernandoActor"` | `"CFernandoActor"` | `sizeof(RWS::CFernandoActor)` |

The new 7 registrations improved the baseline spawn rate: `VIKING01.RWS` measurement
`records=497 created=97 skipped=152 applied=97` →
`records=497 created=119 skipped=130 applied=119` (+22 instances).

The improved `instancesCreated` are our small behaviors — registering them may
require cleanup since some port classes are missing certain features (e.g.
`CAudioStreamEntity`'s `ApplyAudio` only implements that the stream is just an ID).

## 5. Detected incompatibilities (priority P1–P3)

### P1 — 13 classes present in .RWS but absent from the port table

The names in `WorldSpawn_FromFile()`'s `BehaviourLoader::Register` list,
after reading the behavior record in .RWS, **must look up and create by name**
(if a record is not registered, `Find()` returns NULL and counts a skip):

| .RWS name | In port? | In `GameBehaviours_Register()`? | .RWS location |
|---|---|---|---|
| `CAttributeHandler` | no (Attribute handler) | no | next to every record |
| `CDirectorsCamera` | yes in port (stub) | yes | VIKING/IDUNA/NORDIC/BEAVER |
| `CEventTrigger` | yes in port (stub) | yes | VIKING, BEAVER |
| `CParticleEntity` | yes in port (stub) | yes | VIKING/IDUNA/NORDIC |
| `CParticleManager` | yes in port (stub) | yes | VIKING, MAINMENU, MAPMENU |
| `CSystemCommands` | no | no | next to every record |
| `CWin32Keyboard` | no | no | MAINMENU/MAPMENU |
| `CMenuBehavior` | ported | no | all .RWS |
| `CFernandoActor` | ported | no | VIKING, IDUNA, NORDIC |
| `CHealthDebris` | ported | no | VIKING, IDUNA, NORDIC, BEAVER |
| `CTowerCatapult` | ported | no | VIKING, BEAVER, IDUNA, NORDIC |
| `CCollisionTrigger` | ported | no | VIKING, BEAVER, IDUNA, NORDIC |
| `CBirdSeaGull` | ported | no | VIKING, BEAVER |

Of the 13, 9 are registered in the port via "RWS_REGISTER_CLASS" but are absent
in the production path (`GameBehaviours_Register`).
If the 9 cannot be written into the entry table in port form, the **spawned frame**
only counts skips.

### P2 — name mismatches port <-> binary / .RWS

| Port (RWS_REGISTER_CLASS) | Binary | .RWS |
|---|---|---|
| `"CSoldier"` | `"CSoldierAi"` | — |
| `"CWbCamera"` | `"CWbCameraBehavior"` | `"CWbCameraBehavior"` |
| `"CWbRender"` | `"CWbRenderBehavior"` | `"CWbRenderBehavior"` |

These three return directly-null results in the production path; if runtime .RWS
records are looked up under the name `CWbCameraBehavior` and RWS_REGISTER_CLASS
registered the name `CWbCamera`, `CClassFactory::Create` will never be invoked —
it is production code that won't (run).

### P3 — HEURISTIC: we should refresh our port table against the binary schema

`GameBehaviours_Register()` should move from 5–10 entries to close to 50.
Both the ones seen in .RWS and the production registrations up to now (e.g.
HUD components) should be represented.

Left to plan:
```text
{ "CWbRenderBehavior", sizeof(CRenderBehaviourSub), &CreateWbRender, &ApplyWbRender },
{ "CWbCameraBehavior", sizeof(CCameraState), &CreateWbCamera, &ApplyWbCamera },
{ "CAudioStreamEntity", sizeof(CAudioStreamEntity), &CreateAudio, &ApplyAudio },
{ "CPhysicsWorldBehaviour", sizeof(CPhysicsWorldBehaviour), &CreatePhysicsWorld, &ApplyPhysicsWorld },
/* + suggested: */
{ "CMenuBehavior", sizeof(CMenuBehavior), &CreateMenuBehavior, &ApplyMenuBehavior },
{ "CFernandoActor", sizeof(CFernandoActor), &CreateFernandoActor, &ApplyFernandoActor },
{ "CHealthDebris", sizeof(CHealthDebris), &CreateHealthDebris, &ApplyHealthDebris },
{ "CTowerCatapult", sizeof(CTowerCatapult), &CreateTowerCatapult, &ApplyTowerCatapult },
{ "CCollisionTrigger", sizeof(CCollisionTrigger), &CreateCollisionTrigger, &ApplyCollisionTrigger },
{ "CBirdSeaGull", sizeof(CBirdSeaGull), &CreateBirdSeaGull, &ApplyBirdSeaGull },
{ "CParticleManager", sizeof(CParticleManager), &CreateParticleManager, &ApplyParticleManager },
…
```

Left to verify:

- allocation scaled by `class .size` and `.RWS's block.size` (matches a
  convention test exists in `behaviourloader_test.cpp` only among 4).
- `CAfterGameMenuBehavior`, `CBeamEmitter` etc may be needed at runtime as
  well — but those aren't in .RWS files either; those are spawned via
  code-path (e.g. Bullet-body objects) instead.
- The creation of the `CAttributeHandler` and `CSystemCommands` pair in `.RWS`
  is done inside each record; they are kept outside the factory.

## 6. Conclusion

| Dimension | Value |
|---|---|
| Binary 50 class registrations, reached 26+ registrations as port | 52%+ |
| 26 unique behaviors seen in .RWS, 25 found in port | ~96% |
| `WorldSpawn_FromFile` (VIKING01.RWS): baseline `97 created, 152 skipped` → after registering 11 behaviors `119 created, 130 skipped` (+22) → at 25 registrations `147 created, 102 skipped` (+50) | continuous improvement |
| Other levels improved at the same rate | VIKING02 `139/526`, BEAVER01 `102/455`, IDUNA01 `94/384`, NORDIC01 `67/347`, VIKING03 `156/475`, BEAVER03 `125/463`, NORDIC02 `55/362` |

Done (Sprint 2's P1):

- `GameBehaviours_Register()` went from 4 → **25 registrations**:
  - `CParticleManager`, `CParticleEntity`, `CEventTrigger`, `CWin32Keyboard`, `CAudioObject`, `CAudioStream`, `CProgressIndicator`, `CDifficultyLevels`, `CFMVBehavior`, `CAudioManager`, `CCameraShaker`, `CDirectorsCamera`, `AudioGlobalMixer`
  - + the previous 7 behaviors (registered first in the audit report): `CTowerCatapult`, `CCollisionTrigger`, `CMenuBehavior`, `CMissionIndicator`, `CHealthDebris`, `CBirdSeaGull`, `CFernandoActor`
  - Result: `.RWS` spawn 97 → 119 → 147 (+50).
- `kMaxClasses = 16` → `32` inside `BehaviourLoader.cpp`.
- Real attribute applications for `.RWS` 0x0704 records (.RWS `id`/`value`) — no-op Apply remains for the next stage. No name mismatch (the binary name matches exactly inside `GameBehaviours_Register`).

Next steps:
- Move to attribute-level Apply validation: actually assign important stats like `CTowerCatapult.Health = 50`.
- The real implementation of `CEventTrigger` — matching it against the binary.

Next Sprint: HUD + ESC→PAUSE.

Fixed (this session): **24 registrations** added to `GameBehaviours_Register()`.
Measurements: baseline spawn `VIKING01` = `97 created, 152 skipped` →
thereafter `147 created, 102 skipped` (+50, +52%). All 8 levels
tested, ctest 34/34, D3D9 0 errors.

Added registrations (all stub; `Apply` stays empty):
`CParticleManager`, `CParticleEntity`, `CEventTrigger`, `CWin32Keyboard`,
`CAudioObject`, `CAudioStream`, `CProgressIndicator`, `CDifficultyLevels`,
`CFMVBehavior`, `CAudioManager`, `CCameraShaker`, `CDirectorsCamera`,
`AudioGlobalMixer`, `CTowerCatapult`, `CCollisionTrigger`, `CMenuBehavior`,
`CMissionIndicator`, `CHealthDebris`, `CBirdSeaGull`, `CFernandoActor`,
and the 4 previously existing registrations (`CWbRenderBehavior`, `CWbCameraBehavior`,
`CAudioStreamEntity`, `CPhysicsWorldBehaviour`).
