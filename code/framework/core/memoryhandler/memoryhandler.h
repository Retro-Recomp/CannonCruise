#ifndef RWS_MEMORYHANDLER_H
#define RWS_MEMORYHANDLER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (memoryhandler.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\memoryhandler\memoryhandler.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00406000 - FUN_00406280 (RWS::CMemoryHandler)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @class CMemoryHandler
     * @brief Class that manages RenderWare engine's memory allocation/free operations.
     *
     * RenderWare uses its own memory functions (`rwFreeList` based) through
     * externally injectable function pointers.
     * `CMemoryHandler` routes these pointers to the system heap or to a custom pool,
     * keeping RenderWare's memory allocation under control.
     *
     * **Initialization:**
     * ```cpp
     * CMemoryHandler::Initialize();  // Must be called BEFORE RwEngineInit
     * RwEngineInit(nullptr, 0, CMemoryHandler::GetMemoryFunctions());
     * ```
     *
     * **Binary references:**
     * - Initialize: `FUN_00406000`
     * - Shutdown: `FUN_00406060`
     * - RwAlloc: `FUN_004060c0`
     * - RwFree: `FUN_00406100`
     * - RwRealloc: `FUN_00406140`
     * - RwCalloc: `FUN_00406190`
     * - GetMemoryFunctions: `FUN_004061f0`
     */
    class CMemoryHandler
    {
    public:
        /**
         * @brief Allocates memory from the RenderWare free list pool.
         * @param fl   RenderWare free list (system malloc is used if nullptr).
         * @param size Number of bytes to allocate.
         * @param hint Alignment or allocation hint flags.
         * @return void* — Pointer to the allocated memory block.
         */
        static void* RwAlloc(RwFreeList* fl, RwUInt32 size, RwUInt32 hint);

        /**
         * @brief Releases memory allocated with `RwAlloc()`.
         * @param fl  RenderWare free list.
         * @param mem Memory block to release.
         * @return void* — Always nullptr.
         */
        static void* RwFree(RwFreeList* fl, void* mem);

        /**
         * @brief Resizes the existing memory block.
         * @param fl   RenderWare free list.
         * @param mem  Block to resize.
         * @param size New size (bytes).
         * @param hint Allocation hint flags.
         * @return void* — Pointer to the resized block.
         */
        static void* RwRealloc(RwFreeList* fl, void* mem, RwUInt32 size, RwUInt32 hint);

        /**
         * @brief Allocates a zeroed memory block.
         * @param fl      RenderWare free list.
         * @param numObj  Number of objects.
         * @param sizeObj Size of each object (bytes).
         * @param hint    Allocation hint flags.
         * @return void* — Pointer to the zeroed memory block.
         */
        static void* RwCalloc(RwFreeList* fl, RwUInt32 numObj, RwUInt32 sizeObj, RwUInt32 hint);

        /**
         * @brief Initializes the memory handler and registers the RenderWare memory functions.
         * @return TRUE if initialization succeeded.
         */
        static RwBool Initialize();

        /**
         * @brief Shuts down the memory handler.
         */
        static void Shutdown();

        /**
         * @brief Returns the memory function table for the RenderWare engine.
         * @return RwMemoryFunctions* — Pointer to the structure passed to `RwEngineInit()`.
         */
        static RwMemoryFunctions* GetMemoryFunctions();
    };
}

#endif // RWS_MEMORYHANDLER_H
