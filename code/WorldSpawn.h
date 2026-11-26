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
    /// HAnim clips of the ship ("Wing_Left_tag100.anm" -> node 100).
    struct Clip {
        const void *pAnim;   ///< const RwsResource* (rwsRESOURCE_HANIM)
        RwInt32 nodeId;      ///< from the "_tagNNN" name suffix
    } clips[16];
    RwUInt32 clipCount;
};

/// Player ship of the last WorldSpawn_FromFile() call.
const WorldSpawnPlayerShip *WorldSpawn_GetPlayerShip(void);

#define WORLDSPAWN_MAX_ENEMY_CLIPS 8

/// Enemy ship placement from a CShipEnemy record.
struct WorldSpawnEnemyShip {
    RwReal matrix[16];       ///< start transform (RwMatrix)
    const void *pModel;      ///< const RwsResource*: hull model (e.g. Viking_Destroyer.dff)
    RwChar route[64];        ///< CShipEnemy id8: CWaypoint name it sails to ("" = none)
    RwBool hasWaypoint;      ///< route resolved to a CWaypoint record
    RwReal waypoint[3];      ///< that waypoint's position (CSystemCommands matrix)
    /// HAnim clips (Viking_Destroyer: paddles 100/200, rudder 300, catapult 400/500).
    WorldSpawnPlayerShip::Clip clips[WORLDSPAWN_MAX_ENEMY_CLIPS];
    RwUInt32 clipCount;
    RwChar spawnEvent[64];   ///< CSpawnable id1: appears on this event ("" = at start)
    RwChar deathEvent[64];   ///< CShipEnemy id17: sent when sunk ("" = none)
};

/// Collectible fish (CFishGreen record).
struct WorldSpawnFish {
    RwReal pos[3];
    const void *pModel;      ///< const RwsResource* (may be NULL)
    RwChar spawnEvent[64];   ///< CSpawnable id1 ("" = present at start)
    RwChar pickupEvent[64];  ///< CFish id12: sent when collected
};

#define WORLDSPAWN_MAX_FISH 64

/// Crew on the player's ship (CHugoActor / CFernandoActor records).
struct WorldSpawnActor {
    const void *pModel;      ///< const RwsResource* (Hugo.dff / fernando_Anim.dff)
    const void *clips[24];   ///< const RwsResource* animations (full skeleton)
    RwUInt32 clipCount;
};

/// Hugo (index 0) and Fernando (index 1); pModel is NULL when absent.
const WorldSpawnActor *WorldSpawn_GetActor(RwUInt32 index);

/// Fish of the last WorldSpawn_FromFile() call; returns the count.
RwUInt32 WorldSpawn_GetFish(const WorldSpawnFish **ppFish);

#define WORLDSPAWN_MAX_ENEMY_SHIPS 64

/// Enemy ships of the last WorldSpawn_FromFile() call; returns the count.
RwUInt32 WorldSpawn_GetEnemyShips(const WorldSpawnEnemyShip **ppShips);

#define WORLDSPAWN_MAX_TOWERS 64

/**
 * @brief Towers found by the last WorldSpawn_FromFile() call.
 * @param ppTowers Receives a pointer to the internal array (valid until the next call).
 * @return Number of towers (0 when the level has none).
 */
RwUInt32 WorldSpawn_GetTowers(const WorldSpawnTower **ppTowers);

#endif // CANNONCRUISE_WORLDSPAWN_H
