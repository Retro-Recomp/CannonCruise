// ============================================================================
// CannonCruise - Physics World Collision Resource (PhysicsResource.cpp)
// Original path: D:\Projects\CannonCruisePC\code\PhysicsResource.cpp
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
//
// Binary functions:
//   FUN_00438f80 — CPhysicsResource::Load
//   FUN_00439630 — CPhysicsResource::Unload
// ============================================================================

#include "PhysicsResource.h"
#include <cassert>
#include <cstring>

RwUInt32 CPhysicsResource::sm_nLoadedResources = 0;

void* CPhysicsResource::Load(const char* pFilename, const char* pType)
{
    assert(pFilename != 0);
    assert(pType != 0 && "Failed PRE-condition: pacType");
    assert(sm_nLoadedResources < 16 && "m_nLoadedResources < 16 Failed");

    // read static world collision data from the wbID_STATICWORLDCOLLISION stream
    sm_nLoadedResources++;
    return 0;
}

void CPhysicsResource::Unload(void* pResource)
{
    if (pResource && sm_nLoadedResources > 0)
    {
        sm_nLoadedResources--;
    }
}
