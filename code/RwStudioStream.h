// ============================================================================
// CannonCruise - rwstudio Container Reader (RwStudioStream.h)
// Original path: D:\Projects\CannonCruisePC\code\RwStudioStream.h
//
// Streams/*.RWS = the "rwstudio" (WaveBreaker asset pipeline) custom container.
// File = sequence of top-level chunks (standard RW 12-byte header, libid
// 0x1802FFFF = RW 3.6.0.2):
//   0x0716 = asset record — meta (typeName/rwId/sourcePath/GUID) + embedded
//            standard RW chunk (e.g. txd: type=0x23, libid=0x1803FFFF)
//   0x0704 = object/behaviour record (CWb* class names + property list)
//   0x071C = file class table (manifest: class name + version)
// Binary connections: CPhysicsResource::Load (sub_439460) reads the
// COLLISION chunk of wbID_STATICWORLD- from this container; CClassFactory/CEventHandler
// operate on 0x071C/0x0704 class names.
//
// String rules (verified with hexdump):
//   - length-prefixed: [u32 len][len bytes; '\0' + 0xBF padding, 4-aligned]
//   - inside manifest: null-terminated + 0xBF padding (no length)
// ============================================================================

#ifndef RWSTUDIOSTREAM_H
#define RWSTUDIOSTREAM_H

#include "rwcore.h"

// Top-level chunk types (game plugin range — like MAKEPLUGINID)
#define rwstudioID_ASSETRECORD 0x0716
#define rwstudioID_OBJECTRECORD 0x0704
#define rwstudioID_MANIFEST 0x071C

// Embedded types (standard RW chunk type inside the asset record)
#define rwstudioEMBED_TEXDICT 0x0023 // rwID_TEXDICTIONARY (this RW version)

#define rwstudioMAX_NAME 64
#define rwstudioMAX_PATH 256
#define rwstudioMAX_CLASSES 64
#define rwstudioMAX_BEHAVIOURS 8
#define rwstudioMAX_PROPS 32

// 0x0704 object property schema (Phase 4.6f — MAINMENU obj0/obj1/obj2
// hexdump): object = [u32 field0] + repeated segments/properties.
//   segment: [u32 segLen][u32 flags][content]
//     flags 0x20000000 = class name (behaviour start)
//     flags 0x40000000 = GUID (16B)
//     flags 0x80000000 = class name repetition
//     flags 0x00000000 = string value (e.g. "Audio_MenuMusic")
//   property: [u32 size][u32 id][size-8 bytes value]
//     size 12 = u32/float/inline-string; size 20 = 3 float (vec3)
//     size 72 = RwMatrix (CSystemCommands id1: entity transform)
//     other sizes = padded strings (e.g. CTowerCatapult id3 "MissionTower1Dead")
//   Segments and properties share the [len][word][len-8 bytes] layout; the
//   second word tells them apart (segment flags vs property id).
#define rwstudioMAX_PROPVALUE 64

struct RwStudioProperty {
    RwUInt32 id;
    RwUInt32 size;   // total record (8 + value bytes); value truncated to 64
    RwUInt8 value[rwstudioMAX_PROPVALUE];
};

struct RwStudioBehaviour {
    RwChar className[48];
    RwChar name[48]; // flags=0 string segment (empty otherwise)
    RwUInt8 guid[16];
    RwUInt32 propCount;
    RwStudioProperty props[rwstudioMAX_PROPS];
};

struct RwStudioObject {
    RwUInt32 objectIndex; // first u32 (0 in all examples — meaning unclear)
    RwUInt32 behaviourCount;
    RwStudioBehaviour behaviours[rwstudioMAX_BEHAVIOURS];
};

struct RwStudioAsset {
    RwUInt32 chunkType;  // rwstudioID_*
    RwUInt32 chunkLength;

    // 0x0716 meta (empty for other types)
    RwChar typeName[rwstudioMAX_NAME];   // e.g. "Texture Dictionary"
    RwChar rwIdName[rwstudioMAX_NAME];   // e.g. "rwID_TEXDICTIONARY"
    RwChar sourcePath[rwstudioMAX_PATH]; // source .txd/.dff/.col path
    RwUInt8 guid[16];

    // embedded RW chunk (absent if embeddedType==0)
    RwUInt32 embeddedType;   // 0x23 = txd
    RwUInt32 embeddedLength; // total embedded chunk length (including header)
    RwUInt32 embeddedLibId;  // 0x1803FFFF = RW 3.6.0.3

    // resource payload WITHOUT embed (e.g. wbID_STATICWORLDCOLLISION .col):
    // bytes from meta+resource/va/embA/embB to the chunk end.
    // embeddedType!=0 iken 0.
    RwUInt32 resourceLength;
};

struct RwStudioFile;

RwStudioFile *RwStudioFileOpen(const RwChar *pPath);
void RwStudioFileClose(RwStudioFile *pFile);

// Reads the next top-level chunk (all types). The position in the stream is set
// to after the chunk end; it MUST be called BEFORE reading an embedded payload via
// ReadEmbedded (the payload comes after the chunk; if unread,
// NextAsset skips it).
RwBool RwStudioFileNextAsset(RwStudioFile *pFile, RwStudioAsset *pAsset);

// Reads the embedded payload of the current asset (while embeddedType!=0).
// If maxLen is larger than the payload size, the excess is skipped (stream alignment is preserved).
RwUInt32 RwStudioAssetReadEmbedded(RwStudioFile *pFile,
                                   const RwStudioAsset *pAsset, void *pBuf,
                                   RwUInt32 maxLen);

// Reads the resource payload of the current asset WITHOUT embed
// (while resourceLength>0; see STATICWORLDCOLLISION .col).
RwUInt32 RwStudioAssetReadResource(RwStudioFile *pFile,
                                   const RwStudioAsset *pAsset, void *pBuf,
                                   RwUInt32 maxLen);

// Class list after the 0x071C manifest is seen (gap: 0).
RwUInt32 RwStudioFileGetClassCount(const RwStudioFile *pFile);
const RwChar *RwStudioFileGetClassName(const RwStudioFile *pFile,
                                       RwUInt32 index);

// Valid object schema after NextAsset returns 0x0704 (file-internal
// buffer; overwritten by the next NextAsset). NULL for other types.
const RwStudioObject *RwStudioFileGetObject(const RwStudioFile *pFile);

/**
 * @brief Raw body of the last 0x0704 object record (its attribute packet).
 * @param ppData Receives a pointer valid until the next NextAsset/Close.
 * @return Byte count (0 when the last chunk was not an object record).
 */
RwUInt32 RwStudioFileGetObjectRaw(const RwStudioFile *pFile,
                                  const RwUInt8 **ppData);

#endif // RWSTUDIOSTREAM_H
