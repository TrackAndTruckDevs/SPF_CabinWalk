#pragma once

#include "Animation/AnimationSequence.hpp"
#include "Animation/Positions/CameraPositions.hpp"
#include "Animation/Easing/Easing.hpp" // For Easing functions
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates a complete animation sequence for the transition from the Passenger's seat to the Driver's seat.
     * @param start_state The camera's state (position and rotation) at the moment the animation starts (should be passenger seat).
     * @param target_state The camera's target state (position and rotation) at the end of the animation (should be cached driver seat).
     * @return A unique_ptr to the configured AnimationSequence.
     */
    std::unique_ptr<Animation::AnimationSequence> CreatePassengerToDriverSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

} // namespace SPF_CabinWalk::AnimationSequences