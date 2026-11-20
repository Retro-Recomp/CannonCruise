// ============================================================================
// CannonCruise - BehaviourLoader implementation
// Binary: FUN_00411050 (Process) + CClassFactory::Create + apply virtuals
// ============================================================================

#include "BehaviourLoader.h"

#include "attributehandler/cclassfactory.h"
#include "attributehandler/cattributehandler.h"

#include <cstring>

namespace {
const RwInt32 kMaxClasses = 32; // room for 25+ behavior records in the .RWS
BehaviourClassEntry s_Entries[kMaxClasses];
RwInt32 s_EntryCount = 0;
} // namespace

const BehaviourClassEntry *BehaviourLoader::sm_pEntries = s_Entries;
RwInt32 BehaviourLoader::sm_entryCount = 0;

RwBool BehaviourLoader::Register(const BehaviourClassEntry *pEntry) {
    if (!pEntry || !pEntry->className || !pEntry->create || !pEntry->apply) {
        return FALSE;
    }
    if (s_EntryCount >= kMaxClasses) {
        return FALSE;
    }
    s_Entries[s_EntryCount] = *pEntry;
    ++s_EntryCount;
    sm_entryCount = s_EntryCount;

    // In binary, create is registered with CClassFactory (FUN_00413700 Register)
    RWS::CClassFactory::Register(
        pEntry->className,
        (RWS::ClassCreationFunc)pEntry->create, pEntry->size, 1);
    return TRUE;
}

void BehaviourLoader::UnregisterAll() {
    for (RwInt32 i = 0; i < s_EntryCount; ++i) {
        RWS::CClassFactory::UnRegister(s_Entries[i].className);
    }
    s_EntryCount = 0;
    sm_entryCount = 0;
}

void *BehaviourLoader::Create(const char *pClassName) {
    if (!pClassName) {
        return 0;
    }
    return RWS::CClassFactory::Create(pClassName);
}

RwInt32 BehaviourLoader::ProcessObject(const RwStudioObject *pObject,
                                       void **ppInstances,
                                       RwInt32 maxInstances,
                                       const RWS::CAttributePacket *pPacket) {
    if (!pObject) {
        return -1;
    }
    if (pObject->behaviourCount == 0) {
        return 0;
    }
    // One record = one entity. Its class is the first (0x20000000) segment;
    // the other blocks (CSystemCommands, CAttributeHandler, base classes,
    // components such as CCannon) only carry that entity's attributes.
    const char *pOwner = pObject->behaviours[0].className;
    void *pInstance = (pPacket != 0)
                          ? RWS::CClassFactory::Create(pOwner, *pPacket)
                          : Create(pOwner);
    if (!pInstance) {
        return 0; // unknown class: binary skips the record's properties
    }
    // apply virtual (vtable slot1): the owner's properties may be split over
    // several blocks of the same name (e.g. CTowerCatapult around
    // CSystemCommands), so apply all of them.
    for (RwUInt32 i = 0; i < s_EntryCount; ++i) {
        if (std::strcmp(s_Entries[i].className, pOwner) != 0) {
            continue;
        }
        for (RwUInt32 b = 0; b < pObject->behaviourCount; ++b) {
            const RwStudioBehaviour *pBeh = &pObject->behaviours[b];
            if (std::strcmp(pBeh->className, pOwner) != 0) {
                continue;
            }
            for (RwUInt32 p = 0; p < pBeh->propCount; ++p) {
                const RwStudioProperty &prop = pBeh->props[p];
                // Apply adaptors were written for scalar/vec3 records only;
                // longer records (matrices, strings) go through the packet.
                if (prop.size > 20) {
                    continue;
                }
                s_Entries[i].apply(pInstance, prop.id, prop.value);
            }
        }
        break;
    }
    if (ppInstances && maxInstances > 0) {
        ppInstances[0] = pInstance;
    }
    return 1;
}
