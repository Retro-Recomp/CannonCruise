#ifndef RWS_CRESOURCEMANAGER_H
#define RWS_CRESOURCEMANAGER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (cresourcemanager.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\resourcemanager\cresourcemanager.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00405000 - FUN_004052e0 (RWS::CResourceManager)
// ============================================================================

#include <rwcore.h>
#include <map>
#include <string>

namespace RWS
{
    /**
     * @struct CResource
     * @brief Data structure describing a registered game resource (asset).
     *
     * Created by `CResourceManager::RegisterResource()` and
     * added to the resource table. Contains the type, name, path, raw data pointer,
     * reference count and flags.
     *
     * **Resource types (m_Type):**
     * - `"CTextureResource"` — Texture resource
     * - `"CWorldGeometryResource"` — BSP world geometry
     * - `"CPathFindingResource"` — Pathfinding map
     * - `"CAudioResource"` — Audio file
     */
    struct CResource
    {
        std::string m_Type;     ///< Resource type string (e.g. "CTextureResource")
        std::string m_Name;     ///< Human-readable resource name
        std::string m_Path;     ///< File system path
        void*       m_pData;    ///< Loaded resource data (requires type-specific cast)
        RwUInt32    m_RefCount; ///< Reference count (0 → can be released)
        RwUInt32    m_Flags;    ///< Resource flags (loaded, persistent etc.)

        /// @brief Default constructor — zeroes all fields
        CResource() : m_pData(0), m_RefCount(0), m_Flags(0) {}
    };

    /**
     * @class CResourceManager
     * @brief Manages game resources (assets) centrally.
     *
     * Textures, world geometries and audio resources in CannonCruise
     * are loaded and shared through this manager. A reference count
     * mechanism allows the same resource to be used by multiple objects;
     * when the count drops to zero, the resource is removed from memory.
     *
     * **Usage flow:**
     * 1. `Initialize()` — At application startup
     * 2. `RegisterResource()` — When a resource is loaded
     * 3. `FindResource()` — When a resource is about to be used
     * 4. `UnRegisterResource()` / `UnRegisterAll()` — When the scene is closed
     *
     * **Binary references:**
     * - Instance: `FUN_00405000`
     * - FindResource: `FUN_00405080`
     * - RegisterResource: `FUN_004050f0`
     * - UnRegisterResource: `FUN_00405180`
     */
    class CResourceManager
    {
    public:
        /// @return Pointer to the singleton CResourceManager instance
        static CResourceManager* Instance();
        /// @brief Initializes the resource manager
        static void Initialize();
        /// @brief Shuts down the resource manager, releasing all resources
        static void Shutdown();

        /**
         * @brief Finds the resource registered with the given id and type.
         * @param pResId    Resource id string.
         * @param pStrType  Resource type string (optional filter).
         * @return void* — Pointer to the resource data; nullptr if not found.
         */
        static void* FindResource(const char* pResId, const char* pStrType = 0);

        /**
         * @brief Compatibility wrapper for `FindResource()`.
         * @param pResId   Resource id.
         * @param pStrType Resource type string.
         * @param pSize    Optional size output pointer (unused).
         * @return void* — Pointer to the resource data.
         */
        static void* Find(const char* pResId, const char* pStrType = 0, RwUInt32* pSize = 0)
        {
            return FindResource(pResId, pStrType);
        }

        /**
         * @brief Registers a new resource with the manager.
         * @param pResId   Unique resource id.
         * @param pStrType Resource type string.
         * @param pStrName Human-readable resource name.
         * @param pData    Pointer to the resource data.
         * @return TRUE if registration succeeded.
         */
        static RwBool RegisterResource(const char* pResId, const char* pStrType,
                                       const char* pStrName, void* pData);

        /**
         * @brief Unregisters the given resource.
         * @param pResId Resource id whose registration should be removed.
         */
        static void UnRegisterResource(const char* pResId);

        /// @brief Removes all registered resources
        static void UnRegisterAll();

    private:
        CResourceManager();
        ~CResourceManager();

        static CResourceManager* sm_pInstance; ///< Singleton instance pointer

        std::map<std::string, CResource> m_Resources; ///< ResId → CResource mapping table
    };
}

#endif // RWS_CRESOURCEMANAGER_H
