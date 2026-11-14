// ============================================================================
// CannonCruise - rwstudio Container Reader (RwStudioStream.cpp)
// Original path: D:\Projects\CannonCruisePC\code\RwStudioStream.cpp
//
// Format evidence: Streams/MAINMENU.RWS + GLOBAL.RWS + VIKING01.RWS hexdump
// (Phase 4.6e). Example — MAINMENU 0x0716 #0 (txd):
//   [12B RW header 0x0716/libid 0x1802FFFF]
//   [u32 metaLen=0xC8][typeName][GUID16][rwIdName][sourcePath]
//   [u32 4][0xBF×3]                       <- meta tail (inside metaLen)
//   [u32 resource][u32 va]                 <- rwstudio inner fields
//   [u32 embType=0x23][u32 embLen][u32 libid 0x1803FFFF]
//   [embedded payload: embLen-12 bytes]
//     payload: i16 textureCount, i16 device=1 (d3d8), IMAGE chunks...
// embLen, from the embType field to the chunk end (including 12B header):
// metaLen+20+embLen == chunkLength verified.
// Non-embed (WorldGeometry/.dff, StaticCollision/.col): embType=0;
// .col data inline inside the chunk (CPhysicsResource::Load reads this field).
// ============================================================================

#include "RwStudioStream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

struct RwStudioFile {
    RwStream *pStream;
    RwUInt32 pendingPayload;  // embedded payload byte count (if unread)
    RwUInt32 embedPayloadPos; // file position of the embedded payload
    RwUInt32 chunkEndPos;     // end of the current chunk (absolute)
    RwUInt32 classCount;
    RwChar classNames[rwstudioMAX_CLASSES][48];
    RwBool hasObject;         // was the last NextAsset a 0x0704?
    RwStudioObject object;    // last object schema (Phase 4.6f)
};

namespace
{

// length-prefixed string: [u32 len][len bytes; '\0'+0xBF padding, 4-aligned]
RwBool ReadLenString(RwStream *pStream, RwChar *pOut, RwUInt32 maxOut) {
    RwUInt32 len = 0;
    if (RwStreamRead(pStream, &len, 4) != 4 || len == 0 || len > 512) {
        return FALSE;
    }
    RwChar buf[512];
    if (RwStreamRead(pStream, buf, len) != len) {
        return FALSE;
    }
    RwUInt32 n = 0;
    while (n < len && n < maxOut - 1 && buf[n] != '\0') {
        pOut[n] = buf[n];
        ++n;
    }
    pOut[n] = '\0';
    return TRUE;
}

// manifest string: null-terminated + 0xBF padding (4-aligned), no length
RwBool ReadPadString(RwStream *pStream, RwChar *pOut, RwUInt32 maxOut,
                     RwUInt32 *pConsumed) {
    RwChar buf[64];
    RwUInt32 n = 0;
    while (n < sizeof(buf)) {
        if (RwStreamRead(pStream, &buf[n], 1) != 1) {
            return FALSE;
        }
        if (buf[n] == '\0') {
            ++n;
            break;
        }
        ++n;
    }
    while (n % 4 != 0) {
        RwUInt8 pad = 0;
        if (RwStreamRead(pStream, &pad, 1) != 1) {
            return FALSE;
        }
        if (pad != 0xBF && pad != 0x00) {
            return FALSE;
        }
        ++n;
    }
    RwUInt32 copy = n < maxOut - 1 ? n : maxOut - 1;
    RwUInt32 i = 0;
    for (; i < copy && buf[i] != '\0'; ++i) {
        pOut[i] = buf[i];
    }
    pOut[i] = '\0';
    if (pConsumed) {
        *pConsumed = n;
    }
    return TRUE;
}

} // namespace

RwStudioFile *RwStudioFileOpen(const RwChar *pPath) {
    if (!pPath) {
        return 0;
    }
    RwStream *pStream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, pPath);
    if (!pStream) {
        return 0;
    }
    RwStudioFile *pFile = new RwStudioFile();
    pFile->pStream = pStream;
    pFile->pendingPayload = 0;
    pFile->embedPayloadPos = 0;
    pFile->chunkEndPos = 0;
    pFile->classCount = 0;
    pFile->hasObject = FALSE;
    return pFile;
}

