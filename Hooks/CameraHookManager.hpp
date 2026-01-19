#pragma once

#include <SPF_Hooks_API.h>
#include "Animation/AnimationController.hpp" // For CameraPosition enum

namespace SPF_CabinWalk::CameraHookManager
{
    /**
     * @brief Initializes the camera hook manager and registers the necessary hook.
     * @param hooks_api A pointer to the SPF Hooks API.
     * @return true if initialization and hook registration were successful, false otherwise.
     */
    bool Initialize(const SPF_Hooks_API *hooks_api, const char *plugin_name);

    /**
     * @brief Sets the current logical position of the camera.
     * @details This function should be called when an animation to a new position completes.
     * It allows the hook manager to apply the correct logic (e.g., azimuth mirroring)
     * for the given position.
     * @param new_pos The camera's new logical position.
     */
    void SetCurrentCameraPosition(AnimationController::CameraPosition new_pos);

        /**
         * @brief Notifies the camera hook manager that settings have been updated.
         *        This will force a re-application of current camera position settings on the next update.
         */
        void NotifySettingsUpdated();

    } // namespace SPF_CabinWalk::CameraHookManager
