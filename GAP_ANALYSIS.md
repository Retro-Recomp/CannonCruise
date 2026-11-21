# CannonCruise — Gap Analysis (2026-11-20)

What the port does today and what is still missing. Every "works" line below
was checked by a build, a test or a headless run on 2026-11-20; anything not
checked is marked as such. The binary-vs-source class comparison lives in
[docs/GAP_ANALYSIS.md](docs/GAP_ANALYSIS.md).

## Verified state

| Area | State | Evidence |
|---|---|---|
| Win32 build, NULL + D3D9 (VS 18) | ✅ 0 errors | local builds |
| Tests | ✅ 36/36 with game data, 17/17 without (19 disabled) | `ctest` |
| CI | 🟡 workflow added, not yet run on GitHub | `.github/workflows/build.yml` |
| Physics world | ✅ created from the level's `CPhysicsWorldBehaviour`, Bullet steps every active frame | cannonball trajectory trace |
| Towers | ✅ placed from `CTowerCatapult` records (CSystemCommands matrix); mission towers = those with an `id3` death event | VIKING01: 20 towers, 6 mission |
| Cannon → tower damage | ✅ AABB check with 0.3 m contact slop after the Bullet step | `HIT` / `DESTROYED` log |
| LEVELCOMPLETE | ✅ all mission towers down → summary panel (score, towers, accuracy) | headless run + D3D9 screenshot |
| Levels load | ✅ all 15 playable levels + MAPMENU run without crashing | 12 s smoke run each |

## Open gaps (priority order)

1. **Return to map after LEVELCOMPLETE.** The summary only offers quit to
   desktop. `main()` runs map → world → frame loop once; it has to loop over
   those phases (with full world/physics teardown) before the player can
   continue.
2. **Player ship takes no damage.** HP bar is static; the level cannot be lost.
3. **Tower stats are port values.** 150 HP, 100 damage per hit. The record's
   `id1` (40 on VIKING, 135 on BEAVER) is probably hit points — unverified.
4. **Attribute packets work, but most consumers are not driven.** Each
   0x0704 record is passed to its class as a `CAttributePacket`;
   `CAttributeCommandIterator(packet, RWS_CLASS_INDEX(C))` returns that
   class's commands (tested on a real VIKING01 tower record). Remaining:
   - `iMsgRunningTick` is never sent by `main.cpp`, so framework objects
     (fish, flotsam, enemy ships …) are created but never update.
   - Models are resolved but not drawn: `WorldSpawn_RegisterResources`
     registers each level's embedded clumps by GUID (VIKING01: 64),
     `CSystemCommands::ExtractClump` returns the object's first model and
     `ClumpHelper` builds a librw clump placed at the record's matrix. These
     clumps are not added to the RenderPlatform scene yet.
   - Command ids in most ported `HandleAttributes` switches are guesses and
     must be checked against the data before relying on them.
5. **Spawning:** one entity per record (the record's first class).
   `CAmmoCrate`, `CFishGreen`, `CDebris`, `CLifebuoy` are now built from
   their packets (`RWS_REGISTER_PACKET_CLASS`). Still absent from the port:
   `CMine` (header only), `CHugoActor`, `CParticleFountain`,
   `CAnimatedEnvironment`, `CSoldierAi`, `CWaypoint` (source exists, not
   registered). About 108 of 497 VIKING01 records are skipped.
6. **`CShipEnemy`** is created but inert (see 4).
7. **IDUNA / NORDIC missions** have no tower records; they still use two
   placeholder towers derived from the world bounds.
8. **No-op behaviours:** `CEventTrigger`, particle classes, `CCameraShaker`,
   `CDirectorsCamera`, audio/difficulty classes in `WorldSpawn.cpp`.
9. **Audio/language:** about 106 of 118 WAVs are never played; speech
   language is hard-coded to `"Tyr"` (only `Speaks/Tyr/` exists on disk);
   Turkish text (`Localiza/TR/lang_tyr.txt`) is not listed in `lang.def`.
10. **Parity:** the original world-update wrapper passes ~1/30 s, the port
    steps 1/60 s; unresolved.

## Notes

- The RWS object-record parser now reads properties of any size and
  recognises class-name segments by flag. Before this, long class names were
  dropped (no physics world existed) and string/matrix properties ended the
  record.
- Apply classes under `code/Apply*.cpp` are not on the spawn path; the
  loader's apply adaptors and the packet are what reach spawned objects.
