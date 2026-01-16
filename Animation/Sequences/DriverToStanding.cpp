#include "DriverToStanding.hpp"
#include "Animation/AnimationConfig.hpp" // For animation durations
#include "Animation/AnimationController.hpp"

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreateDriverToStandingSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(Animation::Config::DRIVER_TO_STANDING); // Using same duration for now 

        // --- Position X Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({0.35f, start_state.position.x, Easing::easeInCubic});
            track->AddKeyframe({0.5f, start_state.position.x + 0.35f, Easing::easeOutCubic});
            track->AddKeyframe({0.65f, target_state.position.x - 0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInCubic});
            track->AddKeyframe({0.30f, target_state.position.y, Easing::easeOutCubic});
            track->AddKeyframe({0.45f, target_state.position.y + 0.01f, Easing::easeOutCubic});
            track->AddKeyframe({0.5f, target_state.position.y, Easing::easeOutCubic});
            track->AddKeyframe({0.75f, target_state.position.y + 0.01f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::easeInOutCubic});
            track->AddKeyframe({0.15f, start_state.position.z - 0.15f, Easing::easeOutCubic});
            track->AddKeyframe({0.65f, start_state.position.z - 0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::easeOutCubic});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({0.1f, 0.0f, Easing::easeInOutCubic});
            track->AddKeyframe({0.23f, 0.1f, Easing::easeInOutCubic});
            track->AddKeyframe({0.73f, target_state.rotation.x -0.75f, Easing::easeInCubic});
            
            // If there's no pending move, complete the animation by returning to the target rotation.
            // Otherwise, the animation will end here, and the next sequence will pick up from this state.
            if (!AnimationController::HasPendingMoves())
            {
                track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeOutQuad});
            }
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeOutCubic});
            track->AddKeyframe({0.1f, 0.0f, Easing::easeInOutCubic});
            track->AddKeyframe({0.35f, -0.25f, Easing::easeInOutCubic});
            track->AddKeyframe({0.75f, 0.05f, Easing::easeInCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences
