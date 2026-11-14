#ifndef HAVOKMEMORYMANAGER_H
#define HAVOKMEMORYMANAGER_H

// ============================================================================
// CannonCruise - Havok Physics Memory Manager (HavokMemoryManager.h)
// Original path: D:\Projects\CannonCruisePC\code\HavokMemoryManager.h
// Reverse-engineered via Ghidra and CC.asm analysis.
// Binary ref: FUN_004b3000 - FUN_004b3200 (CHavokMemoryManager)
// ============================================================================

#include <rwcore.h>

/**
 * @class CHavokMemoryManager
 * @brief Local 16-byte aligned allocation helper; not a Havok SDK integration.
 *
 * Allocate/Free use the system heap and store the original allocation before
 * the aligned pointer. Only alignment=16 is supported. Initialize/Shutdown
 * are currently no-ops: no SDK callbacks are registered. SDK version, callback
 * signatures and ownership rules remain unverified (see SDK_INTERFACE_AUDIT.md).
 */
class CHavokMemoryManager {
public:
    /**
     * @brief Allocates an aligned memory block.
     * @param size      Number of bytes to allocate.
     * @param alignment Supported alignment: 16 bytes only (also the default).
     * @return Aligned pointer, or null on size overflow/unsupported alignment/allocation failure.
     * Invalid alignment and allocation failure also assert in debug builds.
     */
    static void *Allocate(RwUInt32 size, RwUInt32 alignment = 16);

    /**
     * @brief Frees memory allocated with `Allocate()`.
     * @param pMem Pointer to the memory block to release.
     */
    static void Free(void *pMem);

    /**
     * @brief No-op placeholder; does not register Havok callbacks.
     */
    static void Initialize();

    /**
     * @brief No-op placeholder; outstanding allocations must be freed by callers.
     */
    static void Shutdown();
};

#endif // HAVOKMEMORYMANAGER_H
