#include "Animation/AnimationController.hpp"
#include "SPF_CabinWalk.hpp"
#include "Hooks/CameraHookManager.hpp"
#include "Animation/StandingAnimController.hpp" 
#include <utility> // For std::pair
#include <map>
#include <functional>
#include <memory>

#include "Sequences/DriverToPassenger.hpp"
#include "Sequences/PassengerToDriver.hpp"
#include "Sequences/DriverToStanding.hpp"
#include "Sequences/StandingToDriver.hpp"
#include "Sequences/PassengerToStanding.hpp"
#include "Sequences/StandingToPassenger.hpp"
#include "Sequences/StandingToSofa.hpp"
#include "Sequences/SofaToStanding.hpp"
#include "Sequences/SofaStances.hpp"

namespace SPF_CabinWalk::AnimationController
{
    // =================================================================================================
    // Internal State
    // =================================================================================================
    static PluginContext *g_anim_ctx = nullptr;

    // --- New Animation System State ---
    static std::unique_ptr<Animation::AnimationSequence> g_active_sequence = nullptr;
    static CameraPosition g_current_pos = CameraPosition::Driver;
    static CameraPosition g_target_pos = CameraPosition::Driver;

    // Factory map to store functions that create AnimationSequence objects for specific transitions
    static std::map<
        std::pair<CameraPosition, CameraPosition>,
        std::function<std::unique_ptr<Animation::AnimationSequence>(const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state)>
    > g_sequence_factory;

    // Cache for the driver's initial state, to be used for the return journey
    static Animation::CurrentCameraState g_cached_driver_state;
    // Tracks simulation time to calculate delta time for animations
    static uint64_t last_simulation_time = 0;
    // Stores a sequence of moves for a chained animation.
    static std::queue<CameraPosition> g_pending_moves;
    // Flag to indicate that settings have been updated and may need to be reapplied.
    static bool g_settings_dirty = false;

    // =================================================================================================
    // Internal Helpers
    // =================================================================================================
    
    Animation::Transform GetTargetTransformForPosition(CameraPosition pos)
    {
        switch (pos)
        {
            case CameraPosition::Passenger:
                return { g_anim_ctx->settings.positions.passenger_seat.position, g_anim_ctx->settings.positions.passenger_seat.rotation };
            case CameraPosition::Standing:
                return { g_anim_ctx->settings.positions.standing.position, g_anim_ctx->settings.positions.standing.rotation };
            case CameraPosition::SofaSit1:
                return { g_anim_ctx->settings.positions.sofa_sit1.position, g_anim_ctx->settings.positions.sofa_sit1.rotation };
            case CameraPosition::SofaLie:
                return { g_anim_ctx->settings.positions.sofa_lie.position, g_anim_ctx->settings.positions.sofa_lie.rotation };
            case CameraPosition::SofaSit2:
                return { g_anim_ctx->settings.positions.sofa_sit2.position, g_anim_ctx->settings.positions.sofa_sit2.rotation };
            case CameraPosition::Driver:
                // Driver position is dynamic and cached, not from settings.
                return { g_cached_driver_state.position, g_cached_driver_state.rotation };
            default:
                return { {0,0,0}, {0,0,0} };
        }
    }

    // =================================================================================================
    // Public Functions
    // =================================================================================================

    void NotifySettingsUpdated()
    {
        g_settings_dirty = true;
    }

    void Initialize(PluginContext *ctx)
    {
        g_anim_ctx = ctx;
        g_current_pos = CameraPosition::Driver; // Always start at driver's seat by default

        // Initialize sub-controllers
        StandingAnimController::Initialize(ctx);

        // --- Register Actual Sequence Factories ---
        RegisterSequence(CameraPosition::Driver, CameraPosition::Passenger,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreateDriverToPassengerSequence(start_state, target_state);
            }
        );

