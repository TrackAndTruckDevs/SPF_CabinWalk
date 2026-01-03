#pragma once
#include <SPF_TelemetryData.h>
#include <map>
#include <functional>
#include <memory> // For std::unique_ptr

// Include new animation system components
#include "Animation/AnimationSequence.hpp"
#include "Animation/Keyframe.hpp"
#include "Animation/Track.hpp"
#include "Animation/Positions/CameraPositions.hpp" // For CameraPositions::Transform, not for CameraPosition enum

// Forward declare PluginContext
struct PluginContext;

namespace SPF_CabinWalk
{
    namespace AnimationController
    {
        /**
         * @brief Defines the logical camera positions within the cabin.
         */
        enum class CameraPosition
        {
            Driver,
            Passenger,
            Standing, // To be implemented
            Bed       // To be implemented
        };

        /**
         * @brief Initializes the Animation Controller.
         * @param ctx A pointer to the main plugin context.
         */
        void Initialize(PluginContext *ctx);

        /**
         * @brief Updates the current animation state. Should be called every frame.
         */
        void Update();

        /**
         * @brief Starts a transition to the specified target camera position.
         * @param target The desired camera position.
         */
        void MoveTo(CameraPosition target);

        /**
         * @brief Checks if an animation is currently in progress.
         * @return true if the camera is currently animating, false otherwise.
         */
        bool IsAnimating();

        /**
         * @brief Gets the current logical position of the camera.
         * @return The current CameraPosition.
         */
        CameraPosition GetCurrentPosition();

        /**
         * @brief Registers an animation sequence factory for a given transition.
         * @param from The starting camera position.
         * @param to The target camera position.
         * @param factory A function that creates a unique_ptr to an AnimationSequence,
         *                optionally taking the initial camera state.
         */
        void RegisterSequence(
            CameraPosition from,
            CameraPosition to,
            std::function<std::unique_ptr<Animation::AnimationSequence>(const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state)> factory
        );

    } // namespace AnimationController
} // namespace SPF_CabinWalk
