// ============================================================================
// CannonCruise - Phase 4.6p: WorldSpawn implementation
// ============================================================================

#include "WorldSpawn.h"

#include "AudioStreamEntity.h"
#include "BehaviourLoader.h"
#include "CameraState.h"
#include "PhysicsWorld.h"
#include "RenderBehaviourWorld.h"
#include "RwStudioStream.h"
#include "RwsResources.h"
#include "Bird.h"
#include "FernandoActor.h"
#include "HealthDebris.h"
#include "MenuBehavior.h"
#include "MissionIndicator.h"
#include "TowerCatapult.h"
#include "CollisionTrigger.h"
#include "LevelScript.h"
#include "framework/core/attributehandler/cattributehandler.h"
#include "framework/core/attributehandler/csystemcommands.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// ---- factory + apply adaptors (binary: factory + vtable slot1) ----
// Exact replica of test pattern (behaviourloader_test.cpp); apply
// flows to singleton target (binary: apply DAT_00635648).

void *CreateWbRender() {
    CRenderBehaviourSub *pSub = new CRenderBehaviourSub;
    std::memset(pSub, 0, sizeof(*pSub));
    return pSub;
}

RwBool ApplyWbRender(void *, RwUInt32 id, const RwUInt8 *pValue) {
    if (!pValue) {
        return FALSE;
    }
    RwUInt32 bits = 0;
    std::memcpy(&bits, pValue, sizeof(bits));
    return CRenderBehaviourWorld::Instance()->ApplyWbRenderAttribute(id, bits);
}

void *CreateWbCamera() {
    CCameraState *pCam = new CCameraState;
    std::memset(pCam, 0, sizeof(*pCam));
    return pCam;
}

RwBool ApplyWbCamera(void *pObject, RwUInt32 id, const RwUInt8 *pValue) {
    if (!pObject || !pValue) {
        return FALSE;
    }
    CCameraState *pCam = static_cast<CCameraState *>(pObject);
    if (CCameraStateApplyVec3(pCam, id, pValue) == TRUE) {
        return TRUE; // VEC3 record (12B copy)
    }
    RwUInt32 bits = 0;
    std::memcpy(&bits, pValue, sizeof(bits));
    return pCam->ApplyAttribute(id, bits);
}

void *CreateAudio() {
    CAudioStreamEntity *pAudio = new CAudioStreamEntity;
    CAudioStreamEntityInit(pAudio);
    return pAudio;
}

RwBool ApplyAudio(void *pObject, RwUInt32 id, const RwUInt8 *pValue) {
    if (!pObject || !pValue) {
        return FALSE;
    }
    return CAudioStreamEntityApplyAttribute(
        static_cast<CAudioStreamEntity *>(pObject), id, pValue);
}

void *CreatePhysicsWorld() { return new CPhysicsWorldBehaviour; }

