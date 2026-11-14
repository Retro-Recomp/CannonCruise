#ifndef RWS_AUDIO_H
#define RWS_AUDIO_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (Audio.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\Audio\Audio.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00407000 - FUN_004072a0 (RWS::Audio)
// ============================================================================

#include <rwcore.h>

namespace RWS
{
    /**
     * @namespace RWS::Audio
     * @brief Helper functions for the RenderWare Studio audio infrastructure.
     *
     * CannonCruise's audio system runs through `CAudioManager`;
     * it exposes the underlying sound engine (DirectSound / Miles Sound System)
     * this namespace offers plain functions to start it and control levels.
     *
     * **Sound hierarchy:**
     * - Master Volume → global output level
     * - Effects Volume → SFX channels (cannonball sound, waves etc.)
     * - Music Volume → background music
     *
     * **3D audio:** `SetListenerPosition()` must be called every frame with the camera's position.
     */
    namespace Audio
    {
        /**
         * @brief Initializes the audio subsystem.
         * @return TRUE if initialization succeeded.
         *
         * Must be called before `CAudioManager` is created.
         */
        RwBool Initialize();

        /**
         * @brief Shuts down the audio subsystem, releasing all resources.
         */
        void Shutdown();

        /**
         * @brief Genel ses seviyesini ayarlar.
         * @param volume Audio level in the [0.0, 1.0] range.
         */
        void SetMasterVolume(RwReal volume);

        /**
         * @brief Returns the master audio level.
         * @return RwReal — Current master volume value.
         */
        RwReal GetMasterVolume();

        /**
         * @brief Efekt ses seviyesini ayarlar.
         * @param volume Sound effect level in the [0.0, 1.0] range.
         */
        void SetEffectsVolume(RwReal volume);

        /**
         * @brief Returns the effects audio level.
         * @return RwReal — Current effects volume value.
         */
        RwReal GetEffectsVolume();

        /**
         * @brief Sets the music audio level.
         * @param volume Music level in the [0.0, 1.0] range.
         */
        void SetMusicVolume(RwReal volume);

        /**
         * @brief Returns the music audio level.
         * @return RwReal — Current music volume value.
         */
        RwReal GetMusicVolume();

        /**
         * @brief Updates the position of the 3D audio listener.
         * @param pPos     Listener (camera) world position.
         * @param pForward Direction the listener is facing (normalized).
         * @param pUp      Listener's up direction (normalized).
         *
         * Must be called every frame right after the camera update.
         */
        void SetListenerPosition(const RwV3d* pPos, const RwV3d* pForward, const RwV3d* pUp);
    }
}

#endif // RWS_AUDIO_H
