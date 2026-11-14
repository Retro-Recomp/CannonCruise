#ifndef RWS_CLINEARMEMORYMANAGER_H
#define RWS_CLINEARMEMORYMANAGER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (clinearmemorymanager.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\Allocation Policies\..\clinearmemorymanager.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_004053f0 - FUN_00405600 (RWS::CLinearMemoryManager)
// ============================================================================

#include <rwcore.h>
#include <vector>
#include "../cclassfactory.h"

namespace RWS
{
    /**
     * @class CLinearMemoryManager
     * @brief Linear (block) based custom memory manager.
     *
     * Used for objects that are frequently allocated during gameplay — temporary or objects with the same lifetime —
     * so that allocations are carved out of a single block and the entire pool is
     * released in one go (deallocation). This prevents
     * memory fragmentation and improves speed.
     *
     * **Binary references:**
     * - Alloc: `FUN_004054a0`
     * - Free: `FUN_00405520`
     * - CreatePool: `FUN_00405580`
     * - DestroyPool: `FUN_004055e0`
     */
    class CLinearMemoryManager
    {
    public:
        /**
         * @struct MemoryPool
         * @brief Structure describing an allocated memory pool.
         */
        struct MemoryPool
        {
            void* m_pBase;         ///< Base memory block (malloc output)
            void* m_pAlignedBase;  ///< 64-byte aligned start (alignment)
            void* m_pCurrent;      ///< Next free location (current allocation boundary)
            void* m_pEnd;          ///< End of pool (pAlignedBase + poolSize)
            RwUInt32 m_AllocCount; ///< Number of active allocations in the pool
        };

        /// @return Pointer to the singleton CLinearMemoryManager instance
        static CLinearMemoryManager* Instance();
        /// @brief Destroys the manager and all pools
        static void Delete();

        /**
         * @brief Allocates a block from the active memory pool.
         * @param size Number of bytes to allocate.
         * @return void* — Pointer to the allocated block (nullptr if the pool is full).
         */
        static void* Alloc(RwUInt32 size);

        /**
         * @brief Releases memory allocated from the pool (decrements the counter).
         * @param pMem Memory block to release.
         * @return TRUE if it was released successfully.
         *
         * Note: Actual memory release happens when the pool is destroyed.
         */
        static RwBool Free(void* pMem);

        /**
         * @brief Creates a new memory pool and makes it the active pool.
         * @param poolSize Total size of the pool (bytes).
         */
        static void CreatePool(RwUInt32 poolSize);

        /**
         * @brief Destroys the given memory pool entirely.
         * @param pPool Pool to destroy.
         */
        static void DestroyPool(MemoryPool* pPool);

    private:
        CLinearMemoryManager();
        ~CLinearMemoryManager();

        static CLinearMemoryManager* sm_pInstance; ///< Singleton instance pointer
        static MemoryPool* sm_pActivePool;         ///< Pool currently being allocated from

        std::vector<MemoryPool*> m_StoragePools;   ///< List of all pools
    };
}

#endif // RWS_CLINEARMEMORYMANAGER_H

