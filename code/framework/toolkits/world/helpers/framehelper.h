#ifndef RWS_FRAMEHELPER_H
#define RWS_FRAMEHELPER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (framehelper.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\world\helpers\framehelper.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00408900 - FUN_00408b40 (RWS::FrameHelper)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @class FrameHelper
     * @brief Helper class for working with RwFrame hierarchies.
     *
     * In CannonCruise, every 3D object's transform is managed through a
     * `RwFrame` hierarchy. `FrameHelper` encapsulates common operations like
     * finding a frame by name, setting position and rotation.
     *
     * **Binary references:**
     * - FindFrameByName: `FUN_00408900`
     * - SetPosition: `FUN_004089a0`
     * - Rotate: `FUN_00408a10`
     */
    class FrameHelper
    {
    public:
        /**
         * @brief Searches the frame hierarchy for a frame with the given name.
         * @param pRootFrame Root frame.
         * @param pName      Name of the frame to find.
         * @return RwFrame* — Bulunan frame; bulunamazsa nullptr.
         */
        static RwFrame* FindFrameByName(RwFrame* pRootFrame, const char* pName);

        /**
         * @brief Finds the target frame within the hierarchy.
         * @param pRootFrame Root frame.
         * @param pTarget    Target frame.
         * @return RwFrame* — Bulunan frame; bulunamazsa nullptr.
         */
        static RwFrame* FindChildFrame(RwFrame* pRootFrame, RwFrame* pTarget);

        /**
         * @brief Sets the frame's world position.
         * @param pFrame Target frame.
         * @param pPos   New world position.
         */
        static void SetPosition(RwFrame* pFrame, const RwV3d* pPos);

        /**
         * @brief Gets the frame's world position.
         * @param pFrame Source frame.
         * @param pPos   [out] Output where the position value will be written.
         */
        static void GetPosition(const RwFrame* pFrame, RwV3d* pPos);

        /**
         * @brief Rotates the frame around the given axis.
         * @param pFrame     Target frame.
         * @param pAxis      Rotation axis (normalized).
         * @param angle      Rotation angle (radians).
         * @param combineOp  Matrix combine method (default: POST_CONCAT).
         */
        static void Rotate(RwFrame* pFrame, const RwV3d* pAxis, RwReal angle,
                           RwOpCombineType combineOp = rwCOMBINEPOSTCONCAT);

        /**
         * @brief Translates the frame by the given vector.
         * @param pFrame       Target frame.
         * @param pTranslation Translation vector.
         * @param combineOp    Matrix combine method (default: POST_CONCAT).
         */
        static void Translate(RwFrame* pFrame, const RwV3d* pTranslation,
                              RwOpCombineType combineOp = rwCOMBINEPOSTCONCAT);
    };
}

#endif // RWS_FRAMEHELPER_H
