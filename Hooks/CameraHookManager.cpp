#define _USE_MATH_DEFINES // For M_PI on MSVC
#include <cmath>          // For M_PI
#include "Hooks/CameraHookManager.hpp"
#include "Hooks/Offsets.hpp"
#include "Animation/Positions/CameraPositions.hpp" // For accessing predefined camera positions
#include "SPF_CabinWalk.hpp"                       // For access to g_ctx for logging and passenger seat position.
#include "Animation/AnimationConfig.hpp"

namespace SPF_CabinWalk::CameraHookManager
{
    using namespace Animation::Config;
    // =================================================================================================
    // Internal State
    // =================================================================================================

    // --- Hook Definitions ---
    using UpdateCameraFromInput_t = void (*)(long long camera_object, float delta_time);
    static UpdateCameraFromInput_t o_UpdateCameraFromInput = nullptr;

    // Define the function pointer type for CacheExteriorSoundAngleRange
    using CacheExteriorSoundAngleRange_t = void (*)(long long camera_object);

    // --- State Variables ---
    static AnimationController::CameraPosition g_current_camera_pos = AnimationController::CameraPosition::Driver;
    static AnimationController::CameraPosition g_previous_camera_pos = AnimationController::CameraPosition::Driver;

    // --- Backup Storage ---
    static AzimuthBackup g_original_azimuth_values[20];
    static uint32_t g_azimuth_backup_count = 0;
    static SPF_FVector g_original_camera_pivot = {0};
    static float g_original_mouse_left_limit = 0.0f;
    static float g_original_mouse_right_limit = 0.0f;
    static float g_original_mouse_up_limit = 0.0f;
    static float g_original_mouse_down_limit = 0.0f;

    // =================================================================================================
    // Forward Declarations for Internal Functions
    // =================================================================================================

