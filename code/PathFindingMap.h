#ifndef PATHFINDINGMAP_H
#define PATHFINDINGMAP_H

// ============================================================================
// CannonCruise - Pathfinding Map (PathFindingMap.h)
// Original path: D:\Projects\CannonCruisePC\code\PathFindingMap.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// Binary ref: FUN_004740c0 - FUN_004744b0 (CPathFindingMap methods)
// ============================================================================

#include "Astar.h"
#include <rwcore.h>
#include <vector>

/**
 * @class CPathFindingMap
 * @brief Grid map storing the walkable/blocked cells of the level.
 *
 * Read from a binary file (`*.pfm`) during level load; for each cell the
 * blocked/free state is kept in the `m_Grid` byte array.
 * CAstar uses this map to perform NPC pathfinding.
 *
 * **File format:**
 * - 4 bytes: width (RwInt32)
 * - 4 bytes: height (RwInt32)
 * - width × height bytes: cell state (0 = free, 1 = blocked)
 *
 * **Binary references:**
 * - Ctor: `FUN_004740c0`
 * - Load: `FUN_00474100`
 * - IsCellBlocked: `FUN_00474340`
 */
class CPathFindingMap
{
public:
    /**
     * @brief Default constructor — initializes an empty map.
     */
    CPathFindingMap();

    /**
     * @brief Destructor — releases the grid memory.
     */
    ~CPathFindingMap();

    /**
     * @brief Loads the pathfinding map from a binary file.
     * @param pFilename Path of the `.pfm` file to load.
     * @return RwBool — TRUE if loaded successfully.
     */
    RwBool Load(const char* pFilename);

    /**
     * @brief Clears the map data from memory.
     */
    void Unload();

    /**
     * @brief Returns whether the given grid cell is blocked.
     * @param x X coordinate (column) to check.
     * @param z Z coordinate (row) to check.
     * @return RwBool — TRUE if this cell is blocked.
     */
    RwBool IsCellBlocked(RwInt32 x, RwInt32 z) const;

    /**
     * @brief Returns the column count of the map.
     * @return RwInt32 — Grid width.
     */
    RwInt32 GetWidth() const { return m_iWidth; }

    /**
     * @brief Returns the row count of the map.
     * @return RwInt32 — Grid height.
     */
    RwInt32 GetHeight() const { return m_iHeight; }

private:
    RwInt32 m_iWidth;              ///< Grid column count
    RwInt32 m_iHeight;             ///< Grid row count
    std::vector<RwUInt8> m_Grid;   ///< Cell states (0=free, 1=blocked)
    CAstar m_Astar;                ///< A* instance working on this map
};

#endif // PATHFINDINGMAP_H