void RwStudioFileClose(RwStudioFile *pFile) {
    if (!pFile) {
        return;
    }
    if (pFile->pStream) {
        RwStreamClose(pFile->pStream, 0);
    }
    delete pFile;
}

RwBool RwStudioFileNextAsset(RwStudioFile *pFile, RwStudioAsset *pAsset) {
    if (!pFile || !pAsset) {
        return FALSE;
    }
    // if the previous asset's embedded payload was not read, skip to the chunk end
    if (pFile->pendingPayload > 0) {
        RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
        pFile->pendingPayload = 0;
    }
    std::memset(pAsset, 0, sizeof(*pAsset));
    pFile->hasObject = FALSE;

    // chunk start + end: seek-based alignment (format variants
    // are safely skipped by absolute position instead of byte counting)
    const RwUInt32 chunkStart = RwStreamTell(pFile->pStream);

    RwChunkHeaderInfo header;
    std::memset(&header, 0, sizeof(header));
    if (!RwStreamReadChunkHeaderInfo(pFile->pStream, &header)) {
        if (std::getenv("RWSTUDIO_DEBUG")) {
            std::fprintf(stderr, "NextAsset EOF @tell=%u\n",
                         RwStreamTell(pFile->pStream));
        }
        return FALSE; // EOF
    }
    if (header.type == 0) {
        return FALSE;
    }
    pAsset->chunkType = header.type;
    pAsset->chunkLength = header.length;
    pFile->chunkEndPos = chunkStart + 12 + header.length;

    if (header.type == rwstudioID_ASSETRECORD) {
        RwUInt32 metaLen = 0;
        if (RwStreamRead(pFile->pStream, &metaLen, 4) != 4 || metaLen < 8 ||
            metaLen > 4096) {
            return FALSE;
        }
        if (!ReadLenString(pFile->pStream, pAsset->typeName,
                           rwstudioMAX_NAME)) {
            return FALSE;
        }
        if (RwStreamRead(pFile->pStream, pAsset->guid, 16) != 16) {
            return FALSE;
        }
        if (!ReadLenString(pFile->pStream, pAsset->rwIdName,
                           rwstudioMAX_NAME)) {
            return FALSE;
        }
        if (!ReadLenString(pFile->pStream, pAsset->sourcePath,
                           rwstudioMAX_PATH)) {
            return FALSE;
        }
        // meta tail: in standard assets [u32 4][0xBF×3+0x00]; in variants like
        // (rwID_2DMAESTRO) this is MISSING — the fourth string completes meta.
        // Tolerant rule: if the next u32==4, pad4 + standard field;
        // otherwise it is the resource itself (va/A/B don't shift — in both
        // branches the total is metaLen+16, +4 libid for embeds).
        RwUInt32 tail = 0;
        if (RwStreamRead(pFile->pStream, &tail, 4) != 4) {
            return FALSE;
        }
        if (tail == 4) {
            RwUInt8 pad[4];
            if (RwStreamRead(pFile->pStream, pad, 4) != 4) {
                return FALSE;
            }
        }
        // [u32 resource][u32 va][u32 A][u32 B] (+ [u32 libid] if embed)
        // — txd: [res][va][0x23][embLen][libid][payload]
        //      wbID_Texture: [res][len][pngPath]; 2DMAESTRO/PPB: their own
        //      streams. Embed is DEFINED by identity: no A/B variant
        //      interpretation is needed; others are skipped by seeking to the chunk end.
        RwUInt32 resource = 0, va = 0;
        if (RwStreamRead(pFile->pStream, &resource, 4) != 4 ||
            RwStreamRead(pFile->pStream, &va, 4) != 4) {
            RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
            return TRUE;
        }
        (void)resource;
        (void)va;
        RwUInt32 embA = 0, embB = 0;
        if (RwStreamRead(pFile->pStream, &embA, 4) != 4 ||
            RwStreamRead(pFile->pStream, &embB, 4) != 4) {
            RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
            return TRUE;
        }
        if (embB >= 12 && metaLen + 20 + embB == header.length) {
            pAsset->embeddedType = embA;
            pAsset->embeddedLength = embB;
            if (RwStreamRead(pFile->pStream, &pAsset->embeddedLibId, 4) !=
                4) {
                RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
                return TRUE;
            }
            // embB is the classic RW convention: payload size (excluding the 12B header)
            pFile->embedPayloadPos = RwStreamTell(pFile->pStream);
            pFile->pendingPayload = embB;
        } else {
            // no embed — resource payload runs to the chunk end (e.g. .col):
            // record it + if not consumed via pendingPayload, the next NextAsset skips it
            const RwUInt32 pos = RwStreamTell(pFile->pStream);
            pFile->embedPayloadPos = pos;
            if (pFile->chunkEndPos > pos) {
                pAsset->resourceLength = pFile->chunkEndPos - pos;
                pFile->pendingPayload = pAsset->resourceLength;
            } else {
                RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
            }
        }
        return TRUE;
    }

    if (header.type == rwstudioID_MANIFEST) {
        // [u32 count][entry: (string+0xBF pad) + u32 version]...[tail]
        // — even on count/entry errors, alignment is guaranteed by seeking to the chunk end
        RwUInt32 count = 0;
        if (RwStreamRead(pFile->pStream, &count, 4) == 4 && count <= 256) {
            for (RwUInt32 i = 0; i < count; ++i) {
                RwChar name[48];
                if (!ReadPadString(pFile->pStream, name, sizeof(name), 0)) {
                    break;
                }
                RwUInt32 version = 0;
                if (RwStreamRead(pFile->pStream, &version, 4) != 4) {
                    break;
                }
                if (i < rwstudioMAX_CLASSES) {
                    std::strncpy(pFile->classNames[i], name,
                                 sizeof(pFile->classNames[0]) - 1);
                    pFile->classNames[i][sizeof(pFile->classNames[0]) - 1] =
                        '\0';
                }
            }
            pFile->classCount =
                count < rwstudioMAX_CLASSES ? count : rwstudioMAX_CLASSES;
        }
        RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
        return TRUE;
    }

    if (header.type == rwstudioID_OBJECTRECORD) {
        // 0x0704: [u32 field0] + segment/property array (see header
        // schema). Chunk end is guaranteed via seek; a parse
        // error silently leaves a half object behind.
        RwStudioObject &obj = pFile->object;
        std::memset(&obj, 0, sizeof(obj));
        if (RwStreamRead(pFile->pStream, &obj.objectIndex, 4) != 4) {
            RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
            return TRUE;
        }
        RwStudioBehaviour *pCur = 0;
        // 0x80 behaviours: name repetition in the first behaviour (after 0x20+GUID),
        // the next 0x80 = NEW behaviour (single-segment name — no GUID).
        // State: afterStart = after 0x20 (GUID pending);
        //        afterGuid = after GUID (name repetition pending).
        int state = 0;
        while (RwStreamTell(pFile->pStream) + 8 <= pFile->chunkEndPos) {
            RwUInt32 x = 0;
            if (RwStreamRead(pFile->pStream, &x, 4) != 4) {
                break;
            }
            if (x == 0x1C || x == 0x18) {
                // segment: [segLen][flags][content]
                RwUInt32 flags = 0;
                if (RwStreamRead(pFile->pStream, &flags, 4) != 4) {
                    break;
                }
                if (flags == 0x20000000) {
                    if (obj.behaviourCount >= rwstudioMAX_BEHAVIOURS) {
                        break;
                    }
                    pCur = &obj.behaviours[obj.behaviourCount++];
                    std::memset(pCur, 0, sizeof(*pCur));
                    ReadPadString(pFile->pStream, pCur->className,
                                  sizeof(pCur->className), 0);
                    state = 1; // wait for GUID
                } else if (flags == 0x40000000) {
                    if (pCur) {
                        RwStreamRead(pFile->pStream, pCur->guid, 16);
                    }
                    state = 2; // wait for name repetition
                } else if (flags == 0x80000000) {
                    RwChar name[48];
                    ReadPadString(pFile->pStream, name, sizeof(name), 0);
                    if (state == 2 && pCur &&
                        std::strcmp(name, pCur->className) == 0) {
                        // name repetition — the name was already read at 0x20; validate
                        state = 3;
                    } else {
                        // new behaviour (different name — repeated record without GUID)
                        if (obj.behaviourCount >= rwstudioMAX_BEHAVIOURS) {
                            break;
                        }
                        pCur = &obj.behaviours[obj.behaviourCount++];
                        std::memset(pCur, 0, sizeof(*pCur));
                        std::strncpy(pCur->className, name,
                                     sizeof(pCur->className) - 1);
                        state = 3;
                    }
                } else if (flags == 0x00000000) {
                    // flags=0: string segment (e.g. voice entity name) OR 16B
                    // blob (GUID — e.g. CSystemCommands sub-segments).
                    // content length is exact: x-8; ReadPadString would over-read
                    // the blob and shift the stream — consume the exact length.
                    RwUInt32 contentLen = x >= 8 ? x - 8 : 0;
                    RwChar buf[64];
                    if (contentLen > sizeof(buf)) {
                        // unknown long blob — seek to chunk end
                        RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
                        break;
                    }
                    if (contentLen > 0 &&
                        RwStreamRead(pFile->pStream, buf, contentLen) !=
                            contentLen) {
                        break;
                    }
                    // detect: first byte printable -> string; otherwise blob
                    if (pCur && contentLen > 0 && buf[0] >= 0x20 &&
                        buf[0] < 0x7F) {
                        RwUInt32 i = 0;
                        for (; i + 1 < sizeof(pCur->name) && i < contentLen &&
                                             buf[i] != '\0';
                             ++i) {
                            pCur->name[i] = buf[i];
                        }
                        pCur->name[i] = '\0';
                    }
                } else {
                    break; // unknown segment — seek to chunk end
                }
            } else if (x == 12 || x == 20) {
                // property: [size][id][size-8 value] — even if capacity is missing,
                // consume the bytes (preserve alignment)
                RwUInt32 propId = 0;
                if (RwStreamRead(pFile->pStream, &propId, 4) != 4) {
                    break;
                }
                RwUInt8 valueBuf[16];
                RwUInt32 valueLen = x - 8;
                std::memset(valueBuf, 0, sizeof(valueBuf));
                if (RwStreamRead(pFile->pStream, valueBuf, valueLen) !=
                    valueLen) {
                    break;
                }
                if (pCur && pCur->propCount < rwstudioMAX_PROPS) {
                    RwStudioProperty &prop = pCur->props[pCur->propCount];
                    prop.id = propId;
                    prop.size = x;
                    std::memcpy(prop.value, valueBuf, valueLen);
                    ++pCur->propCount;
                }
            } else {
                break; // unexpected field — preserve alignment
            }
        }
        pFile->hasObject = TRUE;
        RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
        return TRUE;
    }

    // unknown types: seek to chunk end
    RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
    return TRUE;
}

