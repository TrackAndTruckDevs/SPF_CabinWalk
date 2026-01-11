#define _USE_MATH_DEFINES
#include <cmath>
#include "Animation/StandingAnimController.hpp"
#include "Animation/Sequences/StandingStances.hpp"
#include "Animation/AnimationSequence.hpp"
#include "Animation/AnimationController.hpp"
#include "Animation/AnimationConfig.hpp"
#include "SPF_CabinWalk.hpp"

#include <memory>

namespace SPF_CabinWalk::StandingAnimController
{
    using namespace Animation::Config;
    // =================================================================================================
    // Internal State
    // =================================================================================================
    static PluginContext* g_stand_ctx = nullptr;
    static Stance g_current_stance = Stance::Standing;
    static AnimationController::CameraPosition g_final_destination; // For multi-stage transitions
    static Stance g_transition_to_stance = Stance::Standing;
    static std::unique_ptr<Animation::AnimationSequence> g_active_sequence = nullptr;
    static uint64_t last_simulation_time = 0;
    static bool g_has_taken_first_step = false;

    // Timers for holding camera in a trigger zone
    static uint64_t g_time_in_crouch_zone = 0;
    static uint64_t g_time_in_tiptoe_zone = 0;
    static uint64_t g_time_in_standup_zone = 0;
    static uint64_t g_time_in_standdown_zone = 0;

    static AnimationController::GazeDirection GetGazeDirection(float yaw_radians)
    {
        // Define thresholds in radians (M_PI is 180 degrees)
        const float quarter_pi = M_PI / 4.0f;     // 45 degrees
        const float three_quarters_pi = 3.0f * M_PI / 4.0f; // 135 degrees

        if (yaw_radians >= -quarter_pi && yaw_radians <= quarter_pi)
        {
            return AnimationController::GazeDirection::Forward;
        }
        else if (yaw_radians > quarter_pi && yaw_radians <= three_quarters_pi)
        {
            return AnimationController::GazeDirection::Left;
        }
        else if (yaw_radians < -quarter_pi && yaw_radians >= -three_quarters_pi)
        {
            return AnimationController::GazeDirection::Right;
        }
        else // yaw_radians > three_quarters_pi || yaw_radians < -three_quarters_pi
        {
            return AnimationController::GazeDirection::Backward;
        }
    }


    void Initialize(PluginContext* ctx)
    {
        g_stand_ctx = ctx;
    }

    void Update(const Animation::CurrentCameraState& current_state)
    {
        if (!g_stand_ctx || !g_stand_ctx->coreAPI)
        {
            return;
        }

        SPF_Timestamps timestamps;
        g_stand_ctx->coreAPI->telemetry->GetTimestamps(g_stand_ctx->telemetryHandle, &timestamps);
        uint64_t delta_time_ms = timestamps.simulation - last_simulation_time;
        last_simulation_time = timestamps.simulation;

        // --- Handle active animation sequence ---
        if (g_active_sequence && g_active_sequence->IsPlaying())
        {
            bool is_playing = g_active_sequence->Update(delta_time_ms, g_stand_ctx->cameraAPI);

            if (!is_playing)
            {
                // Animation finished, transition to the new stable stance
                g_active_sequence.reset();
                if (g_current_stance == Stance::InTransition)
                {
                    g_current_stance = g_transition_to_stance;
                }
            }
            return; // Exit if an animation is still playing
        }

        // --- Handle stance transitions based on pitch (only if no animation is active) ---
        switch (g_current_stance)
        {
            case Stance::Standing:
            {
                // Reset timers for other states
                g_time_in_standup_zone = 0;
                g_time_in_standdown_zone = 0;

                // Continuous walking logic
                if (SPF_CabinWalk::IsWalkKeyDown())
                {
                    if (!g_active_sequence || !g_active_sequence->IsPlaying()) // Ensure no other animation is playing
                    {
                        const bool is_walking_forward = (current_state.rotation.x >= -M_PI_2 && current_state.rotation.x <= M_PI_2);
                        const float next_z_pos = current_state.position.z + (is_walking_forward ? -Animation::Config::Walking::STEP_AMOUNT : Animation::Config::Walking::STEP_AMOUNT);

                        if (next_z_pos >= Animation::Config::Walking::MIN_Z && next_z_pos <= Animation::Config::Walking::MAX_Z) // Check boundaries before triggering step
                        {
                            if (!g_has_taken_first_step)
                            {
                                g_active_sequence = AnimationSequences::CreateDynamicFirstStepSequence(current_state, is_walking_forward);
                                g_has_taken_first_step = true;
                            }
                            else
                            {
                                g_active_sequence = AnimationSequences::CreateWalkStepSequence(current_state, is_walking_forward);
                            }

                            if (g_active_sequence)
                            {
                                g_active_sequence->Start(current_state);
                                return; // Block other checks until walk animation finishes
                            }
                        }
                    }
                }
                else // Walk key is NOT down
                {
                    g_has_taken_first_step = false; // Reset first step flag
                }

                // If not walking, and no animation is playing, then check for other stance changes.
                if (!g_active_sequence || !g_active_sequence->IsPlaying())
                {
                    // Check for crouch
                    if (current_state.rotation.y < Stances::Triggers::CROUCH_DOWN_ANGLE)
                    {
                        g_time_in_crouch_zone += delta_time_ms;
                        g_time_in_tiptoe_zone = 0; // Reset other timer
                        if (g_time_in_crouch_zone >= Stances::Triggers::HOLD_TIME_MS)
                        {
                            g_time_in_crouch_zone = 0;
                            AnimationController::GazeDirection current_gaze_for_stance = GetGazeDirection(current_state.rotation.x);
                            g_active_sequence = AnimationSequences::CreateCrouchDownSequence(current_state, current_gaze_for_stance);
                            if (g_active_sequence)
                            {
                                g_active_sequence->Start(current_state);
                                g_current_stance = Stance::InTransition;
                                g_transition_to_stance = Stance::Crouching;
                            }
                        }
                    }
                    // Check for tiptoes
                    else if (current_state.rotation.y > Stances::Triggers::TIPTOE_UP_ANGLE)
                    {
                        g_time_in_tiptoe_zone += delta_time_ms;
                        g_time_in_crouch_zone = 0; // Reset other timer
                        if (g_time_in_tiptoe_zone >= Stances::Triggers::HOLD_TIME_MS)
                        {
                            g_time_in_tiptoe_zone = 0;
                            AnimationController::GazeDirection current_gaze_for_stance = GetGazeDirection(current_state.rotation.x);
                            g_active_sequence = AnimationSequences::CreateTiptoeSequence(current_state, current_gaze_for_stance);
                            if (g_active_sequence)
                            {
                                g_active_sequence->Start(current_state);
                                g_current_stance = Stance::InTransition;
                                g_transition_to_stance = Stance::Tiptoes;
                            }
                        }
                    }
                    // Not in any trigger zone
                    else
                    {
                        g_time_in_crouch_zone = 0;
                        g_time_in_tiptoe_zone = 0;
                    }
                }
                break;
            }
            case Stance::Crouching:
            {
                g_time_in_crouch_zone = 0; // Reset other state timers
                g_time_in_tiptoe_zone = 0;

                if (current_state.rotation.y > Stances::Triggers::STAND_UP_ANGLE)
                {
                    g_time_in_standup_zone += delta_time_ms;
                    if (g_time_in_standup_zone >= Stances::Triggers::HOLD_TIME_MS)
                    {
                        g_time_in_standup_zone = 0;
                        TriggerStandUp(); // Use the refactored function
                    }
                }
                else
                {
                    g_time_in_standup_zone = 0;
                }
                break;
            }
            case Stance::Tiptoes:
            {
                g_time_in_crouch_zone = 0; // Reset other state timers
                g_time_in_tiptoe_zone = 0;

                if (current_state.rotation.y < Stances::Triggers::STAND_DOWN_ANGLE)
                {
                    g_time_in_standdown_zone += delta_time_ms;
                    if (g_time_in_standdown_zone >= Stances::Triggers::HOLD_TIME_MS)
                    {
                        g_time_in_standdown_zone = 0;
                        TriggerStandDown(); // Use the refactored function
                    }
                }
                else
                {
                    g_time_in_standdown_zone = 0;
                }
                break;
            }
            case Stance::InTransition: // Should be handled by the active_sequence check above
                break;
            case Stance::ReturningToHome:
            {
                // Logic for automatically walking back to Z-zero
                const float z_home = 0.0f;
                const float z_current = current_state.position.z;
                const float step_amount = Walking::STEP_AMOUNT;

                // Check if we are close enough to home (Z-zero)
                if (std::fabs(z_current - z_home) <= step_amount)
                {
                    // Close enough to home. Transition to sitting.
                    g_current_stance = Stance::Standing; // Reset stance
                    AnimationController::MoveTo(g_final_destination); // Trigger the final sit-down animation
                    return; // New animation started, exit update
                }
                else // Still far from home, trigger another walk step towards home
                {
                    // Determine if we need to walk forward (-Z) or backward (+Z)
                    bool is_walking_forward = (z_current > z_home); // If current Z is greater than zero, walk forward (-Z)

                    // Trigger a walk step.
                    TriggerWalkStepTowards(current_state, is_walking_forward);
                    return; // A new animation might have started, exit update
                }
            }
            default:
                break;
        }
    }

    void OnEnterStandingState()
    {
        g_current_stance = Stance::Standing;
        g_active_sequence.reset();
    }

