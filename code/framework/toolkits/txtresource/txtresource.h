#ifndef RWS_TXTRESOURCE_H
#define RWS_TXTRESOURCE_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (txtresource.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\txtresource\txtresource.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00409400 - FUN_004094a0 (RWS::TxtResource)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @class TxtResource
     * @brief Class managing text-based resource files (`.txt`, `.ini`, `.xml` etc.).
     *
     * Called by `CResourceManager` to load text-based game settings or
     * dialogues.
     *
     * **Binary references:**
     * - Load: `FUN_00409400`
     * - Unload: `FUN_00409460`
     */
    class TxtResource
    {
    public:
        /**
         * @brief Reads a text resource from the file system and loads it into memory.
         * @param pFilename Path of the text file to load.
         * @return char* — Null-terminated string holding the file's content (nullptr on failure).
         */
        static char* Load(const char* pFilename);

        /**
         * @brief Releases the memory of the loaded text resource.
         * @param pTextData Text data to release.
         */
        static void Unload(char* pTextData);
    };
}

#endif // RWS_TXTRESOURCE_H

