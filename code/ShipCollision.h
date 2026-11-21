#ifndef CANNONCRUISE_SHIPCOLLISION_H
#define CANNONCRUISE_SHIPCOLLISION_H

// ============================================================================
// CannonCruise - Ship collision (ShipCollision.h)
//
// Keeps ships out of land and solid objects. The level's world geometry is
// rasterised once into an XZ grid holding the lowest surface height in each
// cell: a cell whose lowest surface is above the waterline is land (beach,
// cliff, pier); open water has no surface or only the sea floor below it.
// Objects (towers, enemy ships) are circles supplied each frame.
// ============================================================================

#include "rwcore.h"

/// Builds the land map from the world clump (RpClump*); returns the number of
/// land cells (0 = no map: nothing blocks).
RwInt32 ShipCollision_BuildLandMap(void *pWorldClump, RwReal waterY);

/// Frees the land map and obstacles.
void ShipCollision_Clear(void);

/// Replaces the obstacle circles (x, z, radius triples).
void ShipCollision_SetObstacles(const RwReal *pXZR, RwInt32 count);

/// TRUE if a hull point at (x, z) would be inside land or an obstacle.
RwBool ShipCollision_IsBlocked(RwReal x, RwReal z);

#endif // CANNONCRUISE_SHIPCOLLISION_H
