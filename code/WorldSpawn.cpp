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
#include "framework/core/attributehandler/cattributehandler.h"

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

void CollectTower(const RwStudioObject *pObj) {
    if (pObj->behaviourCount == 0 ||
        std::strcmp(pObj->behaviours[0].className, "CTowerCatapult") != 0 ||
        s_towerCount >= WORLDSPAWN_MAX_TOWERS) {
        return;
    }
    WorldSpawnTower tower;
    std::memset(&tower, 0, sizeof(tower));
    RwBool bHasPos = FALSE;
    for (RwUInt32 b = 0; b < pObj->behaviourCount; ++b) {
        const RwStudioBehaviour &beh = pObj->behaviours[b];
        const RwBool isSys = std::strcmp(beh.className, "CSystemCommands") == 0;
        const RwBool isTower = std::strcmp(beh.className, "CTowerCatapult") == 0;
        for (RwUInt32 p = 0; p < beh.propCount; ++p) {
            const RwStudioProperty &prop = beh.props[p];
            if (isSys && prop.id == 1 && prop.size == 8 + 64 && bHasPos == FALSE) {
                RwReal m[16];
                std::memcpy(m, prop.value, sizeof(m));
                tower.pos.x = m[12];
                tower.pos.y = m[13];
                tower.pos.z = m[14];
                bHasPos = TRUE;
            } else if (isTower && prop.id == 3 && prop.size > 12) {
                RwUInt32 i = 0;
                const RwUInt32 maxLen = prop.size - 8 < rwstudioMAX_PROPVALUE
                                            ? prop.size - 8
                                            : rwstudioMAX_PROPVALUE;
                for (; i + 1 < sizeof(tower.missionEvent) && i < maxLen; ++i) {
                    const RwUInt8 c = prop.value[i];
                    if (c < 0x20 || c >= 0x7F) {
                        break;
                    }
                    tower.missionEvent[i] = (RwChar)c;
                }
                tower.missionEvent[i] = '\0';
            }
        }
    }
    if (bHasPos == TRUE) {
        s_towers[s_towerCount++] = tower;
    }
}

} // namespace

RwUInt32 WorldSpawn_GetTowers(const WorldSpawnTower **ppTowers) {
    if (ppTowers) {
        *ppTowers = s_towers;
    }
    return s_towerCount;
}

// Pass 1: register the level's embedded models by GUID. Object records may
// come before the assets they reference, so this runs over the whole file
// before any object is created.
namespace {

// Finds the sized clump chunk [u32 len+12][u32 0x10][u32 len][u32 libid][body]
// in a model asset's resource bytes. Usually at offset 0; on some assets the
// meta's texture-folder string precedes it, so scan 4-byte aligned offsets.
RwBool FindClumpChunk(const std::vector<RwUInt8> &buf, RwUInt32 *pBodyOffset,
                      RwUInt32 *pBodySize) {
    const RwUInt32 n = (RwUInt32)buf.size();
    for (RwUInt32 off = 0; off + 16 <= n; off += 4) {
        RwUInt32 hdr[3];
        std::memcpy(hdr, &buf[off], sizeof(hdr));
        if (hdr[1] == rwsRESOURCE_CLUMP && hdr[0] == hdr[2] + 12 &&
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
        if (asset.chunkType != rwstudioID_ASSETRECORD ||
            std::strcmp(asset.rwIdName, "rwID_CLUMP") != 0 ||
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
        if (FindClumpChunk(buf, &bodyOffset, &bodySize) == TRUE) {
            RwsResources_Add(asset.guid, rwsRESOURCE_CLUMP, asset.typeName,
                             &buf[bodyOffset], bodySize);
        }
    }
    RwStudioFileClose(pFile);
    return RwsResources_Count();
}

RwInt32 WorldSpawn_FromFile(const RwChar *pPath, WorldSpawnStats *pStats) {
    s_towerCount = 0;
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
        CollectTower(pObj);
        const RwUInt8 *pRaw = 0;
        const RwUInt32 rawSize = RwStudioFileGetObjectRaw(pFile, &pRaw);
        const RWS::CAttributePacket packet(pRaw, rawSize);
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

    if (pStats) {
        *pStats = stats;
    }
    return totalApplied;
}