    void TriggerWalkStepTowards(const Animation::CurrentCameraState& current_state, bool is_walking_forward)
    {
        // This function is called by AnimationController to make a step towards the home position.
        // It bypasses the IsWalkKeyDown() check because the intent to walk is already established.

        if (g_active_sequence && g_active_sequence->IsPlaying())
        {
            return; // Don't trigger a new step if an animation is already playing
        }

        const float next_z_pos = current_state.position.z + (is_walking_forward ? -Walking::STEP_AMOUNT : Walking::STEP_AMOUNT);

        if (next_z_pos >= Walking::MIN_Z && next_z_pos <= Walking::MAX_Z)
        {
            if (!g_has_taken_first_step)
            {
                // This 'first step' now implies turning to face the Z-axis of the STANDING_POSITION_TARGET.
                // The yaw component of STANDING_POSITION_TARGET should be used for rotation.
                // CreateDynamicFirstStepSequence might need to be adapted or a new sequence created
                // that handles turning towards STANDING_POSITION_TARGET.rotation.x (yaw).
                // For now, let's assume CreateDynamicFirstStepSequence can handle the target rotation or will be adapted.
                g_active_sequence = AnimationSequences::CreateDynamicFirstStepSequence(current_state, is_walking_forward); // This sequence should eventually handle yaw correction
                g_has_taken_first_step = true;
            }
            else
            {
                g_active_sequence = AnimationSequences::CreateWalkStepSequence(current_state, is_walking_forward);
            }

            if (g_active_sequence)
            {
                g_active_sequence->Start(current_state);
            }
        }
        else
        {
            // If boundaries would be exceeded, reset first step flag as we can't walk further in this direction
            g_has_taken_first_step = false;
        }
    }

        bool CanSitDown(AnimationController::CameraPosition target)
        {
            // Get current camera state
            Animation::CurrentCameraState current_state;
            g_stand_ctx->cameraAPI->GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);
            // Rotation not strictly needed for this check, but good practice to get full state if available.
            g_stand_ctx->cameraAPI->GetInteriorHeadRot(&current_state.rotation.x, &current_state.rotation.y);
            current_state.rotation.z = 0.0f; // Roll is not retrieved
    
            const float z_current = current_state.position.z;
            const float step_amount = Walking::STEP_AMOUNT;
    
            if (z_current > step_amount) // Only initiate walk back if Z is positive and beyond step_amount
            {
                // Too far from Z-zero on the positive side, initiate walk back
                g_current_stance = Stance::ReturningToHome;
                g_final_destination = target;
                return false;
            }
                    else
                    {
                        // Close enough to Z-zero (positive or negative), OR too far on the negative side (in which case we ignore and sit)
                        return true;
                    }
                }
            
                Stance GetCurrentStance()
                {
                    return g_current_stance;
                }
            
                bool IsAnimating()
                {
                    return g_active_sequence && g_active_sequence->IsPlaying();
                }
            
                void TriggerStandUp()
                {
                    if (IsAnimating() || g_current_stance != Stance::Crouching)
                    {
                        return; // Don't interrupt or trigger from wrong state
                    }
            
                    // Need current state to create the animation
                    Animation::CurrentCameraState current_state;
                    g_stand_ctx->cameraAPI->GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);
                    g_stand_ctx->cameraAPI->GetInteriorHeadRot(&current_state.rotation.x, &current_state.rotation.y);
                    current_state.rotation.z = 0.0f;
            
                    AnimationController::GazeDirection current_gaze = GetGazeDirection(current_state.rotation.x);
                    g_active_sequence = AnimationSequences::CreateStandUpSequence(current_state, current_gaze);
                    if (g_active_sequence)
                    {
                        g_active_sequence->Start(current_state);
                        g_current_stance = Stance::InTransition;
                        g_transition_to_stance = Stance::Standing;
                    }
                }
            
                void TriggerStandDown()
                {
                    if (IsAnimating() || g_current_stance != Stance::Tiptoes)
                    {
                        return; // Don't interrupt or trigger from wrong state
                    }
            
                    // Need current state to create the animation
                    Animation::CurrentCameraState current_state;
                    g_stand_ctx->cameraAPI->GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);
                    g_stand_ctx->cameraAPI->GetInteriorHeadRot(&current_state.rotation.x, &current_state.rotation.y);
                    current_state.rotation.z = 0.0f;
            
                    AnimationController::GazeDirection current_gaze = GetGazeDirection(current_state.rotation.x);
                    g_active_sequence = AnimationSequences::CreateStandDownSequence(current_state, current_gaze);
                    if (g_active_sequence)
                    {
                        g_active_sequence->Start(current_state);
                        g_current_stance = Stance::InTransition;
                        g_transition_to_stance = Stance::Standing;
                    }
                }
            }
