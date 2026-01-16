#pragma once

#include "Animation/AnimationSequence.hpp"
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates an animation sequence for the transition from Standing to the Sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateStandingToSofaSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

} // namespace SPF_CabinWalk::AnimationSequences