        RegisterSequence(CameraPosition::Passenger, CameraPosition::Driver,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreatePassengerToDriverSequence(start_state, target_state);
            }
        );

        // --- Driver <-> Standing Sequences ---
        RegisterSequence(CameraPosition::Driver, CameraPosition::Standing,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreateDriverToStandingSequence(start_state, target_state);
            }
        );

        RegisterSequence(CameraPosition::Standing, CameraPosition::Driver,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreateStandingToDriverSequence(start_state, target_state);
            }
        );

        // --- Passenger <-> Standing Sequences ---
        RegisterSequence(CameraPosition::Passenger, CameraPosition::Standing,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreatePassengerToStandingSequence(start_state, target_state);
            }
        );

        RegisterSequence(CameraPosition::Standing, CameraPosition::Passenger,
            [](const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state) {
                return AnimationSequences::CreateStandingToPassengerSequence(start_state, target_state);
            }
        );

        // --- Sofa External Sequences ---
        RegisterSequence(CameraPosition::Standing, CameraPosition::SofaSit1, AnimationSequences::CreateStandingToSofaSequence);
        RegisterSequence(CameraPosition::SofaSit1, CameraPosition::Standing, AnimationSequences::CreateSofaToStandingSequence);

        // --- Sofa Internal Sequences ---
        RegisterSequence(CameraPosition::SofaSit1, CameraPosition::SofaLie, AnimationSequences::CreateSofaSit1ToLieSequence);
        RegisterSequence(CameraPosition::SofaSit1, CameraPosition::SofaSit2, AnimationSequences::CreateSofaSit1ToSit2Sequence);
        RegisterSequence(CameraPosition::SofaLie, CameraPosition::SofaSit2, AnimationSequences::CreateSofaLieToSit2Sequence);
        RegisterSequence(CameraPosition::SofaLie, CameraPosition::SofaSit1, AnimationSequences::CreateSofaLieToSofa1Sequence); // Shortcut animation
        RegisterSequence(CameraPosition::SofaSit2, CameraPosition::SofaSit1, AnimationSequences::CreateSofaSit2ToSit1Sequence);
    }
    void Update()
    {
        if (!g_anim_ctx || !g_anim_ctx->coreAPI)
        {
            return;
        }

        // --- Handle settings update ---
        if (g_settings_dirty)
        {
            if (!IsAnimating() && !StandingAnimController::IsAnimating())
            {
                // If we are idle and settings have changed, snap to the new position for the current state.
                // This should ONLY apply to positions that are actually defined by settings.
                if (g_current_pos != CameraPosition::Driver)
                {
                    const auto& target_transform = GetTargetTransformForPosition(g_current_pos);
                    if (g_anim_ctx->cameraAPI)
                    {
                        g_anim_ctx->cameraAPI->Cam_SetInteriorSeatPos(target_transform.position.x, target_transform.position.y, target_transform.position.z);
                        g_anim_ctx->cameraAPI->Cam_SetInteriorHeadRot(target_transform.rotation.x, target_transform.rotation.y);
                        if (g_anim_ctx->loggerHandle) {
                            char log_buffer[256];
                            g_anim_ctx->formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "[AnimationController] Applied settings directly to camera for position %d.", static_cast<int>(g_current_pos));
                            g_anim_ctx->loadAPI->logger->Log(g_anim_ctx->loggerHandle, SPF_LOG_DEBUG, log_buffer);
                        }
                    }
                }

                // Notify the CameraHookManager that settings have changed so it can re-evaluate its state.
                // This needs to happen regardless of the current position.
                CameraHookManager::NotifySettingsUpdated();
                
                // Reset the flag ONLY after the settings have been applied.
                g_settings_dirty = false;
            }
        }

        // --- Handle pending chained animation ---
        if (HasPendingMoves())
        {
            // Check if we are in a neutral, non-animating state before triggering the next move
            if (!IsAnimating() && !StandingAnimController::IsAnimating() && StandingAnimController::GetCurrentStance() == StandingAnimController::Stance::Standing)
            {
                CameraPosition next_move = g_pending_moves.front();
                g_pending_moves.pop();
                MoveTo(next_move); // Trigger the next move in the sequence
                return; // The new MoveTo call will handle the rest of this frame
            }
        }

        // --- 1. Handle Major Transitions ---
        if (g_active_sequence && g_active_sequence->IsPlaying())
        {
            SPF_Timestamps timestamps;
            g_anim_ctx->coreAPI->telemetry->Tel_GetTimestamps(g_anim_ctx->telemetryHandle, &timestamps, sizeof(SPF_Timestamps));
            uint64_t delta_time_ms = timestamps.simulation - last_simulation_time;
            last_simulation_time = timestamps.simulation;

            bool is_playing = g_active_sequence->Update(delta_time_ms, g_anim_ctx->cameraAPI);

            if (!is_playing)
            {
                // Major animation finished
                g_active_sequence.reset();
                g_current_pos = g_target_pos;
                CameraHookManager::SetCurrentCameraPosition(g_current_pos);

                // If we arrived at the standing position, notify the standing controller
                if (g_current_pos == CameraPosition::Standing)
                {
                    StandingAnimController::OnEnterStandingState();
                }

                // --- Immediately trigger the next move in the chain if one exists ---
                if (HasPendingMoves())
                {
                    // Ensure we are in a neutral state before proceeding
                    if (!IsAnimating() && !StandingAnimController::IsAnimating())
                    {
                        CameraPosition next_move = g_pending_moves.front();
                        g_pending_moves.pop();
                        MoveTo(next_move);
                        return; // The new MoveTo call will handle the rest of this frame
                    }
                }
            }
        }
        // --- 2. Handle Standing "Sub-State" Animations ---
        else if (g_current_pos == CameraPosition::Standing)
        {
            Animation::CurrentCameraState current_state;
            g_anim_ctx->cameraAPI->Cam_GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);
            g_anim_ctx->cameraAPI->Cam_GetInteriorHeadRot(&current_state.rotation.x, &current_state.rotation.y);
            current_state.rotation.z = 0.0f; // Roll is not retrieved

            StandingAnimController::Update(current_state);
        }
    }

    void MoveTo(CameraPosition target)
    {
        if ((g_active_sequence && g_active_sequence->IsPlaying()) || StandingAnimController::IsAnimating())
        {
            return; // Animation already in progress in this or sub-controller
        }

        if (target == g_current_pos)
        {
            return; // Already at target position
        }

        if (!g_anim_ctx || !g_anim_ctx->cameraAPI)
        {
            return; // API not ready
        }

        // --- STANCE-BASED TRANSITIONS ---
        if (g_current_pos == CameraPosition::Standing && (target == CameraPosition::Driver || target == CameraPosition::Passenger || target == CameraPosition::SofaSit1))
        {
            StandingAnimController::Stance current_stance = StandingAnimController::GetCurrentStance();

            if (current_stance != StandingAnimController::Stance::Standing)
            {
                // Not in the base standing stance, so we need to transition to it first.
                QueueMove(target); // Set the final destination

                if (current_stance == StandingAnimController::Stance::Crouching)
                {
                    StandingAnimController::TriggerStandUp();
                }
                else if (current_stance == StandingAnimController::Stance::Tiptoes)
                {
                    StandingAnimController::TriggerStandDown();
                }
                // InTransition and WalkingToFinalDestination are busy states, IsAnimating() check should have caught them.
                return; // Exit, the Update loop will handle the pending move later.
            }
            
            // If we get here, stance is Standing, so check if we need to walk.
            float target_z = GetTargetZForPosition(target);

            if (target == CameraPosition::Driver || target == CameraPosition::Passenger)
            {
                // Special logic for seats: only walk if Z is non-negative
                Animation::CurrentCameraState current_state;
                g_anim_ctx->cameraAPI->Cam_GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);

                if (current_state.position.z < 0)
                {
                    // Z is negative, sit immediately by falling through
                }
                else
                {
                    // Z is non-negative, check if a walk is needed
                    if (!StandingAnimController::CanSitDown(target, target_z))
                    {
                        return; // Walk was initiated
                    }
                }
            }
            else if (target == CameraPosition::SofaSit1)
            {
                // Generic logic for sofa: always walk if not at the target Z
                if (!StandingAnimController::CanSitDown(target, target_z))
                {
                    return; // Walk was initiated
                }
            }
            // If we can sit immediately, fall through to the normal transition logic below.
        }

        // --- NORMAL TRANSITION LOGIC ---
        // Cache current camera state before starting animation
        Animation::CurrentCameraState initial_state;
        g_anim_ctx->cameraAPI->Cam_GetInteriorSeatPos(&initial_state.position.x, &initial_state.position.y, &initial_state.position.z);
        g_anim_ctx->cameraAPI->Cam_GetInteriorHeadRot(&initial_state.rotation.x, &initial_state.rotation.y); // .x=yaw, .y=pitch
        initial_state.rotation.z = 0.0f; // Roll is not retrieved by GetInteriorHeadRot

        // Look up the factory for the transition
        auto it = g_sequence_factory.find({g_current_pos, target});
        if (it != g_sequence_factory.end())
        {
            // If moving away from the driver's seat, cache its current state for the return trip.
            if (g_current_pos == CameraPosition::Driver)
            {
                g_cached_driver_state = initial_state;
            }

            // Determine the target_state for the animation
            Animation::CurrentCameraState animation_target_state;
            if (target == CameraPosition::Driver)
            {
                // For returning to driver, the target is the cached driver's state
                animation_target_state = g_cached_driver_state;
            }
            else
            {
                // For all other positions, get the target from our settings
                const auto& target_transform = GetTargetTransformForPosition(target);
                animation_target_state.position = target_transform.position;
                animation_target_state.rotation = target_transform.rotation;
                // Roll is currently not handled by CameraPositions or GetInteriorHeadRot, default to initial.
                animation_target_state.rotation.z = initial_state.rotation.z; // This might need adjustment if roll becomes configurable
            }

            // Found a factory, create the sequence and start it
            SPF_Timestamps timestamps;
            g_anim_ctx->coreAPI->telemetry->Tel_GetTimestamps(g_anim_ctx->telemetryHandle, &timestamps, sizeof(SPF_Timestamps));
            last_simulation_time = timestamps.simulation; // Reset time for new animation

            g_active_sequence = it->second(initial_state, animation_target_state);

            g_active_sequence->Start(initial_state);
            g_target_pos = target;

        }
        else
        {
            // No animation sequence defined for this transition, just snap to position
            // This case should ideally not happen if all transitions are defined
            g_current_pos = target;
            g_target_pos = target;
            g_active_sequence.reset();

            // Direct snap to target using settings or cached driver state
            if (target == CameraPosition::Driver)
            {
                if (g_anim_ctx->cameraAPI)
                {
                    g_anim_ctx->cameraAPI->Cam_SetInteriorSeatPos(g_cached_driver_state.position.x,
                                                               g_cached_driver_state.position.y,
                                                               g_cached_driver_state.position.z);
                    g_anim_ctx->cameraAPI->Cam_SetInteriorHeadRot(g_cached_driver_state.rotation.x,
                                                               g_cached_driver_state.rotation.y);
                }
            }
            else
            {
                const auto& target_transform = GetTargetTransformForPosition(target);
                if (g_anim_ctx->cameraAPI)
                {
                    g_anim_ctx->cameraAPI->Cam_SetInteriorSeatPos(target_transform.position.x,
                                                               target_transform.position.y,
                                                               target_transform.position.z);
                    g_anim_ctx->cameraAPI->Cam_SetInteriorHeadRot(target_transform.rotation.x,
                                                               target_transform.rotation.y);
                }
            }
            // Special handling for Standing position reset stance
            if (target == CameraPosition::Standing)
            {
                StandingAnimController::OnEnterStandingState();
            }
            CameraHookManager::SetCurrentCameraPosition(target);
        }
    }

    bool IsAnimating()
    {
        return g_active_sequence && g_active_sequence->IsPlaying();
    }

    CameraPosition GetCurrentPosition()
    {
        return g_current_pos;
    }

    void OnRequestMove(CameraPosition final_destination)
    {
        // Don't start a new sequence if one is already in progress
        if (HasPendingMoves() || IsAnimating() || StandingAnimController::IsAnimating())
        {
            return;
        }

        CameraPosition current_pos = GetCurrentPosition();
        if (current_pos == final_destination)
        {
            return;
        }

        ClearPendingMoves();

        // --- Path definitions ---

        // -- Specific Path: Seat-to-Seat --
        if ((current_pos == CameraPosition::Driver && final_destination == CameraPosition::Passenger) ||
            (current_pos == CameraPosition::Passenger && final_destination == CameraPosition::Driver))
        {
            // This is a direct, single-animation move.
            QueueMove(final_destination);
        }
        // -- Paths originating from the Sofa --
        else if (current_pos == CameraPosition::SofaLie || current_pos == CameraPosition::SofaSit2 || current_pos == CameraPosition::SofaSit1)
        {
            bool needs_to_stand = (final_destination == CameraPosition::Standing || final_destination == CameraPosition::Driver || final_destination == CameraPosition::Passenger);

            // Step 1: Get to SofaSit1 if not already there.
            if (current_pos == CameraPosition::SofaLie)
            {
                QueueMove(CameraPosition::SofaSit1);
            }
            else if (current_pos == CameraPosition::SofaSit2)
            {
                QueueMove(CameraPosition::SofaSit1);
            }

            // Step 2: Stand up if the destination is off the sofa.
            if (needs_to_stand)
            {
                QueueMove(CameraPosition::Standing);
            }

            // Step 3: Add final destination if it's not an intermediate step.
            if (g_pending_moves.empty() || g_pending_moves.back() != final_destination)
            {
                QueueMove(final_destination);
            }
        }
        // -- Paths originating from the Seats (to non-seat destinations) --
        else if (current_pos == CameraPosition::Driver || current_pos == CameraPosition::Passenger)
        {
            // To go anywhere ELSE from a seat, we must stand up first.
            QueueMove(CameraPosition::Standing);

            if (final_destination != CameraPosition::Standing)
            {
                QueueMove(final_destination);
            }
        }
        // -- All other direct paths --
        else
        {
            QueueMove(final_destination);
        }

        // --- Start the sequence ---
        if (HasPendingMoves())
        {
            CameraPosition next_move = g_pending_moves.front();
            g_pending_moves.pop();
            MoveTo(next_move);
        }
    }

    void QueueMove(CameraPosition target)
    {
        g_pending_moves.push(target);
    }

    void ClearPendingMoves()
    {
        while (!g_pending_moves.empty())
        {
            g_pending_moves.pop();
        }
    }

    bool HasPendingMoves()
    {
        return !g_pending_moves.empty();
    }

    void RegisterSequence(
        CameraPosition from,
        CameraPosition to,
        std::function<std::unique_ptr<Animation::AnimationSequence>(const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state)> factory
    )
    {
        g_sequence_factory[{from, to}] = factory;
    }

    float GetTargetZForPosition(CameraPosition pos)
    {
        switch (pos)
        {
            case CameraPosition::Driver:
                return g_cached_driver_state.position.z;
            case CameraPosition::Passenger:
                return g_anim_ctx->settings.positions.passenger_seat.position.z;
            case CameraPosition::SofaSit1:
                return g_anim_ctx->settings.positions.sofa_sit1.position.z;
            case CameraPosition::SofaLie:
                return g_anim_ctx->settings.positions.sofa_lie.position.z;
            case CameraPosition::SofaSit2:
                return g_anim_ctx->settings.positions.sofa_sit2.position.z;
            default:
                return 0.0f; // Default for other positions, or error handling
        }
    }

} // namespace SPF_CabinWalk::AnimationController