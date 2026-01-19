#pragma once

#include "Animation/AnimationSequence.hpp"
#include <memory>

namespace SPF_CabinWalk::AnimationSequences
{
    /**
     * @brief Creates the animation sequence for transitioning from Sit1 to Lie on the sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaSit1ToLieSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

    /**
     * @brief Creates the animation sequence for transitioning from Lie to Sit2 on the sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaLieToSit2Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

    /**
     * @brief Creates the animation sequence for transitioning from Lie to Sit1 on the sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaLieToSofa1Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

    /**
     * @brief Creates the animation sequence for transitioning from Sit2 back to Sit1 on the sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaSit2ToSit1Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

    /**
     * @brief Creates the animation sequence for transitioning from Sit1 to Sit2 on the sofa.
     */
    std::unique_ptr<Animation::AnimationSequence> CreateSofaSit1ToSit2Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    );

} // namespace SPF_CabinWalk::AnimationSequences
