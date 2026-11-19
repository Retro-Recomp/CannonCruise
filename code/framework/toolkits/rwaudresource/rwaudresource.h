// #pragma pack(push, 1) - Reverted to default alignment for MSVC/RenderWare 3.7 compatibility
#ifndef RWS_RWAUDRESOURCE_H
#define RWS_RWAUDRESOURCE_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (rwaudresource.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\rwaudresource\rwaudresource.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00409600 - FUN_004096a0 (RWS::RwAudResource)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @class RwAudResource
     * @brief Class that loads audio databases and dictionaries (`Wave Dictionary`).
     *
     * Called by `CResourceManager` to manage `.rwa` or `.wav` packages;
     * it binds to the underlying Miles Sound System structures.
     *
     * **Binary references:**
     * - LoadWaveDict: `FUN_00409600`
     * - UnloadWaveDict: `FUN_00409660`
     */
    class RwAudResource
    {
    public:
        /**
         * @brief Loads the audio dictionary in the given file into memory.
         * @param pFilename Audio dictionary file to load.
         * @return void* — Pointer to the loaded Wave Dictionary structure.
         */
        static void* LoadWaveDict(const char* pFilename);

        /**
         * @brief Releases the loaded audio dictionary from memory.
         * @param pWaveDict Wave Dictionary to destroy.
         */
        static void UnloadWaveDict(void* pWaveDict);
    };
}

#endif // RWS_RWAUDRESOURCE_H

