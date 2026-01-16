#pragma once

#include "Animation/AnimationSequence.hpp"
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates an animation sequence for the transition from the Sofa to Standing.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaToStandingSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

} // namespace SPF_CabinWalk::AnimationSequences
