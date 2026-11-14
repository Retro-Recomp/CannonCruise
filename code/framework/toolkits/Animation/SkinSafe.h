#ifndef RWS_SKINSAFE_H
#define RWS_SKINSAFE_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (SkinSafe.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\Animation\SkinSafe.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00409200 - FUN_004092f0 (RWS::SkinSafe)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>
#include <rpskin.h>
#include <rphanim.h>

namespace RWS
{
    /**
     * @class SkinSafe
     * @brief Class providing safe matrix operations for skeletal animations.
     *
     * Used when the animation data is invalid or it is necessary to return
     * to the default T-pose (error tolerance).
     *
     * **Binary references:**
     * - CopyBoneMatrices: `FUN_00409200`
     * - SetDefaultPose: `FUN_00409260`
     * - HierarchyCanAcceptDefaultPose: `FUN_004092b0`
     */
    class SkinSafe
    {
    public:
        /**
         * @brief Safely copies the bone matrices.
         * @param pDstMatrices Destination matrix array.
         * @param pSrcMatrices Source matrix array.
         * @param numBones     Number of bones (matrices) to copy.
         */
        static void CopyBoneMatrices(RwMatrix* pDstMatrices, const RwMatrix* pSrcMatrices, RwUInt32 numBones);

        /**
         * @brief Brings the hierarchy to the default initial pose (usually the T-pose).
         * @param pHierarchy Target HAnim hierarchy.
         */
        static void SetDefaultPose(RpHAnimHierarchy* pHierarchy);

        /**
         * @brief Checks whether the hierarchy can accept the default pose.
         * @param pHierarchy HAnim hierarchy to check.
         * @return TRUE if the default pose can be applied.
         */
        static RwBool HierarchyCanAcceptDefaultPose(const RpHAnimHierarchy* pHierarchy);
    };
}

#endif // RWS_SKINSAFE_H

