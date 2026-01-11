#include "Animation/AnimationController.hpp"
#include "SPF_CabinWalk.hpp"
#include "Hooks/CameraHookManager.hpp"
#include "Utils/Utils.hpp" // Placeholder for potential utility functions
#include "Animation/StandingAnimController.hpp" // Integrate the new controller
#include "Animation/AnimationConfig.hpp"      // For STEP_AMOUNT and other constants

#include <utility> // For std::pair
#include <map>
#include <functional>
#include <memory>

#include "Sequences/DriverToPassenger.hpp"
#include "Sequences/PassengerToDriver.hpp" // Include the actual sequence factory
#include "Sequences/DriverToStanding.hpp"
#include "Sequences/StandingToDriver.hpp"
#include "Sequences/PassengerToStanding.hpp"
#include "Sequences/StandingToPassenger.hpp"

namespace SPF_CabinWalk::AnimationController
{
    using namespace Animation::Config;
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
    // Stores the next move in a chained animation (e.g., Stand Up -> Sit Down)
    static CameraPosition g_pending_move = CameraPosition::None;

    // =================================================================================================
    // Public Functions
    // =================================================================================================

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


    }
    void Update()
    {
        if (!g_anim_ctx || !g_anim_ctx->coreAPI)
        {
            return;
        }

        // --- Handle pending chained animation ---
        if (g_pending_move != CameraPosition::None)
        {
            // Check if the standing controller is now in the default standing stance and not busy
            if (StandingAnimController::GetCurrentStance() == StandingAnimController::Stance::Standing && !StandingAnimController::IsAnimating())
            {
                CameraPosition next_move = g_pending_move;
                g_pending_move = CameraPosition::None;
                MoveTo(next_move); // Trigger the originally requested move
                return; // The new MoveTo call will handle the rest
            }
        }

        // --- 1. Handle Major Transitions ---
        if (g_active_sequence && g_active_sequence->IsPlaying())
        {
            SPF_Timestamps timestamps;
            g_anim_ctx->coreAPI->telemetry->GetTimestamps(g_anim_ctx->telemetryHandle, &timestamps);
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
            }
        }
        // --- 2. Handle Standing "Sub-State" Animations ---
        else if (g_current_pos == CameraPosition::Standing)
        {
            Animation::CurrentCameraState current_state;
            g_anim_ctx->cameraAPI->GetInteriorSeatPos(&current_state.position.x, &current_state.position.y, &current_state.position.z);
            g_anim_ctx->cameraAPI->GetInteriorHeadRot(&current_state.rotation.x, &current_state.rotation.y);
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
        if (g_current_pos == CameraPosition::Standing && (target == CameraPosition::Driver || target == CameraPosition::Passenger))
        {
            StandingAnimController::Stance current_stance = StandingAnimController::GetCurrentStance();

            if (current_stance != StandingAnimController::Stance::Standing)
            {
                // Not in the base standing stance, so we need to transition to it first.
                g_pending_move = target; // Set the final destination

                if (current_stance == StandingAnimController::Stance::Crouching)
                {
                    StandingAnimController::TriggerStandUp();
                }
                else if (current_stance == StandingAnimController::Stance::Tiptoes)
                {
                    StandingAnimController::TriggerStandDown();
                }
                // InTransition and ReturningToHome are busy states, IsAnimating() check should have caught them.
                return; // Exit, the Update loop will handle the pending move later.
            }
            
            // If we get here, stance is Standing, so check if we need to walk back.
            bool can_sit_immediately = StandingAnimController::CanSitDown(target);
            if (!can_sit_immediately)
            {
                // StandingAnimController has initiated the walk-back, so we return.
                // It will eventually call MoveTo again when ready to sit.
                return;
            }
            // If can_sit_immediately, fall through to the normal transition logic below.
        }

        // --- NORMAL TRANSITION LOGIC ---
        // Cache current camera state before starting animation
        Animation::CurrentCameraState initial_state;
        g_anim_ctx->cameraAPI->GetInteriorSeatPos(&initial_state.position.x, &initial_state.position.y, &initial_state.position.z);
        g_anim_ctx->cameraAPI->GetInteriorHeadRot(&initial_state.rotation.x, &initial_state.rotation.y); // .x=yaw, .y=pitch
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
            if (target == CameraPosition::Passenger)
            {
                animation_target_state.position = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position;
                animation_target_state.rotation = Animation::CameraPositions::PASSENGER_SEAT_TARGET.rotation;
                // Roll is currently not handled by CameraPositions or GetInteriorHeadRot, default to initial.
                animation_target_state.rotation.z = initial_state.rotation.z;
            }
            else if (target == CameraPosition::Driver)
            {
                // For returning to driver, the target is the cached driver's state
                animation_target_state = g_cached_driver_state;
            }
            else if (target == CameraPosition::Standing)
            {
                animation_target_state.position = Animation::CameraPositions::STANDING_POSITION_TARGET.position;
                animation_target_state.rotation = Animation::CameraPositions::STANDING_POSITION_TARGET.rotation;
                animation_target_state.rotation.z = initial_state.rotation.z;
            }
            // TODO: Handle other CameraPosition targets here

            // Found a factory, create the sequence and start it
            SPF_Timestamps timestamps;
            g_anim_ctx->coreAPI->telemetry->GetTimestamps(g_anim_ctx->telemetryHandle, &timestamps);
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

            // Direct snap to target
            if (target == CameraPosition::Passenger)
            {
                g_anim_ctx->cameraAPI->SetInteriorSeatPos(Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.x,
                                                           Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.y,
                                                           Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.z);
                g_anim_ctx->cameraAPI->SetInteriorHeadRot(Animation::CameraPositions::PASSENGER_SEAT_TARGET.rotation.x,
                                                           Animation::CameraPositions::PASSENGER_SEAT_TARGET.rotation.y);
            }
            else if (target == CameraPosition::Driver)
            {
                // Snap back to the cached driver position if available
                if (g_cached_driver_state.position.x != 0.0f || g_cached_driver_state.position.y != 0.0f || g_cached_driver_state.position.z != 0.0f ||
                    g_cached_driver_state.rotation.x != 0.0f || g_cached_driver_state.rotation.y != 0.0f)
                {
                    g_anim_ctx->cameraAPI->SetInteriorSeatPos(g_cached_driver_state.position.x, g_cached_driver_state.position.y, g_cached_driver_state.position.z);
                    g_anim_ctx->cameraAPI->SetInteriorHeadRot(g_cached_driver_state.rotation.x, g_cached_driver_state.rotation.y);
                }
                // else, snap to 0,0,0 and 0,0 if cached state is default or invalid.
                else
                {
                     g_anim_ctx->cameraAPI->SetInteriorSeatPos(0.0f, 0.0f, 0.0f);
                     g_anim_ctx->cameraAPI->SetInteriorHeadRot(0.0f, 0.0f);
                }
            }
            else if (target == CameraPosition::Standing)
            {
                g_anim_ctx->cameraAPI->SetInteriorSeatPos(Animation::CameraPositions::STANDING_POSITION_TARGET.position.x,
                                                           Animation::CameraPositions::STANDING_POSITION_TARGET.position.y,
                                                           Animation::CameraPositions::STANDING_POSITION_TARGET.position.z);
                g_anim_ctx->cameraAPI->SetInteriorHeadRot(Animation::CameraPositions::STANDING_POSITION_TARGET.rotation.x,
                                                           Animation::CameraPositions::STANDING_POSITION_TARGET.rotation.y);
                StandingAnimController::OnEnterStandingState(); // Reset stance on snap
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

    void RegisterSequence(
        CameraPosition from,
        CameraPosition to,
        std::function<std::unique_ptr<Animation::AnimationSequence>(const Animation::CurrentCameraState& start_state, const Animation::CurrentCameraState& target_state)> factory
    )
    {
        g_sequence_factory[{from, to}] = factory;
    }

} // namespace SPF_CabinWalk::AnimationController