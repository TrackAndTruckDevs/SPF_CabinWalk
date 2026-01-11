#include "StandingToPassenger.hpp"
#include "Animation/AnimationConfig.hpp" // For animation durations

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreateStandingToPassengerSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(Animation::Config::DRIVER_TO_PASSENGER_DURATION); // Using same duration for now

        // --- Position X Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({0.35f, start_state.position.x, Easing::easeInCubic});
            //track->AddKeyframe({0.5f, target_state.position.x + 0.15f, Easing::easeOutCubic});
            track->AddKeyframe({0.85f, target_state.position.x, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInCubic});
            track->AddKeyframe({0.30f, start_state.position.y + 0.01f, Easing::easeOutCubic});
            track->AddKeyframe({0.45f, start_state.position.y, Easing::easeOutCubic});
            track->AddKeyframe({0.55f, start_state.position.y, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::easeInOutCubic});
            track->AddKeyframe({0.15f, - 0.15f, Easing::easeOutCubic});
            track->AddKeyframe({0.25f, - 0.15f, Easing::easeOutCubic});
            track->AddKeyframe({0.55f, - 0.35f, Easing::easeInOutCubic});
            track->AddKeyframe({0.85f, - 0.15f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::easeInOutCubic});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({0.15f, 0.0f, Easing::easeInOutCubic});
            track->AddKeyframe({0.45f, - 0.75f, Easing::easeInOutCubic});
            track->AddKeyframe({0.65f, + 0.15f, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, 0.0f, Easing::easeOutQuad});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeOutCubic});
            track->AddKeyframe({0.1f, -0.1f, Easing::easeInOutCubic});
            track->AddKeyframe({0.35f, -0.45f, Easing::easeInOutCubic});
            track->AddKeyframe({0.85f, 0.15f, Easing::easeInCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences
