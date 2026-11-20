// ============================================================================
// CannonCruise - Phase 4.6p: world spawn chain from 0x0704 object records
// Binary: CEventHandler::Process loop (FUN_00411050) — class name ->
// CClassFactory::Create -> apply virtual (vtable slot1).
//
// This module does two things:
//   1. GameBehaviours_Register(): registers known behaviour classes
//      with BehaviourLoader (render/camera/audio/physics ports).
//   2. WorldSpawn_FromFile(): walks all OBJECTRECORD records in RWS
//      file and creates instances via ProcessObject; unknown
//      classes silently skipped as in binary.
//
// Ownership: created instances live until process end (in binary, scene
// teardown calls CClassFactory delete — port consciously leaks until 4.6s).
// ============================================================================

#ifndef CANNONCRUISE_WORLDSPAWN_H
#define CANNONCRUISE_WORLDSPAWN_H

#include "rwcore.h"

/// WORLDSPAWN statistics (RWS walk output)
struct WorldSpawnStats {
    RwUInt32 objectRecords;    ///< walked 0x0704 records
    RwUInt32 instancesCreated; ///< Create() successful (known classes)
    RwUInt32 instancesSkipped; ///< factory missing -> binary: property skip
};

/**
 * @brief Registers known behaviour classes (binary Register pattern).
 *
 * CWbRenderBehavior / CWbCameraBehavior / CAudioStreamEntity /
 * CPhysicsWorldBehaviour — apply adaptors wired to 4.6h-j ports.
 * @return TRUE registration successful (repeatable — call UnregisterAll first).
 */
RwBool GameBehaviours_Register();

/**
 * @brief Registers the RWS file's embedded clumps in RwsResources (by GUID).
 *        Called by WorldSpawn_FromFile before objects are created.
 * @return Number of registered resources.
 */
RwUInt32 WorldSpawn_RegisterResources(const RwChar *pPath);

/**
 * @brief Creates+applies all 0x0704 records in RWS file.
 * @param pPath  RWS file path.
 * @param pStats Statistics (optional).
 * @return Number of behaviours applied (-1: file open failed).
 */
RwInt32 WorldSpawn_FromFile(const RwChar *pPath, WorldSpawnStats *pStats);

/// Tower placement read from a CTowerCatapult object record.
struct WorldSpawnTower {
    RwV3d pos;               ///< CSystemCommands id1 RwMatrix translation
    RwChar missionEvent[48]; ///< CTowerCatapult id3 (e.g. "MissionTower1Dead"); "" if none
    RwReal matrix[16];       ///< full RwMatrix (right/up/at/pos)
    const void *pModel;      ///< const RwsResource*: intact model (NULL if none)
    const void *pDamaged;    ///< const RwsResource*: "_Damaged" model (NULL if none)
};

/// Player ship from the level's CShipPlayer record.
struct WorldSpawnPlayerShip {
    RwBool valid;            ///< a CShipPlayer record was found
    RwReal matrix[16];       ///< start transform (RwMatrix)
    const void *pModel;      ///< const RwsResource*: ship model ("Hugo_Ship")
};

/// Player ship of the last WorldSpawn_FromFile() call.
const WorldSpawnPlayerShip *WorldSpawn_GetPlayerShip(void);

#define WORLDSPAWN_MAX_TOWERS 64

/**
 * @brief Towers found by the last WorldSpawn_FromFile() call.
 * @param ppTowers Receives a pointer to the internal array (valid until the next call).
 * @return Number of towers (0 when the level has none).
 */
RwUInt32 WorldSpawn_GetTowers(const WorldSpawnTower **ppTowers);

#endif // CANNONCRUISE_WORLDSPAWN_H
