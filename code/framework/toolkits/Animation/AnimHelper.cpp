// ============================================================================
// CannonCruise - RenderWare Studio Framework (AnimHelper.cpp)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\Animation\AnimHelper.cpp
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// ============================================================================

#include "AnimHelper.h"
#include <cassert>

namespace RWS
{
    void AnimHelper::UpdateHierarchy(RpHAnimHierarchy* pHierarchy, RwReal deltaTime)
    {
        if (!pHierarchy) return;

        RpHAnimHierarchyAddAnimTime(pHierarchy, deltaTime);
        RpHAnimHierarchyUpdateMatrices(pHierarchy);
    }

    void AnimHelper::SetCurrentAnim(RpHAnimHierarchy* pHierarchy, RtAnimAnimation* pAnim)
    {
        assert(pHierarchy != 0);

        if (pAnim)
        {
            RpHAnimHierarchySetCurrentAnim(pHierarchy, pAnim);
        }
    }

    void AnimHelper::BlendHierarchy(RpHAnimHierarchy* pDestHierarchy, RpHAnimHierarchy* pSrcHierarchy, RwReal blendAlpha)
    {
        if (!pDestHierarchy || !pSrcHierarchy) return;

        RpHAnimHierarchyBlendHierarchy(pDestHierarchy, pSrcHierarchy, blendAlpha);
    }

    void AnimHelper::UpdateHierarchyMatrices(RpHAnimHierarchy* pHierarchy)
    {
        if (pHierarchy)
        {
            RpHAnimHierarchyUpdateMatrices(pHierarchy);
        }
    }
}
