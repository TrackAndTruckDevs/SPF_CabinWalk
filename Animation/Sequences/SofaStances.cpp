#include "SofaStances.hpp"
#include "Animation/AnimationConfig.hpp"
#include "Animation/Track.hpp"
#include "Animation/Easing/Easing.hpp"

namespace SPF_CabinWalk::AnimationSequences
{
    std::unique_ptr<Animation::AnimationSequence> CreateSofaSit1ToLieSequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(5000000);

        // --- Position X (Sliding along the sofa) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({0.75f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeInOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y (Lowering into lying position) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            //track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInCubic});
            track->AddKeyframe({0.65f, start_state.position.y, Easing::easeOutCubic}); // Dip slightly lower
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z (Stays constant) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::easeInCubic});
            track->AddKeyframe({0.5f, target_state.position.z, Easing::easeInOutCubic});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw (Look slightly left/right) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({0.55f, target_state.rotation.x, Easing::easeInOutCubic}); // Small look aside
            track->AddKeyframe({0.85f, target_state.rotation.x + 0.25f, Easing::easeInOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeInCubic});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch (The 'lying down' head movement) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeOutCubic});
            track->AddKeyframe({0.35f, start_state.rotation.y +0.25f, Easing::easeInCubic});
            track->AddKeyframe({0.65f, -0.05f, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateSofaLieToSit2Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(2500000);

        // --- Position X (Sliding to the new spot) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.5f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeInOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y (Rising to sitting height) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInCubic});
            track->AddKeyframe({0.5f, target_state.position.y, Easing::easeOutQuad}); // Slight delay before rising
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeOutCubic});
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z (Stays constant) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.5f, start_state.position.z, Easing::linear});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::linear});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Pitch (The 'sitting up' head movement) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeInCubic});
            track->AddKeyframe({0.3f, -0.4f, Easing::easeOutCubic}); // Coming from a 'lying' pitch
            track->AddKeyframe({0.9f, 0.1f, Easing::easeInQuad}); // Overshoot slightly forward
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeOutCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        // --- Rotation Yaw (Look ahead) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeInCubic});
            seq->AddRotationYawTrack(std::move(track));
        }

        return seq;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateSofaSit2ToSit1Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(1200000); // 1.2 seconds for a shorter slide

        // --- Position X (Sliding back to the first spot) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeInOutCubic});
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y (Slight push-up to move) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInQuad});
            track->AddKeyframe({0.5f, start_state.position.y + 0.05f, Easing::easeOutQuad}); // Push up
            track->AddKeyframe({1.0f, target_state.position.y, Easing::easeInCubic}); // Settle down
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z (Stays constant) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::linear});
            track->AddKeyframe({1.0f, target_state.position.z, Easing::linear});
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Yaw (Slight head turn during slide) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({0.4f, start_state.rotation.x - 0.15f, Easing::easeOutQuad}); // Look towards destination
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::easeInQuad});
            seq->AddRotationYawTrack(std::move(track));
        }

        // --- Rotation Pitch (Keep it steady) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeOutCubic});
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeInCubic});
            seq->AddRotationPitchTrack(std::move(track));
        }

        return seq;
    }
    
    std::unique_ptr<Animation::AnimationSequence> CreateSofaLieToSofa1Sequence(
        const Animation::CurrentCameraState& start_state,
        const Animation::CurrentCameraState& target_state
    )
    {
        auto seq = std::make_unique<Animation::AnimationSequence>();
        seq->Initialize(2800000); // 2.8 seconds for sit up + slide

        // --- Position X (Slide from lie spot to sit spot) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.x, Easing::linear});
            track->AddKeyframe({0.15f, start_state.position.x, Easing::easeInCubic}); // Hold position while sitting up
            track->AddKeyframe({0.75f, target_state.position.x, Easing::easeInCubic}); // Hold position while sitting up
            track->AddKeyframe({1.0f, target_state.position.x, Easing::easeInOutCubic}); // Slide in the last part
            seq->AddPositionXTrack(std::move(track));
        }

        // --- Position Y (Rising to sitting height) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.y, Easing::easeInCubic});
            track->AddKeyframe({0.6f, target_state.position.y, Easing::easeOutCubic}); // Rise up first
            track->AddKeyframe({1.0f, target_state.position.y, Easing::linear}); // Hold height while sliding
            seq->AddPositionYTrack(std::move(track));
        }

        // --- Position Z (Move along with X) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.position.z, Easing::linear});
            track->AddKeyframe({0.85f, start_state.position.z, Easing::easeInCubic}); // Hold Z while sitting up
            track->AddKeyframe({1.0f, target_state.position.z, Easing::easeInOutCubic}); // Move Z during the slide
            seq->AddPositionZTrack(std::move(track));
        }

        // --- Rotation Pitch (The 'sitting up' head movement) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.y, Easing::easeInCubic}); // Start from lying pitch
            track->AddKeyframe({0.5f, -0.4f, Easing::easeOutCubic});
            track->AddKeyframe({0.9f, 0.1f, Easing::easeInQuad}); // Overshoot slightly
            track->AddKeyframe({1.0f, target_state.rotation.y, Easing::easeOutCubic}); // Settle to final sitting pitch
            seq->AddRotationPitchTrack(std::move(track));
        }

        // --- Rotation Yaw (Look ahead) ---
        {
            auto track = std::make_unique<Animation::Track<float>>();
            track->AddKeyframe({0.0f, start_state.rotation.x, Easing::easeOutCubic});
            track->AddKeyframe({0.6f, target_state.rotation.x - 1.0f, Easing::easeInCubic}); // Turn head while sitting up
            track->AddKeyframe({1.0f, target_state.rotation.x, Easing::linear});
            seq->AddRotationYawTrack(std::move(track));
        }

        return seq;
    }

} // namespace SPF_CabinWalk::AnimationSequences