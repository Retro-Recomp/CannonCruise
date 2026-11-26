#ifndef OCEAN_H
#define OCEAN_H

// ============================================================================
// CannonCruise - Ocean Water Simulation (Ocean.h)
// Original path: D:\Projects\CannonCruisePC\code\Ocean.h
// Reconstructed via reverse engineering with Ghidra and CC.asm analysis.
// ============================================================================

#include "framework/core/eventhandler/ceventhandler.h"
#include <rwcore.h>
#include <rpworld.h>
#include <vector>

#define OCEAN_MAX_SKY_DOMES 4

#include "OceanTile.h"

/**
 * @class COcean
 * @brief Dynamic ocean water simulation and surface mesh system.
 *
 * Implements a tiled water surface with analytical wave height calculation
 * (Gerstner / harmonic sine waves) used both for visual rendering and
 * physical ship buoyancy / projectile splash queries. Also manages sky dome
 * environmental clumps and listens to engine lifecycle messages (iMsgOceanUpdate,
 * iMsgOceanDestroy, MSG_THUNDER0_RENDER).
 */
class COcean : public RWS::CEventHandler
{
public:
    /**
     * @brief Constructs an ocean simulation instance.
     *
     * Clears sky dome pointers, sets default high-detail rendering, and resets wave time.
     */
    COcean();

    /**
     * @brief Destructor – deallocates all child water tiles and destroys sky dome clumps.
     */
    virtual ~COcean();

    /**
     * @brief Framework message dispatcher.
     * @param msg  Incoming message descriptor (iMsgOceanUpdate, iMsgOceanDestroy, etc.).
     */
    virtual void HandleEvents(const RWS::CMsg& msg);

    /**
     * @brief Creates a grid of procedural water tiles spanning the active play area.
     * @param numTilesX   Number of tile partitions along the X axis.
     * @param numTilesZ   Number of tile partitions along the Z axis.
     * @param tileSize    Dimension of each individual square tile in world units.
     * @param originX     Grid corner X (world coordinate — default 0).
     * @param originZ     Grid corner Z (world coordinate — default 0).
     */
    void Initialize(RwInt32 numTilesX, RwInt32 numTilesZ, RwReal tileSize,
                    RwReal originX = 0.0f, RwReal originZ = 0.0f);

    /**
     * @brief Per-frame simulation tick advancing wave phases and tessellation offsets.
     * @param deltaTime  Elapsed frame duration in seconds.
     */
    void Update(RwReal deltaTime);

    /// Water colours from the level's COcean record (ARGB attributes:
    /// deep, base, crest, horizon).
    void SetWaterColors(RwUInt32 deep, RwUInt32 base, RwUInt32 crest, RwUInt32 horizon);

    /// Viewer position for the distance fade toward the horizon colour.
    void SetViewer(RwReal x, RwReal z) { m_Shade.viewX = x; m_Shade.viewZ = z; }

    /// Accumulated wave time (seconds) the tiles are currently drawn with.
    RwReal GetWaveTime() const { return m_fWaveTime; }

    /**
     * @brief Renders all visible ocean surface tiles using Direct3D / RenderWare Im3D.
     */
    void Render();

    /**
     * @brief Queries instantaneous wave elevation and surface normal at specified world coordinates.
     * @param worldX        X coordinate in world space.
     * @param worldZ        Z coordinate in world space.
     * @param[out] pNormal  Optional pointer to receive unit surface normal vector.
     * @return Wave elevation Y in world units.
     */
    RwReal CalculateWaterHeight(RwReal worldX, RwReal worldZ, RwV3d* pNormal = 0) const;

    /// Height of the drawn water mesh at a point (the tiles are coarse, so it
    /// differs from CalculateWaterHeight by up to ~0.5 m); ships ride on this.
    RwReal GetSurfaceHeight(RwReal worldX, RwReal worldZ) const;

    static COcean* GetInstance() { return sm_pInstance; }
    static RwReal GetWaterHeight(RwReal worldX, RwReal worldZ, RwV3d* pNormal = 0)
    {
        if (sm_pInstance) return sm_pInstance->CalculateWaterHeight(worldX, worldZ, pNormal);
        return 0.0f;
    }

private:
    static COcean* sm_pInstance;                        ///< Active ocean instance pointer.
    std::vector<COceanTile*> m_Tiles;                   ///< Collection of subdivided water surface grid tiles.
    RpClump* m_apSkyDomes[OCEAN_MAX_SKY_DOMES];         ///< Environmental sky hemisphere clumps.
    RwUInt32 m_uSkyDomeCount;                           ///< Number of active sky dome models.
    RwReal m_fWaveTime;                                 ///< Accumulated wave animation time in seconds.
    RwBool m_bHiDetail;                                 ///< Flag enabling high-resolution water mesh geometry.
    RwReal m_fGridOriginX, m_fGridOriginZ;              ///< Corner of the tile grid.
    RwReal m_fGridCell;                                 ///< Width of one mesh quad.
    COceanTile::Shade m_Shade;                          ///< Water colours (valid once set).
};

#endif // OCEAN_H
