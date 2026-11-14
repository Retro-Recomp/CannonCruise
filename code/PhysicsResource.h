#ifndef PHYSICSRESOURCE_H
#define PHYSICSRESOURCE_H

// ============================================================================
// CannonCruise - Physics World Collision Resource (PhysicsResource.h)
// Original path: D:\Projects\CannonCruisePC\code\PhysicsResource.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include <rwcore.h>

class CPhysicsResource {
public:
  static void *Load(const char *pFilename, const char *pType);
  static void Unload(void *pResource);

  static RwUInt32 GetLoadedCount() { return sm_nLoadedResources; }

private:
  static RwUInt32 sm_nLoadedResources;
};

#endif // PHYSICSRESOURCE_H
