#ifndef RT2DMAESTROSTREAM_H
#define RT2DMAESTROSTREAM_H

// ============================================================================
// CannonCruise - Rt2dMaestro .anm stream parser (Rt2dMaestroStream.h)
//
// Menu.anm / CAG_Menu.anm binary format (2026-10-10 reverse engineering):
//
//   Header (40B): u32{0, dataSize, hdr2=433, hdr3} + u32 libid(0x1803ffff)
//                 + u32 0 + f32{0, -0, canvasW=640, canvasH=480}
//   @40:  u32 nodeCount
//   Node records (variable): [u32 nameLen][name\0][u32 type][u32 arg]
//     type=1 -> group/scene ("/MusicSoundWave/"), arg = scene/meta id
//     type=3 -> button binding (btnUp...), arg = pad code (e.g. 98=up)
//     type=6 → command (QuitGame, NewSave...), arg = 0xFFFFFFFF
//   Scene section: u32{sizeA, numRecords, sizeB} + libid + ...
// ============================================================================

#include "rwcore.h"

enum Rt2dMaestroNodeType
{
    rwrt2dNODE_GROUP   = 1, ///< "/name/" scene/group node
    rwrt2dNODE_BUTTON  = 3, ///< btnUp/btnCross... — arg = pad kodu
    rwrt2dNODE_COMMAND = 6  ///< QuitGame/NewSave... — arg = -1
};

#define rwrt2dMAX_NODES 64

struct Rt2dMaestroNode
{
    char     name[64];
    RwUInt32 type;  ///< Rt2dMaestroNodeType
    RwUInt32 arg;
};

struct Rt2dMaestroStreamInfo
{
    RwUInt32 field1;         ///< header[1] -- data size-like
    RwUInt32 field2;         ///< header[2] -- 433 in both menus
    RwUInt32 field3;         ///< header[3]
    RwReal   canvasWidth;    ///< 640.0
    RwReal   canvasHeight;   ///< 480.0
    RwUInt32 nodeCount;      ///< parsed node count (<= rwrt2dMAX_NODES)
    Rt2dMaestroNode nodes[rwrt2dMAX_NODES];
    RwUInt32 sceneSectionOffset; ///< scene section relative to .anm start
    RwUInt32 sceneSizeA;     ///< scene section[0]
    RwUInt32 sceneNumRecords;///< scene section[1]
    RwUInt32 sceneSizeB;     ///< scene section[2]
};

/**
 * @brief Parses the .anm stream (header + node table + scene header).
 * @param pData Full .anm payload (EXCLUDING the asset record; the stream itself).
 * @param len   Payload length.
 * @param pInfo Output info structure.
 * @return TRUE success; FALSE corrupt/missing data.
 */
RwBool Rt2dMaestro_Inspect(const void *pData, RwUInt32 len,
                           Rt2dMaestroStreamInfo *pInfo);

/**
 * @brief Finds the embedded .anm stream inside a .txd/.dff asset record.
 *
 * RwStudio 0x0716 record schema: [metaLen][strLen][name][guid16]
 * [idNameLen]["rwID_2DMAESTRO\0"...]... record end = metaLen field + metaLen;
 * the stream starts there.
 *
 * @param pData  Full asset chunk payload (0x0716 header INCLUDED).
 * @param len    Chunk length (payload without header).
 * @param pOff   Output: offset of the .anm stream within the payload.
 * @param pLen   Output: .anm stream length.
 * @return TRUE bulundu.
 */
RwBool Rt2dMaestro_FindAnimInAsset(const void *pData, RwUInt32 len,
                                   RwUInt32 *pOff, RwUInt32 *pLen);

/**
 * @brief Parses the first embedded .anm in the RWS file and creates a maestro.
 *        Layout: the three copies of the 0x0716 walk in main.cpp's menu/map phases
 *        + tests all go through this single path (NULL = not found/corrupt).
 */
struct Rt2dMaestro;
Rt2dMaestro *Rt2dMaestro_LoadFromRwsFile(const char *path);

#endif // RT2DMAESTROSTREAM_H