RwUInt32 RwStudioAssetReadEmbedded(RwStudioFile *pFile,
                                   const RwStudioAsset *pAsset, void *pBuf,
                                   RwUInt32 maxLen) {
    if (!pFile || !pAsset || !pBuf || pFile->pendingPayload == 0) {
        return 0;
    }
    // absolute position + seek to chunk end: alignment preserved in every case
    RwStreamSeekAbs(pFile->pStream, pFile->embedPayloadPos);
    RwUInt32 toRead = pFile->pendingPayload;
    if (toRead > maxLen) {
        toRead = maxLen;
    }
    RwUInt32 got = RwStreamRead(pFile->pStream, pBuf, toRead);
    RwStreamSeekAbs(pFile->pStream, pFile->chunkEndPos);
    pFile->pendingPayload = 0;
    return got;
}

RwUInt32 RwStudioAssetReadResource(RwStudioFile *pFile,
                                   const RwStudioAsset *pAsset, void *pBuf,
                                   RwUInt32 maxLen) {
    if (!pFile || !pAsset || !pBuf || pFile->pendingPayload == 0 ||
        pAsset->resourceLength == 0 || pAsset->embeddedType != 0) {
        return 0;
    }
    // embed payload mechanism shares the same fields (pos + pending)
    return RwStudioAssetReadEmbedded(pFile, pAsset, pBuf, maxLen);
}

RwUInt32 RwStudioFileGetClassCount(const RwStudioFile *pFile) {
    return pFile ? pFile->classCount : 0;
}

const RwChar *RwStudioFileGetClassName(const RwStudioFile *pFile,
                                       RwUInt32 index) {
    if (!pFile || index >= pFile->classCount) {
        return "";
    }
    return pFile->classNames[index];
}

const RwStudioObject *RwStudioFileGetObject(const RwStudioFile *pFile) {
    if (!pFile || !pFile->hasObject) {
        return 0;
    }
    return &pFile->object;
}
