// ============================================================================
// CannonCruise - Embedded TXD (d3d8 IMAGE) decode
//
// Binary data source: MAINMENU.RWS 0x0716 asset, embedded payload
// (embeddedType=0x23, libId=0x1803FFFF). Payload layout (Phase 4.6l dump):
//
//   [u16 textureCount][u16 deviceID]                     (4 byte header)
//   repeat textureCount:
//     [u32 flag]                                         (per texture, 1 in file)
//     [id=0x18][len][ver=0x1803FFFF]  raster: nested [id=1][16B:
//         u32 width, u32 height, u32 depth(bpp), u32 pitch] + pixels
//         data (pitch*height bytes; 4-aligned)
//     [id=0x06][len][ver]  texture struct: nested [id=1][4B:
//         filterMode+addressing], [id=2][name], [id=2][maskName];
//         body tail holds the [id=3][0][ver] terminator
//
// Note: 0x18 len = 12(nested hdr) + 16 + pixels + pad; 0x06 len = 68+short name
// (nested total + 12B terminator). Pixel data is d3d8 RGBA (first texture:
// beyaz 0x00FFFFFF).
// ============================================================================

#ifndef CANNONCRUISE_TXDDECODE_H
#define CANNONCRUISE_TXDDECODE_H

#include "rwcore.h"

#define rwtxdMAX_TEXTURES 64
#define rwtxdMAX_NAME 32

/**
 * @struct RwTxdTexture
 * @brief A single decoded d3d8 texture record.
 */
struct RwTxdTexture {
    RwChar name[rwtxdMAX_NAME];     ///< nested id=2 (NUL-terminated)
    RwChar maskName[rwtxdMAX_NAME]; ///< nested id=2 (may be empty)
    RwUInt32 filterFlags;           ///< nested id=1 (filter+addressing)
    RwUInt32 width;                 ///< raster nested: width
    RwUInt32 height;                ///< raster nested: height
    RwUInt32 depth;                 ///< raster nested: bpp (32)
    RwUInt32 pitch;                 ///< raster nested: row bytes
    const RwUInt8 *pPixels;         ///< pixel data (pointer into the payload)
    RwUInt32 pixelBytes;            ///< pitch*height
};

/**
 * @struct RwTxdImage
 * @brief Decoded texture dictionary (d3d8 IMAGE).
 */
struct RwTxdImage {
    RwUInt16 textureCount;
    RwUInt16 deviceId;
    RwUInt32 count;
    RwTxdTexture textures[rwtxdMAX_TEXTURES];
};

/**
 * @brief Decodes the embedded TXD payload.
 *
 * Chunk boundaries and nested structures are validated byte by byte; on corrupt data
 * a partial result + FALSE is returned.
 *
 * @param pPayload Embedded payload (RwStudioAssetReadEmbedded output).
 * @param len      Payload boyutu (embeddedLength).
 * @param pOut     Output (texture list + pixel pointers).
 * @return TRUE if the header + all textures were read successfully.
 */
RwBool RwTxdDecodeImage(const RwUInt8 *pPayload, RwUInt32 len,
                        RwTxdImage *pOut);

/**
 * @brief The UI (GUI_/compass_/fishbasic) textures inside the level texture pack
 *        records (d3d8-IMAGE nested format; located at the tail of the level
 *        chunk0).
 *
 * Record: [id=6][len][ver=0x1803FFFF] → [id=1][4B filter] →
 *        [id=2][len][ver] name → [id=2]mask → [id=3] terminator →
 *        [u32 flag]? → [id=0x18][len][ver] raster + id=1 dims + pixels,
 *        followed by the mip chain (id=0x18...). Only the main mip is taken.
 * pOut->textures[i].pPixels points into the input buffer (must remain alive).
 */
RwBool RwTxdDecodeGuiPack(const RwUInt8 *pData, RwUInt32 len,
                          RwTxdImage *pOut);

/**
 * @brief Level texture pack decode (VIKING01 chunk0 special format).
 *
 * Records: [06 11][02][len][libid]<name> … tokens …
 * [01][0x10][libid]<w,h,depth,pitch> + pixels (pitch*h, 32bpp RGBA).
 * pPixels points into the input buffer (must remain alive until upload completes).
 */
RwBool RwTxdDecodeLevelPack(const RwUInt8 *pData, RwUInt32 len,
                            RwTxdImage *pOut);

#endif // CANNONCRUISE_TXDDECODE_H
