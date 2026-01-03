#pragma once

#include "Animation/AnimationSequence.hpp"
#include "Animation/Positions/CameraPositions.hpp"
#include "Animation/Easing/Easing.hpp" // For Easing functions
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates a complete animation sequence for the transition from the Driver's seat to the Passenger's seat.
     * @param initial_state The camera's state (position and rotation) at the moment the animation starts.
     * @return A unique_ptr to the configured AnimationSequence.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateDriverToPassengerSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

} // namespace SPF_CabinWalk::AnimationSequences