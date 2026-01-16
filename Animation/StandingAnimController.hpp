#pragma once

#include "SPF_TelemetryData.h"
#include "Animation/AnimationSequence.hpp"
#include "Animation/AnimationController.hpp" // For CameraPosition enum

// Forward declare PluginContext
struct PluginContext;

namespace SPF_CabinWalk
{
    namespace StandingAnimController
    {
        /**
         * @brief Defines the vertical stance of the camera while in the 'Standing' position.
         */
        enum class Stance
        {
            Standing,
            Crouching,
            Tiptoes,
            InTransition,  // Represents the state while an animation is playing
            WalkingToFinalDestination // Automatically walking to a specific Z-coordinate before a final destination
        };

        /**
         * @brief Initializes the Standing Animations Controller.
         * @param ctx A pointer to the main plugin context.
         */
        void Initialize(PluginContext* ctx);

        /**
         * @brief Initiates an automatic walk towards a specific Z-coordinate.
         * @param target_z The Z-coordinate to walk to.
         * @param final_destination The CameraPosition to transition to after the walk is complete.
         */
        void StartWalkingToZ(float target_z, AnimationController::CameraPosition final_destination);

    /**
     * @brief Updates the standing animation state based on the current camera view.
     * @param current_state The current state (position and rotation) of the camera.
     */
    void Update(const Animation::CurrentCameraState& current_state);

    /**
     * @brief Resets the standing animation state, typically called when entering the standing position.
     */
    void OnEnterStandingState();

    /**
     * @brief Triggers a single walk step towards or away from the home position.
     * @param current_state The current state of the camera.
     * @param is_walking_forward True if walking forward (negative Z), false if walking backward (positive Z).
     */
    void TriggerWalkStepTowards(const Animation::CurrentCameraState& current_state, bool is_walking_forward);

    /**
     * @brief Checks if the player can immediately sit down, or initiates a walk back to a target Z.
     * @param target The CameraPosition to sit down into (Driver or Passenger).
     * @param target_z The target Z-coordinate to be at before sitting down.
     * @return True if the player is close enough to sit down immediately, false otherwise (a walk back was initiated).
     */
    bool CanSitDown(AnimationController::CameraPosition target, float target_z);

    /**
     * @brief Gets the current vertical stance of the camera.
     * @return The current Stance.
     */
    Stance GetCurrentStance();

    /**
     * @brief Checks if the controller is currently playing a stance-related animation.
     * @return True if an animation is active, false otherwise.
     */
    bool IsAnimating();

    /**
     * @brief Triggers an animation to stand up from a crouching position.
     */
    void TriggerStandUp();

    /**
     * @brief Triggers an animation to stand down from a tiptoeing position.
     */
    void TriggerStandDown();

}
} // namespace SPF_CabinWalk
