#ifndef RWS_CMATERIALEVENT_H
#define RWS_CMATERIALEVENT_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (cmaterialevent.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\world\cmaterialevent.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_0040a000 - FUN_0040a200 (RWS::CMaterialEvent)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>
#include <string>

namespace RWS
{
    /**
     * @struct MaterialEventData
     * @brief Structure for collision events (plugin data) attached to materials.
     *
     * Used to assign properties to materials such as the ground (water, sand, stone) or walls;
     * used when, e.g., a cannonball hits, to play material-specific
     * particle effects or sounds — `m_EventId` is checked for this.
     */
    struct MaterialEventData
    {
        RwUInt32 m_EventId;       ///< Event id (e.g. EVENT_HIT_WATER)
        RwUInt32 m_MaterialIndex; ///< Index of the material within the model
        RwChar m_Tag[32];         ///< Custom tag/name assigned to the material
    };

    /**
     * @class CMaterialEvent
     * @brief Plugin manager that adds event data to RenderWare RpMaterial objects.
     *
     * Extra storage is allocated in the material structure via `RpMaterialRegisterPlugin()`.
     * This way, material info coming from the physics engine is
     * connected to the RWS event system (CEventHandler).
     *
     * **Binary references:**
     * - PluginAttach: `FUN_0040a000`
     * - GetMaterialData: `FUN_0040a080`
     * - SetMaterialEventId: `FUN_0040a0f0`
     */
    class CMaterialEvent
    {
    public:
        /**
         * @brief Registers the material event plugin with the RenderWare engine.
         * @return TRUE if the plugin was registered successfully.
         *
         * Must be called after `RwEngineInit()` but before any objects are loaded
         * (usually within `AttachPlugins`).
         */
        static RwBool PluginAttach();

        /**
         * @brief Returns a pointer to the event data (plugin field) of the given material.
         * @param pMaterial Target RenderWare material.
         * @return MaterialEventData* — The material's event data.
         */
        static MaterialEventData* GetMaterialData(RpMaterial* pMaterial);

        /**
         * @brief Assigns a collision/interaction event id to the material.
         * @param pMaterial Target material.
         * @param eventId   `CEventId` to assign (e.g. iMsgHit).
         */
        static void SetMaterialEventId(RpMaterial* pMaterial, RwUInt32 eventId);

    private:
        static RwInt32 sm_PluginOffset; ///< Byte offset of the plugin data within RpMaterial

        // RenderWare plugin callbacks
        static void* MaterialConstructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
        static void* MaterialDestructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject);
        static void* MaterialCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject);
    };
}

#endif // RWS_CMATERIALEVENT_H