// CPhysicsWorldBehaviour apply: no property in binary — message-driven
// (HandleEvents); port returns no-op.
RwBool ApplyPhysicsWorld(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

// === Stub behavior classes — consumes .RWS 0x0704 records ===
// None of these classes runs the ACTUAL game; they only provide runtime instantiation
// ("spawn" instead of binary.RWS "skip"). If needed, real
// implementations can replace them later.
class CEventTriggerStub : public RWS::CEventHandler {
public:
    CEventTriggerStub() {}
    virtual ~CEventTriggerStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CParticleEntityStub : public RWS::CEventHandler {
public:
    CParticleEntityStub() {}
    virtual ~CParticleEntityStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CParticleManagerStub : public RWS::CEventHandler {
public:
    CParticleManagerStub() {}
    virtual ~CParticleManagerStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CWin32KeyboardStub : public RWS::CEventHandler {
public:
    CWin32KeyboardStub() {}
    virtual ~CWin32KeyboardStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CAudioObjectStub : public RWS::CEventHandler {
public:
    CAudioObjectStub() {}
    virtual ~CAudioObjectStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CAudioStreamStub : public RWS::CEventHandler {
public:
    CAudioStreamStub() {}
    virtual ~CAudioStreamStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CProgressIndicatorStub : public RWS::CEventHandler {
public:
    CProgressIndicatorStub() {}
    virtual ~CProgressIndicatorStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CDifficultyLevelsStub : public RWS::CEventHandler {
public:
    CDifficultyLevelsStub() {}
    virtual ~CDifficultyLevelsStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CFMVBehaviorStub : public RWS::CEventHandler {
public:
    CFMVBehaviorStub() {}
    virtual ~CFMVBehaviorStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CAudioManagerStub : public RWS::CEventHandler {
public:
    CAudioManagerStub() {}
    virtual ~CAudioManagerStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CCameraShakerStub : public RWS::CEventHandler {
public:
    CCameraShakerStub() {}
    virtual ~CCameraShakerStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CDirectorsCameraStub : public RWS::CEventHandler {
public:
    CDirectorsCameraStub() {}
    virtual ~CDirectorsCameraStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};
class CAudioGlobalMixerStub : public RWS::CEventHandler {
public:
    CAudioGlobalMixerStub() {}
    virtual ~CAudioGlobalMixerStub() {}
    virtual void HandleEvents(const RWS::CMsg &) {}
};

void *CreateEventTrigger() { return new CEventTriggerStub; }
RwBool ApplyEventTrigger(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateParticleEntity() { return new CParticleEntityStub; }
RwBool ApplyParticleEntity(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateParticleManager() { return new CParticleManagerStub; }
RwBool ApplyParticleManager(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateWin32Keyboard() { return new CWin32KeyboardStub; }
RwBool ApplyWin32Keyboard(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateAudioObject() { return new CAudioObjectStub; }
RwBool ApplyAudioObject(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateAudioStream() { return new CAudioStreamStub; }
RwBool ApplyAudioStream(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateProgressIndicator() { return new CProgressIndicatorStub; }
RwBool ApplyProgressIndicator(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateDifficultyLevels() { return new CDifficultyLevelsStub; }
RwBool ApplyDifficultyLevels(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateFMVBehavior() { return new CFMVBehaviorStub; }
RwBool ApplyFMVBehavior(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateAudioManager() { return new CAudioManagerStub; }
RwBool ApplyAudioManager(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateCameraShaker() { return new CCameraShakerStub; }
RwBool ApplyCameraShaker(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateDirectorsCamera() { return new CDirectorsCameraStub; }
RwBool ApplyDirectorsCamera(void *, RwUInt32, const RwUInt8 *) { return TRUE; }
void *CreateAudioGlobalMixer() { return new CAudioGlobalMixerStub; }
RwBool ApplyAudioGlobalMixer(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

// `Create + Apply` helpers — directly create the .RWS behavior classes
// (no object registration for the specials); misapplies become no-ops.
void *CreateTowerCatapult() { return new CTowerCatapult(); }

RwBool ApplyTowerCatapult(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateCollisionTrigger() { return new CCollisionTrigger(); }

RwBool ApplyCollisionTrigger(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateMenuBehavior() { return new CMenuBehavior(); }

RwBool ApplyMenuBehavior(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateMissionIndicator() { return new CMissionIndicator(); }

RwBool ApplyMissionIndicator(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateHealthDebris() {
    RWS::CAttributePacket emptyPacket;
    return new RWS::CHealthDebris(emptyPacket);
}

RwBool ApplyHealthDebris(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateBirdSeaGull() {
    RWS::CAttributePacket emptyPacket;
    return new RWS::CBirdSeaGull(emptyPacket);
}

RwBool ApplyBirdSeaGull(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

void *CreateFernandoActor() {
    RWS::CAttributePacket emptyPacket;
    return new RWS::CFernandoActor(emptyPacket);
}

RwBool ApplyFernandoActor(void *, RwUInt32, const RwUInt8 *) { return TRUE; }

} // namespace

RwBool GameBehaviours_Register() {
    BehaviourLoader::UnregisterAll();

    BehaviourClassEntry entries[] = {
        {"CWbRenderBehavior", sizeof(CRenderBehaviourSub), &CreateWbRender,
         &ApplyWbRender},
        {"CWbCameraBehavior", sizeof(CCameraState), &CreateWbCamera,
         &ApplyWbCamera},
        {"CAudioStreamEntity", sizeof(CAudioStreamEntity), &CreateAudio,
         &ApplyAudio},
        {"CPhysicsWorldBehaviour", sizeof(CPhysicsWorldBehaviour),
         &CreatePhysicsWorld, &ApplyPhysicsWorld},
        {"CParticleManager", sizeof(CParticleManagerStub), &CreateParticleManager,
         &ApplyParticleManager},
        {"CParticleEntity", sizeof(CParticleEntityStub), &CreateParticleEntity,
         &ApplyParticleEntity},
        {"CEventTrigger", sizeof(CEventTriggerStub), &CreateEventTrigger,
         &ApplyEventTrigger},
        {"CWin32Keyboard", sizeof(CWin32KeyboardStub), &CreateWin32Keyboard,
         &ApplyWin32Keyboard},
        {"CAudioObject", sizeof(CAudioObjectStub), &CreateAudioObject,
         &ApplyAudioObject},
        {"CAudioStream", sizeof(CAudioStreamStub), &CreateAudioStream,
         &ApplyAudioStream},
        {"CProgressIndicator", sizeof(CProgressIndicatorStub),
         &CreateProgressIndicator, &ApplyProgressIndicator},
        {"CDifficultyLevels", sizeof(CDifficultyLevelsStub),
         &CreateDifficultyLevels, &ApplyDifficultyLevels},
        {"CFMVBehavior", sizeof(CFMVBehaviorStub), &CreateFMVBehavior,
         &ApplyFMVBehavior},
        {"CAudioManager", sizeof(CAudioManagerStub), &CreateAudioManager,
         &ApplyAudioManager},
        {"CCameraShaker", sizeof(CCameraShakerStub), &CreateCameraShaker,
         &ApplyCameraShaker},
        {"CDirectorsCamera", sizeof(CDirectorsCameraStub), &CreateDirectorsCamera,
         &ApplyDirectorsCamera},
        {"AudioGlobalMixer", sizeof(CAudioGlobalMixerStub),
         &CreateAudioGlobalMixer, &ApplyAudioGlobalMixer},
        // `.RWS` behaviors detected in the audit (450+ instances):
        {"CTowerCatapult", sizeof(CTowerCatapult), &CreateTowerCatapult,
         &ApplyTowerCatapult},
        {"CCollisionTrigger", sizeof(CCollisionTrigger), &CreateCollisionTrigger,
         &ApplyCollisionTrigger},
        {"CMenuBehavior", sizeof(CMenuBehavior), &CreateMenuBehavior,
         &ApplyMenuBehavior},
        {"CMissionIndicator", sizeof(CMissionIndicator), &CreateMissionIndicator,
         &ApplyMissionIndicator},
        {"CHealthDebris", sizeof(RWS::CHealthDebris), &CreateHealthDebris,
         &ApplyHealthDebris},
        {"CBirdSeaGull", sizeof(RWS::CBirdSeaGull), &CreateBirdSeaGull,
         &ApplyBirdSeaGull},
        {"CFernandoActor", sizeof(RWS::CFernandoActor), &CreateFernandoActor,
         &ApplyFernandoActor},
    };

    const RwUInt32 entryCount = sizeof(entries) / sizeof(entries[0]);
    for (RwUInt32 i = 0; i < entryCount; ++i) {
        if (BehaviourLoader::Register(&entries[i]) != TRUE) {
            BehaviourLoader::UnregisterAll();
            return FALSE;
        }
    }
    return TRUE;
}

// ---- Tower placements (CTowerCatapult object records) ----
// Record layout (VIKING01 hexdump): first behaviour CTowerCatapult; its
// attribute block (2nd CTowerCatapult segment) holds id3 = mission death
// event string on mission towers; CSystemCommands id1 (72-byte record) is the
// entity RwMatrix, translation at floats 12..14.
namespace {

WorldSpawnTower s_towers[WORLDSPAWN_MAX_TOWERS];
RwUInt32 s_towerCount = 0;

// Clump resources referenced by the packet's CSystemCommands GUIDs, in
// record order (model first, then collision hull, damaged model ...).
const RwsResource *FindModel(const RWS::CAttributePacket &packet,
                             const char *pNameContains, RwBool bSkipHulls) {
    for (RWS::CAttributeCommandIterator it(packet, "CSystemCommands");
         !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 0 || it->GetDataSize() != 16) {
            continue;
        }
        const RwsResource *pRes = RwsResources_Find(
            static_cast<const RwUInt8 *>(it->GetCommandData()));
        if (!pRes || pRes->embeddedType != rwsRESOURCE_CLUMP) {
            continue;
        }
        if (bSkipHulls == TRUE && std::strstr(pRes->name, "Hull") != 0) {
            continue;
        }
        if (pNameContains == 0 || std::strstr(pRes->name, pNameContains) != 0) {
            return pRes;
        }
    }
    return 0;
}

// Printable string payload of attribute `id` in the `className` block.
RwBool CopyAttributeString(const RWS::CAttributePacket &packet,
                           const char *className, RwUInt32 id,
                           RwChar *pOut, RwUInt32 outSize) {
    pOut[0] = '\0';
    for (RWS::CAttributeCommandIterator it(packet, className);
         !it.IsFinished(); ++it) {
        if (it->GetCommandId() != id || it->GetDataSize() <= 1) {
            continue;
        }
        const RwUInt8 *pStr = static_cast<const RwUInt8 *>(it->GetCommandData());
        RwUInt32 i = 0;
        for (; i + 1 < outSize && i < it->GetDataSize(); ++i) {
            if (pStr[i] < 0x20 || pStr[i] >= 0x7F) {
                break;
            }
            pOut[i] = (RwChar)pStr[i];
        }
        pOut[i] = '\0';
        return i > 0 ? TRUE : FALSE;
    }
    return FALSE;
}

// HAnim clips referenced by the record: node ID from the "_tagNNN" part of
// the asset name ("Wing_Left_tag100.anm" -> node 100).
void CollectClips(const RWS::CAttributePacket &packet,
                  WorldSpawnPlayerShip::Clip *pClips, RwUInt32 maxClips,
                  RwUInt32 *pCount) {
    *pCount = 0;
    for (RWS::CAttributeCommandIterator it(packet, "CSystemCommands");
         !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 0 || it->GetDataSize() != 16 ||
            *pCount >= maxClips) {
            continue;
        }
        const RwsResource *pRes = RwsResources_Find(
            static_cast<const RwUInt8 *>(it->GetCommandData()));
        if (!pRes || pRes->embeddedType != rwsRESOURCE_HANIM) {
            continue;
        }
        const char *pTag = std::strstr(pRes->name, "_tag");
        if (!pTag) {
            continue;
        }
        const int nodeId = std::atoi(pTag + 4);
        if (nodeId <= 0) {
            continue;
        }
        pClips[*pCount].pAnim = pRes;
        pClips[*pCount].nodeId = nodeId;
        ++*pCount;
    }
}

WorldSpawnPlayerShip s_playerShip;
WorldSpawnEnemyShip s_enemyShips[WORLDSPAWN_MAX_ENEMY_SHIPS];
RwUInt32 s_enemyShipCount = 0;

void CollectEnemyShip(const RwStudioObject *pObj,
                      const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CShipEnemy") != 0 ||
        s_enemyShipCount >= WORLDSPAWN_MAX_ENEMY_SHIPS) {
        return;
    }
    WorldSpawnEnemyShip ship;
    std::memset(&ship, 0, sizeof(ship));
    if (RWS::CSystemCommands::ExtractMatrix(packet, ship.matrix) != TRUE) {
        return;
    }
    ship.pModel = FindModel(packet, 0, TRUE);
    CopyAttributeString(packet, "CShipEnemy", 8, ship.route, sizeof(ship.route));
    CollectClips(packet, ship.clips, WORLDSPAWN_MAX_ENEMY_CLIPS, &ship.clipCount);
    CopyAttributeString(packet, "CSpawnable", 1, ship.spawnEvent, sizeof(ship.spawnEvent));
    CopyAttributeString(packet, "CShipEnemy", 17, ship.deathEvent, sizeof(ship.deathEvent));
    s_enemyShips[s_enemyShipCount++] = ship;
}

WorldSpawnFish s_fish[WORLDSPAWN_MAX_FISH];
RwUInt32 s_fishCount = 0;

WorldSpawnActor s_actors[2];
WorldSpawnBird s_birds[WORLDSPAWN_MAX_BIRDS];
RwUInt32 s_birdCount = 0;

void CollectBird(const RwStudioObject *pObj, const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CBirdSeaGull") != 0 ||
        s_birdCount >= WORLDSPAWN_MAX_BIRDS) {
        return;
    }
    WorldSpawnBird b;
    std::memset(&b, 0, sizeof(b));
    RwReal m[16];
    if (RWS::CSystemCommands::ExtractMatrix(packet, m) != TRUE) return;
    b.pos[0] = m[12];
    b.pos[1] = m[13];
    b.pos[2] = m[14];
    b.pModel = FindModel(packet, 0, TRUE);
    WorldSpawnPlayerShip::Clip clips[2];
    RwUInt32 n = 0;
    CollectClips(packet, clips, 2, &n);
    if (n > 0) {
        b.pFlap = clips[0].pAnim;
        b.flapNode = clips[0].nodeId;
    }
    if (b.pModel != 0) s_birds[s_birdCount++] = b;
}
WorldSpawnSky s_sky;
WorldSpawnGuiMap s_guiMap;

void CollectGuiMap(const RwStudioObject *pObj, const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 || std::strcmp(pObj->behaviours[0].className, "CGUIMap") != 0) {
        return;
    }
    for (RWS::CAttributeCommandIterator it(packet, "CGUIMap"); !it.IsFinished(); ++it) {
        if (it->GetDataSize() != 4) continue;
        RwReal f = 0.0f;
        std::memcpy(&f, it->GetCommandData(), 4);
        if (it->GetCommandId() == 5) s_guiMap.worldW = f;
        if (it->GetCommandId() == 6) s_guiMap.worldH = f;
        if (it->GetCommandId() == 7) s_guiMap.zoom = f;
    }
    s_guiMap.valid = (s_guiMap.worldW > 1.0f && s_guiMap.worldH > 1.0f) ? TRUE : FALSE;
}

// COcean: sky dome, cloud layer and sun models + its colour attributes.
void CollectSky(const RwStudioObject *pObj, const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 || std::strcmp(pObj->behaviours[0].className, "COcean") != 0 ||
        s_sky.modelCount > 0) {
        return;
    }
    for (RWS::CAttributeCommandIterator it(packet, "CSystemCommands"); !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 0 || it->GetDataSize() != 16 || s_sky.modelCount >= 4) {
            continue;
        }
        const RwsResource *pRes =
            RwsResources_Find(static_cast<const RwUInt8 *>(it->GetCommandData()));
        if (pRes != 0 && pRes->embeddedType == rwsRESOURCE_CLUMP) {
            s_sky.models[s_sky.modelCount++] = pRes;
        }
    }
    for (RWS::CAttributeCommandIterator it(packet, "COcean"); !it.IsFinished(); ++it) {
        const RwUInt32 id = it->GetCommandId();
        if (id < 40 && it->GetDataSize() == 4) {
            std::memcpy(&s_sky.attr[id], it->GetCommandData(), 4);
        }
    }
}

// CHugoActor (0) / CFernandoActor (1): main model + every animation the
// record references.
void CollectActor(const RwStudioObject *pObj, const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0) {
        return;
    }
    const char *cls = pObj->behaviours[0].className;
    const int idx = std::strcmp(cls, "CHugoActor") == 0       ? 0
                    : std::strcmp(cls, "CFernandoActor") == 0 ? 1
                                                               : -1;
    if (idx < 0 || s_actors[idx].pModel != 0) {
        return;
    }
    WorldSpawnActor &a = s_actors[idx];
    a.pModel = FindModel(packet, idx == 0 ? "Hugo.dff" : "fernando_Anim", TRUE);
    if (a.pModel == 0) {
        a.pModel = FindModel(packet, 0, TRUE);
    }
    for (RWS::CAttributeCommandIterator it(packet, "CSystemCommands"); !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 0 || it->GetDataSize() != 16 || a.clipCount >= 24) {
            continue;
        }
        const RwsResource *pRes =
            RwsResources_Find(static_cast<const RwUInt8 *>(it->GetCommandData()));
        if (pRes != 0 && pRes->embeddedType == rwsRESOURCE_HANIM) {
            a.clips[a.clipCount++] = pRes;
        }
    }
    std::printf("WorldSpawn: %s model %s, %u clips\n", cls,
                a.pModel ? static_cast<const RwsResource *>(a.pModel)->name : "-",
                (unsigned)a.clipCount);
}

void CollectFish(const RwStudioObject *pObj, const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CFishGreen") != 0 ||
        s_fishCount >= WORLDSPAWN_MAX_FISH) {
        return;
    }
    WorldSpawnFish fish;
    std::memset(&fish, 0, sizeof(fish));
    RwReal m[16];
    if (RWS::CSystemCommands::ExtractMatrix(packet, m) != TRUE) {
        return;
    }
    fish.pos[0] = m[12];
    fish.pos[1] = m[13];
    fish.pos[2] = m[14];
    fish.pModel = FindModel(packet, 0, TRUE);
    CopyAttributeString(packet, "CSpawnable", 1, fish.spawnEvent, sizeof(fish.spawnEvent));
    CopyAttributeString(packet, "CFish", 12, fish.pickupEvent, sizeof(fish.pickupEvent));
    s_fish[s_fishCount++] = fish;
}

// CWaypoint records: id0 = name, CSystemCommands matrix = position. Enemy
// routes (CShipEnemy id8) name one of these.
struct WaypointEntry {
    RwChar name[64];
    RwReal pos[3];
};
WaypointEntry s_waypoints[256];
RwUInt32 s_waypointCount = 0;

void CollectWaypoint(const RwStudioObject *pObj,
                     const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CWaypoint") != 0 ||
        s_waypointCount >= sizeof(s_waypoints) / sizeof(s_waypoints[0])) {
        return;
    }
    WaypointEntry wp;
    std::memset(&wp, 0, sizeof(wp));
    RwReal m[16];
    if (RWS::CSystemCommands::ExtractMatrix(packet, m) != TRUE ||
        CopyAttributeString(packet, "CWaypoint", 0, wp.name, sizeof(wp.name)) != TRUE) {
        return;
    }
    wp.pos[0] = m[12];
    wp.pos[1] = m[13];
    wp.pos[2] = m[14];
    s_waypoints[s_waypointCount++] = wp;
}

void ResolveEnemyRoutes() {
    for (RwUInt32 i = 0; i < s_enemyShipCount; ++i) {
        WorldSpawnEnemyShip &ship = s_enemyShips[i];
        if (ship.route[0] == '\0') {
            continue;
        }
        for (RwUInt32 w = 0; w < s_waypointCount; ++w) {
            if (std::strcmp(s_waypoints[w].name, ship.route) == 0) {
                ship.hasWaypoint = TRUE;
                std::memcpy(ship.waypoint, s_waypoints[w].pos, sizeof(ship.waypoint));
                break;
            }
        }
    }
}

void CollectPlayerShip(const RwStudioObject *pObj,
                       const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CShipPlayer") != 0 ||
        s_playerShip.valid == TRUE) {
        return;
    }
    if (RWS::CSystemCommands::ExtractMatrix(packet, s_playerShip.matrix) !=
        TRUE) {
        return;
    }
    s_playerShip.pModel = FindModel(packet, 0, TRUE);
    CollectClips(packet, s_playerShip.clips, 16, &s_playerShip.clipCount);
    s_playerShip.valid = TRUE;
}

void CollectTower(const RwStudioObject *pObj,
                  const RWS::CAttributePacket &packet) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CTowerCatapult") != 0 ||
        s_towerCount >= WORLDSPAWN_MAX_TOWERS) {
        return;
    }
    WorldSpawnTower tower;
    std::memset(&tower, 0, sizeof(tower));
    if (RWS::CSystemCommands::ExtractMatrix(packet, tower.matrix) != TRUE) {
        return;
    }
    tower.pos.x = tower.matrix[12];
    tower.pos.y = tower.matrix[13];
    tower.pos.z = tower.matrix[14];
    // id3: mission death event (string) on mission towers, a number otherwise.
    for (RWS::CAttributeCommandIterator it(packet, "CTowerCatapult");
         !it.IsFinished(); ++it) {
        if (it->GetCommandId() != 3 || it->GetDataSize() <= 4) {
            continue;
        }
        const RwUInt8 *pStr = static_cast<const RwUInt8 *>(it->GetCommandData());
        RwUInt32 i = 0;
        for (; i + 1 < sizeof(tower.missionEvent) && i < it->GetDataSize(); ++i) {
            if (pStr[i] < 0x20 || pStr[i] >= 0x7F) {
                break;
            }
            tower.missionEvent[i] = (RwChar)pStr[i];
        }
        tower.missionEvent[i] = '\0';
        break;
    }
    tower.pModel = FindModel(packet, 0, TRUE);
    tower.pDamaged = FindModel(packet, "_Damaged", TRUE);
    if (tower.pDamaged == tower.pModel) {
        tower.pDamaged = 0;
    }
    s_towers[s_towerCount++] = tower;
}

} // namespace

const WorldSpawnSky *WorldSpawn_GetSky(void) { return &s_sky; }

RwUInt32 WorldSpawn_GetBirds(const WorldSpawnBird **ppBirds) {
    if (ppBirds) *ppBirds = s_birds;
    return s_birdCount;
}

const WorldSpawnGuiMap *WorldSpawn_GetGuiMap(void) { return &s_guiMap; }

const WorldSpawnActor *WorldSpawn_GetActor(RwUInt32 index) {
    return index < 2 ? &s_actors[index] : 0;
}

RwUInt32 WorldSpawn_GetFish(const WorldSpawnFish **ppFish) {
    if (ppFish) {
        *ppFish = s_fish;
    }
    return s_fishCount;
}

RwUInt32 WorldSpawn_GetEnemyShips(const WorldSpawnEnemyShip **ppShips) {
    if (ppShips) {
        *ppShips = s_enemyShips;
    }
    return s_enemyShipCount;
}

const WorldSpawnPlayerShip *WorldSpawn_GetPlayerShip(void) {
    return &s_playerShip;
}

RwUInt32 WorldSpawn_GetTowers(const WorldSpawnTower **ppTowers) {
    if (ppTowers) {
        *ppTowers = s_towers;
    }
    return s_towerCount;
}

// Pass 1: register the level's embedded models and animations by GUID. Object records may
// come before the assets they reference, so this runs over the whole file
// before any object is created.
namespace {

// Finds the sized clump chunk [u32 len+12][u32 0x10][u32 len][u32 libid][body]
// in a model asset's resource bytes. Usually at offset 0; on some assets the
// meta's texture-folder string precedes it, so scan 4-byte aligned offsets.
RwBool FindClumpChunk(const std::vector<RwUInt8> &buf, RwUInt32 chunkId,
                      RwUInt32 *pBodyOffset, RwUInt32 *pBodySize) {
    const RwUInt32 n = (RwUInt32)buf.size();
    for (RwUInt32 off = 0; off + 16 <= n; off += 4) {
        RwUInt32 hdr[3];
        std::memcpy(hdr, &buf[off], sizeof(hdr));
        if (hdr[1] == chunkId && hdr[0] == hdr[2] + 12 &&
            hdr[2] > 0 && hdr[2] <= n - off - 16) {
            *pBodyOffset = off + 16;
            *pBodySize = hdr[2];
            return TRUE;
        }
    }
    return FALSE;
}

} // namespace

RwUInt32 WorldSpawn_RegisterResources(const RwChar *pPath) {
    RwsResources_Clear();
    RwStudioFile *pFile = pPath ? RwStudioFileOpen(pPath) : 0;
    if (!pFile) {
        return 0;
    }
    RwStudioAsset asset;
    std::vector<RwUInt8> buf;
    while (RwStudioFileNextAsset(pFile, &asset) == TRUE) {
        RwUInt32 chunkId = 0;
        if (std::strcmp(asset.rwIdName, "rwID_CLUMP") == 0) {
            chunkId = rwsRESOURCE_CLUMP;
        } else if (std::strcmp(asset.rwIdName, "rwID_HANIMANIMATION") == 0) {
            chunkId = rwsRESOURCE_HANIM;
        }
        if (asset.chunkType != rwstudioID_ASSETRECORD || chunkId == 0 ||
            asset.resourceLength < 16) {
            continue;
        }
        buf.resize(asset.resourceLength);
        if (RwStudioAssetReadResource(pFile, &asset, &buf[0],
                                      asset.resourceLength) !=
            asset.resourceLength) {
            continue;
        }
        RwUInt32 bodyOffset = 0, bodySize = 0;
        if (FindClumpChunk(buf, chunkId, &bodyOffset, &bodySize) == TRUE) {
            RwsResources_Add(asset.guid, chunkId, asset.typeName,
                             &buf[bodyOffset], bodySize);
        }
    }
    RwStudioFileClose(pFile);
    return RwsResources_Count();
}

RwInt32 WorldSpawn_FromFile(const RwChar *pPath, WorldSpawnStats *pStats) {
    s_towerCount = 0;
    std::memset(&s_playerShip, 0, sizeof(s_playerShip));
    s_enemyShipCount = 0;
    s_waypointCount = 0;
    s_fishCount = 0;
    std::memset(s_actors, 0, sizeof(s_actors));
    s_birdCount = 0;
    std::memset(&s_sky, 0, sizeof(s_sky));
    std::memset(&s_guiMap, 0, sizeof(s_guiMap));
    LevelScript_Reset();
    WorldSpawn_RegisterResources(pPath);
    WorldSpawnStats stats;
    std::memset(&stats, 0, sizeof(stats));
    if (pStats) {
        std::memset(pStats, 0, sizeof(*pStats));
    }
    if (!pPath) {
        return -1;
    }

    RwStudioFile *pFile = RwStudioFileOpen(pPath);
    if (!pFile) {
        return -1;
    }

    RwStudioAsset asset;
    RwInt32 totalApplied = 0;
    while (RwStudioFileNextAsset(pFile, &asset) == TRUE) {
        const RwStudioObject *pObj = RwStudioFileGetObject(pFile);
        if (!pObj) {
            continue;
        }
        ++stats.objectRecords;
        const RwUInt8 *pRaw = 0;
        const RwUInt32 rawSize = RwStudioFileGetObjectRaw(pFile, &pRaw);
        const RWS::CAttributePacket packet(pRaw, rawSize);
        CollectTower(pObj, packet);
        CollectPlayerShip(pObj, packet);
        CollectEnemyShip(pObj, packet);
        CollectWaypoint(pObj, packet);
        CollectFish(pObj, packet);
        CollectActor(pObj, packet);
        CollectBird(pObj, packet);
        CollectSky(pObj, packet);
        CollectGuiMap(pObj, packet);
        // CANNONCRUISE_DUMPREFS=CClassA,CClassB: list the resources and
        // attributes such records reference (diagnostics).
        static const char *s_dumpRefs = std::getenv("CANNONCRUISE_DUMPREFS");
        if (s_dumpRefs != 0 && pObj->behaviourCount > 0 &&
            std::strstr(s_dumpRefs, pObj->behaviours[0].className) != 0) {
            std::printf("DumpRefs: %s\n", pObj->behaviours[0].className);
            for (RWS::CAttributeCommandIterator it(packet, 0); !it.IsFinished(); ++it) {
                if (it->GetCommandId() == 0 && it->GetDataSize() == 16) {
                    const RwsResource *pRes = RwsResources_Find(
                        static_cast<const RwUInt8 *>(it->GetCommandData()));
                    std::printf("  res %s (type 0x%x)\n", pRes ? pRes->name : "?",
                                pRes ? (unsigned)pRes->embeddedType : 0u);
                } else if (it->GetDataSize() == 4) {
                    RwReal f = 0.0f;
                    RwInt32 n = 0;
                    std::memcpy(&f, it->GetCommandData(), 4);
                    std::memcpy(&n, it->GetCommandData(), 4);
                    std::printf("  id%u = %d / %g\n", (unsigned)it->GetCommandId(), (int)n, (double)f);
                } else {
                    const char *p = static_cast<const char *>(it->GetCommandData());
                    std::printf("  id%u (%u bytes) '%.*s'\n", (unsigned)it->GetCommandId(),
                                (unsigned)it->GetDataSize(), (int)(it->GetDataSize() < 60 ? it->GetDataSize() : 60), p);
                }
            }
        }
        LevelScript_CollectObject(pObj, packet);
        void *pInstances[1];
        const RwInt32 applied = BehaviourLoader::ProcessObject(
            pObj, pInstances, 1, rawSize > 0 ? &packet : 0);
        if (applied > 0) {
            stats.instancesCreated += (RwUInt32)applied;
            totalApplied += applied;
        } else if (pObj->behaviourCount > 0) {
            // unknown class: factory miss (binary: property skip)
            ++stats.instancesSkipped;
        }
    }
    RwStudioFileClose(pFile);
    ResolveEnemyRoutes();

    if (pStats) {
        *pStats = stats;
    }
    return totalApplied;
}