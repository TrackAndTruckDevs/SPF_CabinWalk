#include "Animation/AnimationController.hpp"
#include "SPF_CabinWalk.hpp"
#include "Hooks/CameraHookManager.hpp"
#include "Utils/Utils.hpp" // Placeholder for potential utility functions

#include <utility> // For std::pair
#include <map>
#include <functional>
#include <memory>

#include "Sequences/DriverToPassenger.hpp"
#include "Sequences/PassengerToDriver.hpp" // Include the actual sequence factory

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

    // =================================================================================================
    // Public Functions
    // =================================================================================================

    void Initialize(PluginContext *ctx)
    {
        g_anim_ctx = ctx;
        g_current_pos = CameraPosition::Driver; // Always start at driver's seat by default

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

    }
    void Update()
    {
        if (!g_active_sequence || !g_active_sequence->IsPlaying() || !g_anim_ctx || !g_anim_ctx->coreAPI)
        {
            return;
        }

        SPF_Timestamps timestamps;
        g_anim_ctx->coreAPI->telemetry->GetTimestamps(g_anim_ctx->telemetryHandle, &timestamps);
        uint64_t delta_time_ms = timestamps.simulation - last_simulation_time;
        last_simulation_time = timestamps.simulation;

        bool is_playing = g_active_sequence->Update(delta_time_ms, g_anim_ctx->cameraAPI);

        if (!is_playing)
        {
            // Animation finished
            g_active_sequence.reset(); // Clear the sequence
            g_current_pos = g_target_pos; // Update current position

            // Notify hook manager of the new state
            if (g_current_pos == CameraPosition::Passenger)
            {
                CameraHookManager::SetPassengerSeatState(true);
            }
            else
            {
                CameraHookManager::SetPassengerSeatState(false);
            }
        }
    }

    void MoveTo(CameraPosition target)
    {
        if (g_active_sequence && g_active_sequence->IsPlaying())
        {
            return; // Animation already in progress
        }

        if (target == g_current_pos)
        {
            return; // Already at target position
        }

        if (!g_anim_ctx || !g_anim_ctx->cameraAPI)
        {
            return; // API not ready
        }

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
                CameraHookManager::SetPassengerSeatState(true);
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
                CameraHookManager::SetPassengerSeatState(false);
            }
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