// ============================================================================
// CannonCruise - level resource registry (RwsResources.h)
//
// Streams/*.RWS 0x0716 asset records carry a GUID and, for models, an
// embedded RenderWare chunk (rwID_CLUMP). Objects reference those assets by
// GUID: CSystemCommands command 0 in their attribute packet (one 16-byte GUID
// per resource: model, collision hull, damaged model, animations …).
//
// WorldSpawn_FromFile registers the level's embedded resources here before it
// creates objects, so CSystemCommands::ExtractClump can resolve them.
// ============================================================================

#ifndef CANNONCRUISE_RWSRESOURCES_H
#define CANNONCRUISE_RWSRESOURCES_H

#include "rwcore.h"

/// RenderWare chunk id of an embedded clump (.dff).
#define rwsRESOURCE_CLUMP 0x0010

/// One embedded resource of the current level.
struct RwsResource {
    RwUInt8 guid[16];
    RwUInt32 embeddedType;  ///< RW chunk id of the payload (rwsRESOURCE_CLUMP …)
    RwChar name[64];        ///< asset name, e.g. "Crate_Weapons.dff"
    const RwUInt8 *pData;   ///< chunk payload (after the 12-byte RW header)
    RwUInt32 size;          ///< payload size
};

/// Drops every registered resource (call before loading another level).
void RwsResources_Clear(void);

/// Registers (copies) a resource; a GUID registered twice keeps the first.
RwBool RwsResources_Add(const RwUInt8 guid[16], RwUInt32 embeddedType,
                        const RwChar *pName, const void *pData, RwUInt32 size);

/// @return The resource with this GUID, or NULL.
const RwsResource *RwsResources_Find(const RwUInt8 guid[16]);

/// @return Number of registered resources.
RwUInt32 RwsResources_Count(void);

#endif // CANNONCRUISE_RWSRESOURCES_H
