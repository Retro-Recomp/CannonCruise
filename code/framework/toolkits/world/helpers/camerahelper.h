#ifndef RWS_CAMERAHELPER_H
#define RWS_CAMERAHELPER_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (camerahelper.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\world\helpers\camerahelper.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00408000 - FUN_00408200 (RWS::CameraHelper)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @class CameraHelper
     * @brief Helper utility class for RenderWare camera objects.
     *
     * The `CScene` and `CWbCamera` classes use this helper
     * to create, configure and manage RwCamera objects.
     *
     * **Binary references:**
     * - CreateCamera: `FUN_00408000`
     * - SetFieldOfView: `FUN_00408080`
     * - SetNearFarClip: `FUN_004080f0`
     */
    class CameraHelper
    {
    public:
        /**
         * @brief Creates a new RenderWare camera.
         * @param width   Render target width (pixels).
         * @param height  Render target height (pixels).
         * @param zBuffer Z-buffer etkin mi.
         * @return RwCamera* — The created camera; nullptr on failure.
         */
        static RwCamera* CreateCamera(RwInt32 width, RwInt32 height, RwBool zBuffer);

        /**
         * @brief Destroys the camera and the associated render targets.
         * @param pCamera Camera to destroy.
         */
        static void DestroyCamera(RwCamera* pCamera);

        /**
         * @brief Sets the camera's field of view (FoV).
         * @param pCamera Target camera.
         * @param fov     Vertical field of view angle (radians).
         * @param width   View width.
         * @param height  View height.
         */
        static void SetFieldOfView(RwCamera* pCamera, RwReal fov, RwInt32 width, RwInt32 height);

        /**
         * @brief Sets the camera's near/far clip planes.
         * @param pCamera  Target camera.
         * @param nearClip Near clip distance.
         * @param farClip  Far clip distance.
         */
        static void SetNearFarClip(RwCamera* pCamera, RwReal nearClip, RwReal farClip);

        /**
         * @brief Updates the camera's render target to the new screen size.
         * @param pCamera Target camera.
         * @param width   New width (pixels).
         * @param height  New height (pixels).
         */
        static void UpdateScreenSize(RwCamera* pCamera, RwInt32 width, RwInt32 height);
    };
}

#endif // RWS_CAMERAHELPER_H
