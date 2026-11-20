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
#include "Bird.h"
#include "FernandoActor.h"
#include "HealthDebris.h"
#include "MenuBehavior.h"
#include "MissionIndicator.h"
#include "TowerCatapult.h"
#include "CollisionTrigger.h"
#include "framework/core/attributehandler/cattributehandler.h"

#include <cstdio>
#include <cstring>

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

RwInt32 WorldSpawn_FromFile(const RwChar *pPath, WorldSpawnStats *pStats) {
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
        void *pInstances[64];
        const RwInt32 applied = BehaviourLoader::ProcessObject(
            pObj, pInstances,
            (RwInt32)(sizeof(pInstances) / sizeof(pInstances[0])));
        const RwUInt32 createdCount = (applied > 0) ? (RwUInt32)applied : 0;
        if (applied > 0) {
            stats.instancesCreated += createdCount;
            totalApplied += applied;
        }
        // unknown classes: factory miss (binary: property skip)
        if (pObj->behaviourCount > createdCount) {
            stats.instancesSkipped += pObj->behaviourCount - createdCount;
        }
    }
    RwStudioFileClose(pFile);

    if (pStats) {
        *pStats = stats;
    }
    return totalApplied;
}