    static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time);
    static void BackupAndModifyAzimuths(long long camera_object);
    static void RestoreAzimuths(long long camera_object);
    static void ZeroAzimuths(long long camera_object);

    // =================================================================================================
    // Public Functions
    // =================================================================================================

    bool Initialize(const SPF_Hooks_API *hooks_api, const char *plugin_name)
    {
        if (!hooks_api || !plugin_name)
        {
            return false;
        }

        return hooks_api->Register(
            plugin_name,
            "CabinWalk_UpdateCameraFromInput_Hook",
            "Cabin Walk Camera Update Hook",
            (void *)&Detour_UpdateCameraFromInput,
            (void **)&o_UpdateCameraFromInput,
            Offsets::G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE,
            true);
    }

    void SetCurrentCameraPosition(AnimationController::CameraPosition new_pos)
    {
        g_current_camera_pos = new_pos;
    }

    // =================================================================================================
    // Internal Hook Implementation
    // =================================================================================================

    static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time)
    {
        // If the logical camera position has not changed, just run the original function and 360-wrap logic.
        if (g_current_camera_pos == g_previous_camera_pos)
        {
            if (o_UpdateCameraFromInput) {
                o_UpdateCameraFromInput(camera_object, delta_time);
            }
        }
        else // A position change has occurred
        {
            // First, always restore to a clean slate if the previous state was not the default driver state.
            // This prevents modifications from stacking (e.g., passenger inverted limits + zeroed azimuths).
            if (g_previous_camera_pos != AnimationController::CameraPosition::Driver)
            {
                RestoreAzimuths(camera_object);
            }
            
            // Now, apply the new state's modifications from the clean, default state.
            switch (g_current_camera_pos)
            {
                case AnimationController::CameraPosition::Passenger:
                    BackupAndModifyAzimuths(camera_object);
                    break;

                case AnimationController::CameraPosition::Standing:
                case AnimationController::CameraPosition::SofaSit1:
                case AnimationController::CameraPosition::SofaLie:
                case AnimationController::CameraPosition::SofaSit2:
                    ZeroAzimuths(camera_object); // The switch inside this handles different limits
                    break;

                case AnimationController::CameraPosition::Driver:
                default:
                    // Do nothing, we are already restored to default.
                    break;
            }

            // Update the previous position for the next frame's comparison.
            g_previous_camera_pos = g_current_camera_pos;

            // Call the original function after our modifications.
            if (o_UpdateCameraFromInput)
            {
                o_UpdateCameraFromInput(camera_object, delta_time);
            }
        }

        // Handle 360-degree rotation wrapping for any free-look position
        if ((g_current_camera_pos == AnimationController::CameraPosition::Standing ||
             g_current_camera_pos == AnimationController::CameraPosition::SofaSit1 ||
             g_current_camera_pos == AnimationController::CameraPosition::SofaLie ||
             g_current_camera_pos == AnimationController::CameraPosition::SofaSit2) 
             && g_ctx.cameraAPI)
        {
            float yaw, pitch;
            g_ctx.cameraAPI->GetInteriorHeadRot(&yaw, &pitch);

            const float wrap_threshold = M_PI; // 180 degrees in radians
            const float wrap_value = 2 * M_PI; // 360 degrees in radians

            if (yaw > wrap_threshold)
            {
                g_ctx.cameraAPI->SetInteriorHeadRot(yaw - wrap_value, pitch);
            }
            else if (yaw < -wrap_threshold)
            {
                g_ctx.cameraAPI->SetInteriorHeadRot(yaw + wrap_value, pitch);
            }
        }
    }

    static void BackupAndModifyAzimuths(long long camera_object)
    {
        // 1. Handle Camera Pivot
        float *p_camera_pivot_x = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset);
        float *p_camera_pivot_y = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset + 4);
        float *p_camera_pivot_z = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset + 8);

        g_original_camera_pivot.x = *p_camera_pivot_x;
        g_original_camera_pivot.y = *p_camera_pivot_y;
        g_original_camera_pivot.z = *p_camera_pivot_z;

        *p_camera_pivot_x = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.x;
        *p_camera_pivot_y = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.y;
        *p_camera_pivot_z = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.z;

        // 2. Handle Mouse Limits via API
        if (g_ctx.cameraAPI)
        {
            g_ctx.cameraAPI->GetInteriorRotationLimits(
                &g_original_mouse_left_limit,
                &g_original_mouse_right_limit,
                &g_original_mouse_up_limit,
                &g_original_mouse_down_limit);

            float new_left_limit = g_original_mouse_right_limit * -1.0f;
            float new_right_limit = g_original_mouse_left_limit * -1.0f;

            g_ctx.cameraAPI->SetInteriorRotationLimits(
                new_left_limit,
                new_right_limit,
                g_original_mouse_up_limit,
                g_original_mouse_down_limit);
        }

        // 3. Handle Azimuth Ranges
        long long **azimuth_array_ptr = (long long **)((char *)camera_object + Offsets::g_offsets.azimuth_array_offset);
        long long azimuth_count = *(long long *)((char *)camera_object + Offsets::g_offsets.azimuth_count_offset);
        g_azimuth_backup_count = (azimuth_count < 20) ? (uint32_t)azimuth_count : 20;

        for (uint32_t i = 0; i < g_azimuth_backup_count; ++i)
        {
            long long azimuth_struct_ptr = (*azimuth_array_ptr)[i];
            if (azimuth_struct_ptr)
            {
                // --- Backup original values FIRST ---
                float *p_start_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_azimuth_offset);
                float *p_end_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_azimuth_offset);
                g_original_azimuth_values[i].start = *p_start_azimuth;
                g_original_azimuth_values[i].end = *p_end_azimuth;

                float *p_start_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_head_offset_x_offset);
                float *p_end_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_head_offset_x_offset);
                g_original_azimuth_values[i].start_head_offset = {p_start_offset_vec[0], p_start_offset_vec[1], p_start_offset_vec[2]};
                g_original_azimuth_values[i].end_head_offset = {p_end_offset_vec[0], p_end_offset_vec[1], p_end_offset_vec[2]};

                // --- Now, perform modifications ---
                // 1. Invert and check if angles need to be swapped
                *p_start_azimuth *= -1.0f;
                *p_end_azimuth *= -1.0f;
                bool angles_were_swapped = false;
                if (*p_start_azimuth > *p_end_azimuth)
                {
                    float temp = *p_start_azimuth;
                    *p_start_azimuth = *p_end_azimuth;
                    *p_end_azimuth = temp;
                    angles_were_swapped = true;
                }

                // 2. Conditionally swap head offsets based on angle swap
                if (angles_were_swapped)
                {
                    // SWAP head offsets
                    p_start_offset_vec[0] = g_original_azimuth_values[i].end_head_offset.x * -1.0f;
                    p_start_offset_vec[1] = g_original_azimuth_values[i].end_head_offset.y;
                    p_start_offset_vec[2] = g_original_azimuth_values[i].end_head_offset.z;

                    p_end_offset_vec[0] = g_original_azimuth_values[i].start_head_offset.x * -1.0f;
                    p_end_offset_vec[1] = g_original_azimuth_values[i].start_head_offset.y;
                    p_end_offset_vec[2] = g_original_azimuth_values[i].start_head_offset.z;
                }
                else
                {
                    // DO NOT SWAP head offsets
                    p_start_offset_vec[0] = g_original_azimuth_values[i].start_head_offset.x * -1.0f;
                    p_start_offset_vec[1] = g_original_azimuth_values[i].start_head_offset.y;
                    p_start_offset_vec[2] = g_original_azimuth_values[i].start_head_offset.z;

                    p_end_offset_vec[0] = g_original_azimuth_values[i].end_head_offset.x * -1.0f;
                    p_end_offset_vec[1] = g_original_azimuth_values[i].end_head_offset.y;
                    p_end_offset_vec[2] = g_original_azimuth_values[i].end_head_offset.z;
                }

                // --- Logging ---
                if (g_ctx.loggerHandle && g_ctx.formattingAPI)
                {
                    char log_buffer[1024];
                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Azimuth[%d] Angles: [%.2f, %.2f] -> [%.2f, %.2f]. Swapped: %s.",
                                                i,
                                                g_original_azimuth_values[i].start, g_original_azimuth_values[i].end,
                                                *p_start_azimuth, *p_end_azimuth,
                                                angles_were_swapped ? "YES" : "NO");
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Azimuth[%d] Start Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                                                i,
                                                g_original_azimuth_values[i].start_head_offset.x, g_original_azimuth_values[i].start_head_offset.y, g_original_azimuth_values[i].start_head_offset.z,
                                                p_start_offset_vec[0], p_start_offset_vec[1], p_start_offset_vec[2]);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Azimuth[%d] End Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                                                i,
                                                g_original_azimuth_values[i].end_head_offset.x, g_original_azimuth_values[i].end_head_offset.y, g_original_azimuth_values[i].end_head_offset.z,
                                                p_end_offset_vec[0], p_end_offset_vec[1], p_end_offset_vec[2]);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);
                }
            }
        }

        // 4. Recalculate outside sound cache
        if (Offsets::g_offsets.pfnCacheExteriorSoundAngleRange)
        {
            CacheExteriorSoundAngleRange_t pfnCache = (CacheExteriorSoundAngleRange_t)Offsets::g_offsets.pfnCacheExteriorSoundAngleRange;
            pfnCache(camera_object);
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, "[CabinWalk] Called CacheExteriorSoundAngleRange after modifying azimuths.");
            }
        }
    }

    static void RestoreAzimuths(long long camera_object)
    {
        // 1. Restore Camera Pivot
        float *p_camera_pivot_x = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset);
        float *p_camera_pivot_y = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset + 4);
        float *p_camera_pivot_z = (float *)((char *)camera_object + Offsets::g_offsets.camera_pivot_offset + 8);
        *p_camera_pivot_x = g_original_camera_pivot.x;
        *p_camera_pivot_y = g_original_camera_pivot.y;
        *p_camera_pivot_z = g_original_camera_pivot.z;

        // 2. Restore Mouse Limits via API
        if (g_ctx.cameraAPI)
        {
            g_ctx.cameraAPI->SetInteriorRotationLimits(
                g_original_mouse_left_limit,
                g_original_mouse_right_limit,
                g_original_mouse_up_limit,
                g_original_mouse_down_limit);
        }

        // 3. Restore Azimuth Ranges
        long long **azimuth_array_ptr = (long long **)((char *)camera_object + Offsets::g_offsets.azimuth_array_offset);
        for (uint32_t i = 0; i < g_azimuth_backup_count; ++i)
        {
            long long azimuth_struct_ptr = (*azimuth_array_ptr)[i];
            if (azimuth_struct_ptr)
            {
                // Restore angles
                float *p_start_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_azimuth_offset);
                float *p_end_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_azimuth_offset);
                *p_start_azimuth = g_original_azimuth_values[i].start;
                *p_end_azimuth = g_original_azimuth_values[i].end;

                // Restore full head offset vectors
                float *p_start_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_head_offset_x_offset);
                p_start_offset_vec[0] = g_original_azimuth_values[i].start_head_offset.x;
                p_start_offset_vec[1] = g_original_azimuth_values[i].start_head_offset.y;
                p_start_offset_vec[2] = g_original_azimuth_values[i].start_head_offset.z;

                float *p_end_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_head_offset_x_offset);
                p_end_offset_vec[0] = g_original_azimuth_values[i].end_head_offset.x;
                p_end_offset_vec[1] = g_original_azimuth_values[i].end_head_offset.y;
                p_end_offset_vec[2] = g_original_azimuth_values[i].end_head_offset.z;
            }
        }

        // 4. Recalculate outside sound cache
        if (Offsets::g_offsets.pfnCacheExteriorSoundAngleRange)
        {
            CacheExteriorSoundAngleRange_t pfnCache = (CacheExteriorSoundAngleRange_t)Offsets::g_offsets.pfnCacheExteriorSoundAngleRange;
            pfnCache(camera_object);
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, "[CabinWalk] Called CacheExteriorSoundAngleRange after restoring azimuths.");
            }
        }
    }

    static void ZeroAzimuths(long long camera_object)
    {
        // 1. Handle Base Head Offset
        // No change needed for base head offset as animation controller directly sets position

        // 2. Handle Mouse Limits via API
        if (g_ctx.cameraAPI)
        {
            g_ctx.cameraAPI->GetInteriorRotationLimits(
                &g_original_mouse_left_limit,
                &g_original_mouse_right_limit,
                &g_original_mouse_up_limit,
                &g_original_mouse_down_limit);

            switch (g_current_camera_pos)
            {
                case AnimationController::CameraPosition::Standing:
                    // Set wide limits for free look in standing mode
                    g_ctx.cameraAPI->SetInteriorRotationLimits(231.0f, -231.0f, g_original_mouse_up_limit, -80.0f);
                    break;
                
                case AnimationController::CameraPosition::SofaSit1:
                case AnimationController::CameraPosition::SofaLie:
                case AnimationController::CameraPosition::SofaSit2:
                    // Set custom limits for sofa positions
                    g_ctx.cameraAPI->SetInteriorRotationLimits(
                        Sofa::Limits::YAW_LEFT,
                        Sofa::Limits::YAW_RIGHT,
                        Sofa::Limits::PITCH_UP,
                        Sofa::Limits::PITCH_DOWN
                    );
                    break;
                
                default:
                    // Fallback, should not happen if logic is correct
                    break;
            }
        }

        // 3. Handle Azimuth Ranges - Zero them out
        long long **azimuth_array_ptr = (long long **)((char *)camera_object + Offsets::g_offsets.azimuth_array_offset);
        long long azimuth_count = *(long long *)((char *)camera_object + Offsets::g_offsets.azimuth_count_offset);
        g_azimuth_backup_count = (azimuth_count < 20) ? (uint32_t)azimuth_count : 20;

        for (uint32_t i = 0; i < g_azimuth_backup_count; ++i)
        {
            long long azimuth_struct_ptr = (*azimuth_array_ptr)[i];
            if (azimuth_struct_ptr)
            {
                // Backup original values before zeroing
                float *p_start_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_azimuth_offset);
                float *p_end_azimuth = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_azimuth_offset);
                g_original_azimuth_values[i].start = *p_start_azimuth;
                g_original_azimuth_values[i].end = *p_end_azimuth;

                float *p_start_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.start_head_offset_x_offset);
                float *p_end_offset_vec = (float *)((char *)azimuth_struct_ptr + Offsets::g_offsets.end_head_offset_x_offset);
                g_original_azimuth_values[i].start_head_offset = {p_start_offset_vec[0], p_start_offset_vec[1], p_start_offset_vec[2]};
                g_original_azimuth_values[i].end_head_offset = {p_end_offset_vec[0], p_end_offset_vec[1], p_end_offset_vec[2]};

                // Zero out the azimuths and head offsets
                *p_start_azimuth = 0.0f;
                *p_end_azimuth = 0.0f;

                float *p_start_offset_vec_current = p_start_offset_vec;
                float *p_end_offset_vec_current = p_end_offset_vec;

                p_start_offset_vec[0] = 0.0f;
                p_start_offset_vec[1] = 0.0f;
                p_start_offset_vec[2] = 0.0f;
                p_end_offset_vec[0] = 0.0f;
                p_end_offset_vec[1] = 0.0f;
                p_end_offset_vec[2] = 0.0f;

                // --- Logging ---
                if (g_ctx.loggerHandle && g_ctx.formattingAPI)
                {
                    char log_buffer[1024];
                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Zeroing Azimuth[%d] Angles: [%.2f, %.2f] -> [%.2f, %.2f].",
                                                i,
                                                g_original_azimuth_values[i].start, g_original_azimuth_values[i].end,
                                                *p_start_azimuth, *p_end_azimuth);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Zeroing Azimuth[%d] Start Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                                                i,
                                                g_original_azimuth_values[i].start_head_offset.x, g_original_azimuth_values[i].start_head_offset.y, g_original_azimuth_values[i].start_head_offset.z,
                                                p_start_offset_vec_current[0], p_start_offset_vec_current[1], p_start_offset_vec_current[2]);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                                                "[CabinWalk] Zeroing Azimuth[%d] End Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                                                i,
                                                g_original_azimuth_values[i].end_head_offset.x, g_original_azimuth_values[i].end_head_offset.y, g_original_azimuth_values[i].end_head_offset.z,
                                                p_end_offset_vec_current[0], p_end_offset_vec_current[1], p_end_offset_vec_current[2]);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);
                }
            }
        }

        // 4. Recalculate outside sound cache
        if (Offsets::g_offsets.pfnCacheExteriorSoundAngleRange)
        {
            CacheExteriorSoundAngleRange_t pfnCache = (CacheExteriorSoundAngleRange_t)Offsets::g_offsets.pfnCacheExteriorSoundAngleRange;
            pfnCache(camera_object);
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, "[CabinWalk] Called CacheExteriorSoundAngleRange after zeroing azimuths.");
            }
        }
    }

} // namespace SPF_CabinWalk::CameraHookManager
