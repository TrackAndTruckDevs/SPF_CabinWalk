#include "SofaToStanding.hpp"
#include "Animation/AnimationConfig.hpp"
#include "Animation/Track.hpp"
#include "Animation/Easing/Easing.hpp"

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreateSofaToStandingSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(Animation::Config::SOFA_TO_STANDING_DURATION); // Use defined duration

        // --- Position X Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::linear});
            track->AddKeyframe({0.5f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeInQuad});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y Track (Stand Up) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::linear});
            track->AddKeyframe({0.2f, start_state.position.y + 0.1f, Easing::easeOutQuad});
            track->AddKeyframe({0.6f, target_state.position.y - 0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeOutQuint});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::linear});
            track->AddKeyframe({0.4f, start_state.position.z - 0.05f, Easing::easeOutQuad});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::easeInCubic});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::linear});
            track->AddKeyframe({0.45f, target_state.rotation.x + 0.15f, Easing::easeOutQuad});
            track->AddKeyframe({0.75f, target_state.rotation.x - 0.1f, Easing::easeOutQuad});
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeInCubic});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch Track ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::linear});
            track->AddKeyframe({0.25f, target_state.rotation.y - 0.25f, Easing::easeOutQuad});
            track->AddKeyframe({0.6f, target_state.rotation.y - 0.05f, Easing::easeOutQuad});
            track->AddKeyframe({0.85f, target_state.rotation.y + 0.15f, Easing::easeOutQuad});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeInCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences
