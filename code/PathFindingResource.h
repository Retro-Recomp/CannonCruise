#ifndef PATHFINDINGRESOURCE_H
#define PATHFINDINGRESOURCE_H

// ============================================================================
// CannonCruise - Pathfinding Resource Loader (PathFindingResource.h)
// Original path: D:\Projects\CannonCruisePC\code\PathFindingResource.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "PathFindingMap.h"
#include <rwcore.h>

class CPathFindingResource
{
public:
    static void* Load(const char* pFilename, const char* pType);
    static void Unload(void* pResource);
};

#endif // PATHFINDINGRESOURCE_H
