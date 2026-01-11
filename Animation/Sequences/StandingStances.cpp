#define _USE_MATH_DEFINES
#include <cmath>
#include "Animation/Sequences/StandingStances.hpp"
#include "Animation/AnimationConfig.hpp"
#include "Animation/Track.hpp"
#include "Animation/Easing/Easing.hpp"

namespace SPF_CabinWalk::AnimationSequences
{
    using namespace Animation::Config;
    std::unique_ptr<Animation::AnimationSequence> CreateCrouchDownSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze)
    {
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(Stances::CROUCH_DURATION);

        // Y-axis (vertical) movement
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y - Stances::CROUCH_DEPTH, Easing::easeOutCubic));
        sequence->AddPositionYTrack(std::move(y_track));

        auto x_track = std::make_unique<Animation::Track<float>>();
        auto z_track = std::make_unique<Animation::Track<float>>();

        switch (gaze)
        {
            case AnimationController::GazeDirection::Forward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.z - 0.07f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.z - 0.03f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Backward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.z + 0.07f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.z + 0.03f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Right:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.x + 0.07f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.x + 0.03f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Left:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.x - 0.07f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.x - 0.03f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x + 0.0f, Easing::easeOutQuint));
                break;
        }

        sequence->AddPositionXTrack(std::move(x_track));
        sequence->AddPositionZTrack(std::move(z_track));

        // Pitch offset to look straight ahead at the end
        auto pitch_track = std::make_unique<Animation::Track<float>>();
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.y, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.43f, initial_state.rotation.y + 0.07f, Easing::easeInOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.87f, 0.0f, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(1.0f, 0.0f, Easing::easeOutCubic));
        sequence->AddRotationPitchTrack(std::move(pitch_track));


        auto yaw_track = std::make_unique<Animation::Track<float>>();
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.3f, initial_state.rotation.x -0.03f, Easing::easeInOutQuint)); // Shake left
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.7f, initial_state.rotation.x +0.01f, Easing::easeInOutQuint));  // Shake right
        yaw_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        sequence->AddRotationYawTrack(std::move(yaw_track));

        return sequence;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateStandUpSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze)
    {
        (void)gaze; // TODO: Implement dynamic rocking based on gaze
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(Stances::CROUCH_DURATION);

        // Y-axis (vertical) movement - from crouch to standing
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic)); 
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y + Stances::CROUCH_DEPTH, Easing::easeOutCubic));
        sequence->AddPositionYTrack(std::move(y_track));

        auto x_track = std::make_unique<Animation::Track<float>>();
        auto z_track = std::make_unique<Animation::Track<float>>();

        switch (gaze)
        {
            case AnimationController::GazeDirection::Forward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.z - 0.07f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.z - 0.05f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Backward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.z + 0.07f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.z + 0.05f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Right:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.x + 0.07f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.x + 0.05f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x + 0.0f, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Left:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.x - 0.07f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.65f, initial_state.position.x - 0.05f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.91f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x + 0.0f, Easing::easeOutQuint));
                break;
        }

        sequence->AddPositionXTrack(std::move(x_track));
        sequence->AddPositionZTrack(std::move(z_track));

        // Pitch offset to look straight ahead at the end
        auto pitch_track = std::make_unique<Animation::Track<float>>();
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.y, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.45f, initial_state.rotation.y - 0.07f, Easing::easeInOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.87f, 0.0f, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(1.0f, 0.0f, Easing::easeOutCubic));
        sequence->AddRotationPitchTrack(std::move(pitch_track));


        auto yaw_track = std::make_unique<Animation::Track<float>>();
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.3f, initial_state.rotation.x -0.03f, Easing::easeInOutQuint)); // Shake left
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.7f, initial_state.rotation.x +0.01f, Easing::easeInOutQuint));  // Shake right
        yaw_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        sequence->AddRotationYawTrack(std::move(yaw_track));

        return sequence;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateTiptoeSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze)
    {
        (void)gaze; // TODO: Implement dynamic rocking based on gaze
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(Stances::TIPTOE_DURATION);

        // Y-axis (vertical) movement
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y + Stances::TIPTOE_HEIGHT, Easing::easeOutCubic));
        sequence->AddPositionYTrack(std::move(y_track));

        auto x_track = std::make_unique<Animation::Track<float>>();
        auto z_track = std::make_unique<Animation::Track<float>>();

        switch (gaze)
        {
            case AnimationController::GazeDirection::Forward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.25f, initial_state.position.z - 0.13f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z, Easing::easeInOutQuint));
                break;

            case AnimationController::GazeDirection::Backward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.25f, initial_state.position.z + 0.13f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z, Easing::easeInOutQuint));
                break;

            case AnimationController::GazeDirection::Right:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.25f, initial_state.position.x - 0.13f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x, Easing::easeInOutQuint));
                break;

            case AnimationController::GazeDirection::Left:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.25f, initial_state.position.x + 0.13f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x, Easing::easeInOutQuint));
                break;
        }

        sequence->AddPositionXTrack(std::move(x_track));
        sequence->AddPositionZTrack(std::move(z_track));

        // Pitch offset to look straight ahead at the end
        auto pitch_track = std::make_unique<Animation::Track<float>>();
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.y, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.45f, initial_state.rotation.y + 0.07f, Easing::easeInOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.87f, 0.0f, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(1.0f, 0.0f, Easing::easeOutCubic));
        sequence->AddRotationPitchTrack(std::move(pitch_track));


        auto yaw_track = std::make_unique<Animation::Track<float>>();
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.3f, initial_state.rotation.x -0.02f, Easing::easeInQuint)); // Shake left
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.7f, initial_state.rotation.x +0.02f, Easing::easeOutQuint));  // Shake right
        yaw_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        sequence->AddRotationYawTrack(std::move(yaw_track));

        return sequence;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateStandDownSequence(const Animation::CurrentCameraState& initial_state, AnimationController::GazeDirection gaze)
    {
        (void)gaze; // TODO: Implement dynamic rocking based on gaze
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(Stances::TIPTOE_DURATION);

        // Y-axis (vertical) movement - from tiptoes to standing
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y - Stances::TIPTOE_HEIGHT, Easing::easeOutCubic));
        sequence->AddPositionYTrack(std::move(y_track));

        auto x_track = std::make_unique<Animation::Track<float>>();
        auto z_track = std::make_unique<Animation::Track<float>>();

        switch (gaze)
        {
            case AnimationController::GazeDirection::Forward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.85f, initial_state.position.z + 0.01f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Backward:
                z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z + 0.0f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(0.85f, initial_state.position.z - 0.01f, Easing::easeInOutQuint));
                z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.z, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Right:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.85f, initial_state.position.x + 0.01f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x, Easing::easeOutQuint));
                break;

            case AnimationController::GazeDirection::Left:
                x_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.x + 0.0f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(0.85f, initial_state.position.x - 0.01f, Easing::easeInOutQuint));
                x_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.x, Easing::easeOutQuint));
                break;
        }

        sequence->AddPositionXTrack(std::move(x_track));
        sequence->AddPositionZTrack(std::move(z_track));

        // Pitch offset to look straight ahead at the end
        auto pitch_track = std::make_unique<Animation::Track<float>>();
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.y, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.45f, initial_state.rotation.y - 0.09f, Easing::easeInOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(0.87f, 0.0f, Easing::easeOutCubic));
        pitch_track->AddKeyframe(Animation::Keyframe<float>(1.0f, 0.0f, Easing::easeOutCubic));
        sequence->AddRotationPitchTrack(std::move(pitch_track));


        auto yaw_track = std::make_unique<Animation::Track<float>>();
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.3f, initial_state.rotation.x -0.02f, Easing::easeInQuint)); // Shake left
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.7f, initial_state.rotation.x +0.02f, Easing::easeOutQuint));  // Shake right
        yaw_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.rotation.x, Easing::easeInOutQuint));
        sequence->AddRotationYawTrack(std::move(yaw_track));

        return sequence;
    }


    std::unique_ptr<Animation::AnimationSequence> CreateWalkStepSequence(const Animation::CurrentCameraState& initial_state, bool is_walking_forward)
    {
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(Walking::STEP_DURATION);

        // --- Z-axis Track (Walking forward/backward) ---
        auto z_track = std::make_unique<Animation::Track<float>>();
        float z_target = initial_state.position.z + (is_walking_forward ? -Walking::STEP_AMOUNT : Walking::STEP_AMOUNT);
        z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z, Easing::linear));
        z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, z_target, Easing::linear));
        sequence->AddPositionZTrack(std::move(z_track));

        // --- Y-axis Track (Head bobbing) ---
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(0.5f, initial_state.position.y + Walking::BOB_AMOUNT, Easing::easeInCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y, Easing::easeInCubic));
        sequence->AddPositionYTrack(std::move(y_track));

        return sequence;
    }

    std::unique_ptr<Animation::AnimationSequence> CreateDynamicFirstStepSequence(const Animation::CurrentCameraState& initial_state, bool is_walking_forward)
    {
        // Calculate dynamic turn duration
        const float current_yaw = initial_state.rotation.x;
        float target_yaw = is_walking_forward ? 0.0f : M_PI; // Start with +180

        if (!is_walking_forward)
        {
            // We are walking backward. Target is 180 degrees.
            // Should we use +180 (M_PI) or -180 (-M_PI)?
            // Choose the one that results in a smaller turn angle.
            // If current_yaw is negative, turning to -M_PI is shorter.
            if (current_yaw < 0)
            {
                target_yaw = -M_PI;
            }
        }
        float angle_to_turn = std::fabs(current_yaw - target_yaw);

        // Handle wrapping for angle_to_turn
        if (angle_to_turn > M_PI) {
            angle_to_turn = 2 * M_PI - angle_to_turn;
        }

        const uint64_t base_turn_duration_ms = Walking::FirstStep::BASE_TURN_DURATION;
        const uint64_t extra_turn_duration_ms_per_pi = Walking::FirstStep::EXTRA_TURN_DURATION_PER_PI;
        uint64_t turn_duration_ms = base_turn_duration_ms + static_cast<uint64_t>((angle_to_turn / M_PI) * extra_turn_duration_ms_per_pi);

        // Ensure minimum total duration for a step to occur
        const uint64_t walk_step_animation_part_ms = Walking::STEP_DURATION;
        if (turn_duration_ms < walk_step_animation_part_ms) {
            turn_duration_ms = walk_step_animation_part_ms;
        }
        
        auto sequence = std::make_unique<Animation::AnimationSequence>();
        sequence->Initialize(turn_duration_ms);

        // --- Yaw Track (Head Alignment) ---
        auto yaw_track = std::make_unique<Animation::Track<float>>();
        yaw_track->AddKeyframe(Animation::Keyframe<float>(0.0f, current_yaw, Easing::easeOutCubic));
        yaw_track->AddKeyframe(Animation::Keyframe<float>(1.0f, target_yaw, Easing::easeOutCubic));
        sequence->AddRotationYawTrack(std::move(yaw_track));

        // --- Z-axis Track (Step) ---
        const float step_amount = Walking::STEP_AMOUNT;
        const float z_target = initial_state.position.z + (is_walking_forward ? -step_amount : step_amount);
        
        // Walk part starts in the last 250ms
        const float walk_start_time_ratio = static_cast<float>(turn_duration_ms - walk_step_animation_part_ms) / static_cast<float>(turn_duration_ms);

        auto z_track = std::make_unique<Animation::Track<float>>();
        z_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.z, Easing::linear));
        if (walk_start_time_ratio > 0.0f) { // If there's a delay before walk starts
            z_track->AddKeyframe(Animation::Keyframe<float>(walk_start_time_ratio - 0.001f, initial_state.position.z, Easing::linear)); // Hold position
        }
        z_track->AddKeyframe(Animation::Keyframe<float>(1.0f, z_target, Easing::linear));
        sequence->AddPositionZTrack(std::move(z_track));

        // --- Y-axis Track (Head bobbing) ---
        const float bob_amount = Walking::BOB_AMOUNT;
        auto y_track = std::make_unique<Animation::Track<float>>();
        y_track->AddKeyframe(Animation::Keyframe<float>(0.0f, initial_state.position.y, Easing::easeOutCubic));
        if (walk_start_time_ratio > 0.0f) { // If there's a delay before bob starts
            y_track->AddKeyframe(Animation::Keyframe<float>(walk_start_time_ratio - 0.001f, initial_state.position.y, Easing::easeOutCubic)); // Hold position
        }
        y_track->AddKeyframe(Animation::Keyframe<float>(walk_start_time_ratio + (1.0f - walk_start_time_ratio) * 0.5f, initial_state.position.y + bob_amount, Easing::easeInCubic));
        y_track->AddKeyframe(Animation::Keyframe<float>(1.0f, initial_state.position.y, Easing::easeInCubic));
        sequence->AddPositionYTrack(std::move(y_track));
        
        return sequence;
    }

} // namespace SPF_CabinWalk::AnimationSequences
