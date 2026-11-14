// ============================================================================
// CannonCruise - RenderWare Studio Framework (SkinSafe.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\Animation\SkinSafe.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// ============================================================================

#include "SkinSafe.h"
#include <cassert>
#include <cstring>

namespace RWS
{
    void SkinSafe::CopyBoneMatrices(RwMatrix* pDstMatrices, const RwMatrix* pSrcMatrices, RwUInt32 numBones)
    {
        assert(pDstMatrices != 0 && "Failed PRE-condition: pDstMatrices");
        assert(pSrcMatrices != 0 && "Failed PRE-condition: pSrcMatrices");

        memcpy(pDstMatrices, pSrcMatrices, numBones * sizeof(RwMatrix));
    }

    RwBool SkinSafe::HierarchyCanAcceptDefaultPose(const RpHAnimHierarchy* pHierarchy)
    {
        if (!pHierarchy) return FALSE;
        return (pHierarchy->numNodes > 0 && pHierarchy->pNodeInfo != 0);
    }

    void SkinSafe::SetDefaultPose(RpHAnimHierarchy* pHierarchy)
    {
        if (!pHierarchy) return;
        assert(HierarchyCanAcceptDefaultPose(pHierarchy) && "Failed PRE-condition: HierarchyCanAcceptDefaultPose( hierarchy )");

        // Reset the hierarchy matrices to the default (identity) pose
        RpHAnimHierarchyUpdateMatrices(pHierarchy);
    }
}
