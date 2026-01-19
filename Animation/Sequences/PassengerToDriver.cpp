#include "Animation/Sequences/PassengerToDriver.hpp"
#include "SPF_CabinWalk.hpp" // For g_ctx

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreatePassengerToDriverSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(g_ctx.settings.animation_durations.main_animation_speed.passenger_to_driver * 1000);

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
            track->AddKeyframe({0.3f, start_state.position.y, Easing::linear});
            track->AddKeyframe({0.35f, g_ctx.settings.general.height, Easing::easeOutCubic});
            track->AddKeyframe({0.55f, g_ctx.settings.general.height + 0.01f, Easing::easeInQuint});
            track->AddKeyframe({0.85f, g_ctx.settings.general.height, Easing::linear});
            // track->AddKeyframe({0.85f, 0.250f, Easing::easeInQuint});
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInOutCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z Track (Move Forward/Backward) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::linear});
            track->AddKeyframe({0.15f, -0.1f, Easing::easeOutExpo});
            track->AddKeyframe({0.50f, -0.35f, Easing::easeInOutCubic});
            track->AddKeyframe({0.75f, -0.35f, Easing::linear});
            track->AddKeyframe({0.85f, -0.15f, Easing::linear});
            track->AddKeyframe({0.97f, -0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::linear});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw Track (Look Left/Right) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            const float direction_multiplier = (g_ctx.settings.general.cabin_layout == LHD) ? 1.0f : -1.0f;
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::linear});
            track->AddKeyframe({0.2f, 1.35f * direction_multiplier, Easing::easeOutCubic});
            track->AddKeyframe({0.65f, 0.15f * direction_multiplier, Easing::linear});
            track->AddKeyframe({1.0f, 0.0f, Easing::easeInOutCubic});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch Track (Look Up/Down) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::linear});
            track->AddKeyframe({0.35f, -0.15f , Easing::easeOutCubic});
            track->AddKeyframe({0.65f, -0.55f, Easing::easeInOutCubic});
            track->AddKeyframe({0.95f, 0.05f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeInOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences