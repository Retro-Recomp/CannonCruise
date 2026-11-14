#ifndef RWS_ANIMHELPER_H
#define RWS_ANIMHELPER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (AnimHelper.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\Animation\AnimHelper.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00409000 - FUN_004091a0 (RWS::AnimHelper)
// ============================================================================

#include <rwcore.h>
#include <rpworld.h>
#include <rphanim.h>
#include <rtanim.h>

namespace RWS
{
    /**
     * @class AnimHelper
     * @brief Helper utility class for RenderWare HAnim animation hierarchies.
     *
     * Used by `CAnimController` so that 3D character animations
     * (walking, shooting etc.) are played, updated and transitioned (blended)
     * in a managed way.
     *
     * **Binary references:**
     * - UpdateHierarchy: `FUN_00409000`
     * - SetCurrentAnim: `FUN_00409060`
     * - BlendHierarchy: `FUN_004090d0`
     * - UpdateHierarchyMatrices: `FUN_00409140`
     */
    class AnimHelper
    {
    public:
        /**
         * @brief Advances the animation hierarchy's time.
         * @param pHierarchy HAnim hierarchy to update.
         * @param deltaTime  Elapsed time (seconds).
         */
        static void UpdateHierarchy(RpHAnimHierarchy* pHierarchy, RwReal deltaTime);

        /**
         * @brief Sets the current animation for the hierarchy.
         * @param pHierarchy Target HAnim hierarchy.
         * @param pAnim      RenderWare animation object to play.
         */
        static void SetCurrentAnim(RpHAnimHierarchy* pHierarchy, RtAnimAnimation* pAnim);

        /**
         * @brief Blends between two animation hierarchies.
         * @param pDestHierarchy Destination (output) hierarchy.
         * @param pSrcHierarchy  Source (to blend from) hierarchy.
         * @param blendAlpha     Blend weight (in the 0.0 - 1.0 range).
         */
        static void BlendHierarchy(RpHAnimHierarchy* pDestHierarchy, RpHAnimHierarchy* pSrcHierarchy, RwReal blendAlpha);

        /**
         * @brief Updates the bone matrices in the hierarchy.
         * @param pHierarchy Target HAnim hierarchy.
         *
         * This operation is usually called after `UpdateHierarchy` or `BlendHierarchy`
         * to transform the matrices into world coordinates.
         */
        static void UpdateHierarchyMatrices(RpHAnimHierarchy* pHierarchy);
    };
}

#endif // RWS_ANIMHELPER_H

