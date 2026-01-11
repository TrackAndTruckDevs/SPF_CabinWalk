#include "DriverToPassenger.hpp"
#include "Animation/AnimationConfig.hpp" // For animation durations

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreateDriverToPassengerSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(Animation::Config::DRIVER_TO_PASSENGER_DURATION);

        // --- Position X Track (Move Right) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::linear});
            track->AddKeyframe({0.25f, start_state.position.x, Easing::linear});
            track->AddKeyframe({0.75f, target_state.position.x, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y Track (Move Up/Down) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::linear});
            track->AddKeyframe({0.35f, 0.250f, Easing::easeOutCubic});
            track->AddKeyframe({0.55f, 0.260f, Easing::easeInOutQuint});
            track->AddKeyframe({0.75f, 0.250f, Easing::easeInQuint});
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInOutCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z Track (Move Forward/Backward) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::linear});
            track->AddKeyframe({0.25f, -0.1f, Easing::easeOutExpo});
            track->AddKeyframe({0.50f, 0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({0.75f, -0.1f, Easing::easeInOutCubic});
            track->AddKeyframe({0.95f, -0.25f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::linear});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw Track (Look Left/Right) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::linear});
            track->AddKeyframe({0.2f, -1.15f, Easing::easeOutCubic});
            track->AddKeyframe({0.4f, -0.85f, Easing::easeInOutQuad});
            track->AddKeyframe({0.6f, -1.0f, Easing::easeInOutQuad});
            track->AddKeyframe({0.85f, 0.5f, Easing::easeInOutQuad});
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeInOutCubic});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch Track (Look Up/Down) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::linear});
            track->AddKeyframe({0.35f, 0.15f , Easing::easeOutCubic});
            track->AddKeyframe({0.65f, -0.75f, Easing::easeInOutCubic});
            track->AddKeyframe({0.85f, -0.3f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeInOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences