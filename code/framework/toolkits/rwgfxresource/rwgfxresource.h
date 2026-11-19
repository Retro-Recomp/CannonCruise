// #pragma pack(push, 1) - Reverted to default alignment for MSVC/RenderWare 3.7 compatibility
#ifndef RWS_RWGFXRESOURCE_H
#define RWS_RWGFXRESOURCE_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (rwgfxresource.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\rwgfxresource\rwgfxresource.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00409800 - FUN_004098f0 (RWS::RwGfxResource)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>

namespace RWS
{
    /**
     * @class RwGfxResource
     * @brief Handles loading RenderWare graphics resources (.dff, .bsp, .txd etc.).
     *
     * Works integrated with `CResourceManager` and `CStreamHandler`.
     * Selects the appropriate RenderWare chunk handler based on the resource's type (`pType`) string.
     *
     * **Binary references:**
     * - Load: `FUN_00409800`
     * - Unload: `FUN_00409860`
     * - RegisterHandlers: `FUN_004098b0`
     */
    class RwGfxResource
    {
    public:
        /**
         * @brief Reads a graphics resource from a file path and loads it into memory.
         * @param pFilename File path to read.
         * @param pType     Resource type (`CTextureResource`, `CWorldGeometryResource` etc.)
         * @return void* — Pointer to the created resource (RpClump, RwTexture etc.)
         */
        static void* Load(const char* pFilename, const char* pType);

        /**
         * @brief Deletes the graphics resource from memory.
         * @param pResource Resource to destroy.
         * @param pType     Resource type.
         */
        static void Unload(void* pResource, const char* pType);

        /**
         * @brief Registers the graphics read chunk handlers with `CStreamHandler`.
         *
         * Called at engine startup so that files such as `.dff`, `.bsp`
         * can be parsed.
         */
        static void RegisterHandlers();
    };
}

#endif // RWS_RWGFXRESOURCE_H

