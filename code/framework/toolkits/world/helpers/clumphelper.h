#ifndef RWS_CLUMPHELPER_H
#define RWS_CLUMPHELPER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (clumphelper.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\world\helpers\clumphelper.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00408400 - FUN_00408680 (RWS::ClumpHelper)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>

namespace RWS
{
    /**
     * @class ClumpHelper
     * @brief Helper utility class for RpClump objects.
     *
     * In CannonCruise every 3D object (ship, soldier, cannonball etc.) is contained in a `RpClump`.
     * `ClumpHelper` offers the common operations on these objects (search,
     * cloning, destroying, bounding sphere) through a central API.
     *
     * **Binary references:**
     * - FindFirstAtomic: `FUN_00408400`
     * - CalculateBoundingSphere: `FUN_00408480`
     * - Clone: `FUN_004084f0`
     * - Destroy: `FUN_00408560`
     */
    class ClumpHelper
    {
    public:
        /**
         * @brief Finds the first `RpAtomic` object inside the clump.
         * @param pClump Clump to search.
         * @return RpAtomic* — The first atomic; nullptr if not found.
         */
        static RpAtomic* FindFirstAtomic(RpClump* pClump);

        /**
         * @brief Applies the given render flags to all atomics.
         * @param pClump Target clump.
         * @param flags  `rpATOMICRENDER` etc. flags to apply.
         */
        static void SetAtomicFlags(RpClump* pClump, RwUInt32 flags);

        /**
         * @brief Computes the sphere enclosing all geometry of the clump.
         * @param pClump  Target clump.
         * @param pSphere Output parameter where the computed sphere will be written.
         */
        static void CalculateBoundingSphere(RpClump* pClump, RwSphere* pSphere);

        /**
         * @brief Clones the clump (deep copy).
         * @param pClump Source clump to clone.
         * @return RpClump* — The newly cloned clump; nullptr on failure.
         */
        static RpClump* Clone(RpClump* pClump);

        /**
         * @brief Destroys the clump and all its attached resources.
         * @param pClump Clump to destroy.
         */
        static void Destroy(RpClump* pClump);

        /// @brief Compatibility wrapper for `Destroy()`
        static void DestroyClump(RpClump* pClump) { Destroy(pClump); }

        /**
         * @brief Creates a clump from raw resource data (stub).
         * @return RpClump* — always nullptr in CannonCruise.
         */
        static RpClump* CreateClumpFromResource(const void* pResourceData) { return 0; }

        /**
         * @brief Applies an attribute packet to the clump (stub).
         * @param pClump  Target clump.
         * @param packet  Attribute packet to apply.
         */
        static void HandleAttributes(RpClump* pClump, const class CAttributePacket& packet) {}
    };
}



#endif // RWS_CLUMPHELPER_H
