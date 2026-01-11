#pragma once

#include "Animation/AnimationSequence.hpp"
#include "Animation/AnimationController.hpp"
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates the animation sequence for crouching down.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateCrouchDownSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze);

    /**
     * @brief Creates the animation sequence for standing up from a crouch.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateStandUpSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze);

    /**
     * @brief Creates the animation sequence for getting on tiptoes.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateTiptoeSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze);

    /**
     * @brief Creates the animation sequence for getting off tiptoes.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateStandDownSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze);

    /**
     * @brief Creates the animation sequence for a single walk step.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateWalkStepSequence(const Animation::CurrentCameraState& initial_state, bool is_walking_forward);

    /**
     * @brief Creates a combined animation for the first walk step, including dynamic head alignment.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateDynamicFirstStepSequence(const Animation::CurrentCameraState& initial_state, bool is_walking_forward);

} // namespace SPF_CabinWalk::AnimationSequences
