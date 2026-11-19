# Gap Analysis — Binary (.asm) vs Ported Source (2026-11-19)

Comparison of the disassembled `CannonCruise.exe` (IDA `CC_ida.asm` +
Ghidra `CC.asm`) against the ported C++ source in `code/`.

## 1. Method

- **Binary inventory**: function labels (`sub_*` in IDA, `FUN_*` in Ghidra),
  class registration thunks (`call sub_413700` — `CClassFactory::Register`),
  class-name strings and their code cross-references.
- **Port inventory**: `RWS_REGISTER_CLASS` + `GameBehaviours_Register()` +
  `FUN_*` addresses referenced in source comments.
- Cross-checked per class and per behavior.

## 2. Scale

| Dimension | Value |
|---|---|
| Binary functions (IDA) | 9,156 |
| Binary functions (Ghidra) | 3,579 |
| Binary functions (union) | 9,699 |
| Port `FUN_*` references | 423 |
| Binary functions referenced by port | 188 (1.9%) |

The raw 1.9% is **not** a defect: the binary statically links MFC, CRT,
RenderWare and Havok SDK code. The port reimplements game logic only, so the
meaningful metric is **class/feature coverage** (below).

## 3. Game classes: binary 50 vs port

Binary registers **50** classes (`call sub_413700`). The port has **52**
class/behavior names. Missing from the port (no equivalent at all):

| Binary class | Port status |
|---|---|
| `CAnimatedEnvironment` | — missing |
| `CHugoActor` | — missing |
| `CLogic_OR` | — missing |
| `CParticleFountain` | — missing (port has `CParticleEmitter`/`CParticleBank`) |
| `CThunder` | — missing |
| `CWaypoint` | — missing |

Name mismatches (same class, different port name):

| Binary | Port |
|---|---|
| `CSoldierAi` | `CSoldier` |
| `CWbCameraBehavior` | `CWbCamera` |
| `CWbRenderBehavior` | `CWbRender` |

Present but **not registered** in the port (used directly): `CLoadLevel`,
`CMine` (both exist as source).

## 4. Behavior wiring: 25 registered, 13 are no-op stubs

`.RWS` spawns 26 unique behavior classes; the port registers 25. Of those,
**13 are stubs whose `Apply` does nothing** (attribute values in `.RWS`
records are not applied):

`CParticleManager`, `CParticleEntity`, `CEventTrigger`, `CWin32Keyboard`,
`CAudioObject`, `CAudioStream`, `CProgressIndicator`, `CDifficultyLevels`,
`CFMVBehavior`, `CAudioManager`, `CCameraShaker`, `CDirectorsCamera`,
`AudioGlobalMixer`

Real implementations exist elsewhere in the port for several of these
(`CTrigger` for event triggers, `CAudioManager`/`AudioStreamEntity` for
audio, `CParticleEmitter`/`CParticleBank` for particles) — the gap is the
**Apply wiring**, not the classes.

## 5. Feature-level gaps (roadmap)

| Feature | Status |
|---|---|
| Save/Load (Slot0) | `CSaveGameManager` exists; not wired to gameplay |
| LEVELCOMPLETE screen | not wired (both towers destroyed → nothing happens) |
| Particles | `CParticleEmitter`/`CParticleBank`/`CParticleScript` exist; behavior stub |
| Audio | `CAudioManager`/`CAudioStreamEntity`/`BinkPlayer` exist; behavior stubs |
| Event triggers | `CTrigger` (real) exists; `CEventTrigger` behavior is a stub |
| World features | mine/waypoint/thunder/animated-environment classes missing |
| Logic | `CLogic_OR` missing (OR-combined trigger logic) |

## 6. Conclusion

- **Class coverage**: 44/50 binary classes have a port equivalent (88%) —
  41 exact-name + 3 name mismatches; 6 missing.
- **Behavior coverage**: 25/26 `.RWS` behaviors registered (96%); 13 need
  real `Apply` wiring.
- **Function coverage**: 1.9% raw — dominated by statically-linked library
  code; not actionable.

Highest-value next gaps: LEVELCOMPLETE wiring (closes the mission loop),
Save/Load, and real `Apply` for the 13 stub behaviors.
