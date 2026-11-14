#ifndef RWS_SKINHELPER_H
#define RWS_SKINHELPER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (skinhelper.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\world\helpers\skinhelper.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00408c00 - FUN_00408d40 (RWS::SkinHelper)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>
#include <rpskin.h>
#include <rphanim.h>

namespace RWS
{
    /**
     * @class SkinHelper
     * @brief Helper class for RpSkin and HAnim hierarchy.
     *
     * Skeletal-animated characters such as soldiers and birds contain atomics
     * using the `RpSkin` plugin. `SkinHelper` finds these atomics and
     * binds the `RpHAnimHierarchy` animation hierarchy.
     *
     * **Usage example:**
     * ```cpp
     * RpAtomic* pSkinAtomic = SkinHelper::FindSkinAtomic(pClump);
     * RpHAnimHierarchy* pHier = SkinHelper::GetHierarchy(pSkinAtomic);
     * SkinHelper::SetHierarchy(pSkinAtomic, pHier);
     * ```
     *
     * **Binary references:**
     * - FindSkinAtomic: `FUN_00408c00`
     * - GetHierarchy: `FUN_00408c60`
     * - SetHierarchy: `FUN_00408cc0`
     */
    class SkinHelper
    {
    public:
        /**
         * @brief Finds the atomic with the RpSkin plugin inside the clump.
         * @param pClump Clump to search.
         * @return RpAtomic* — The skin atomic; nullptr if not found.
         */
        static RpAtomic* FindSkinAtomic(RpClump* pClump);

        /**
         * @brief Returns the HAnim hierarchy attached to the atomic.
         * @param pAtomic Target skin atomic.
         * @return RpHAnimHierarchy* — The hierarchy; nullptr if absent.
         */
        static RpHAnimHierarchy* GetHierarchy(RpAtomic* pAtomic);

        /**
         * @brief Binds the HAnim hierarchy to the atomic.
         * @param pAtomic     Target skin atomic.
         * @param pHierarchy  Hierarchy to bind.
         * @return TRUE if the binding succeeded.
         */
        static RwBool SetHierarchy(RpAtomic* pAtomic, RpHAnimHierarchy* pHierarchy);
    };
}

#endif // RWS_SKINHELPER_H
