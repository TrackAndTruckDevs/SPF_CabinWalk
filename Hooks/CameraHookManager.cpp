#include "Hooks/CameraHookManager.hpp"
#include "Hooks/Offsets.hpp"
#include "Animation/Positions/CameraPositions.hpp" // For accessing predefined camera positions
#include "SPF_CabinWalk.hpp" // For access to g_ctx for logging and passenger seat position.

namespace SPF_CabinWalk::CameraHookManager
{
    // =================================================================================================
    // Internal State
    // =================================================================================================

    // --- Hook Definitions ---
    using UpdateCameraFromInput_t = void (*)(long long camera_object, float delta_time);
    static UpdateCameraFromInput_t o_UpdateCameraFromInput = nullptr;

    // Define the function pointer type for CacheExteriorSoundAngleRange
    using CacheExteriorSoundAngleRange_t = void(*)(long long camera_object);

    // --- State Variables ---
    static bool g_is_on_passenger_seat = false;
    static bool g_are_azimuth_values_modified = false;

    // --- Backup Storage ---
    static AzimuthBackup g_original_azimuth_values[20];
    static uint32_t g_azimuth_backup_count = 0;
    static SPF_FVector g_original_base_head_offset = {0};
    static float g_original_mouse_left_limit = 0.0f;
    static float g_original_mouse_right_limit = 0.0f;

    // =================================================================================================
    // Forward Declarations for Internal Functions
    // =================================================================================================

    static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time);
    static void BackupAndModifyAzimuths(long long camera_object);
    static void RestoreAzimuths(long long camera_object);

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

    void SetPassengerSeatState(bool is_on_passenger_seat)
    {
        g_is_on_passenger_seat = is_on_passenger_seat;
    }

    // =================================================================================================
    // Internal Hook Implementation
    // =================================================================================================

    static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time)
    {
        if (g_is_on_passenger_seat && !g_are_azimuth_values_modified)
        {
            BackupAndModifyAzimuths(camera_object);
            g_are_azimuth_values_modified = true;
        }
        else if (!g_is_on_passenger_seat && g_are_azimuth_values_modified)
        {
            RestoreAzimuths(camera_object);
            g_are_azimuth_values_modified = false;
        }

        if (o_UpdateCameraFromInput)
        {
            o_UpdateCameraFromInput(camera_object, delta_time);
        }
    }

    static void BackupAndModifyAzimuths(long long camera_object)
    {
        // 1. Handle Base Head Offset
        float *p_base_head_offset_x = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset);
        float *p_base_head_offset_y = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset + 4);
        float *p_base_head_offset_z = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset + 8);

        g_original_base_head_offset.x = *p_base_head_offset_x;
        g_original_base_head_offset.y = *p_base_head_offset_y;
        g_original_base_head_offset.z = *p_base_head_offset_z;

        *p_base_head_offset_x = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.x;
        *p_base_head_offset_y = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.y;
        *p_base_head_offset_z = Animation::CameraPositions::PASSENGER_SEAT_TARGET.position.z;

        // 2. Handle Mouse Limits
        float *p_mouse_left_limit = (float *)((char *)camera_object + Offsets::g_offsets.mouse_left_limit_offset);
        float *p_mouse_right_limit = (float *)((char *)camera_object + Offsets::g_offsets.mouse_right_limit_offset);

        g_original_mouse_left_limit = *p_mouse_left_limit;
        g_original_mouse_right_limit = *p_mouse_right_limit;

        float new_left_limit = g_original_mouse_right_limit * -1.0f;
        float new_right_limit = g_original_mouse_left_limit * -1.0f;
        *p_mouse_left_limit = new_left_limit;
        *p_mouse_right_limit = new_right_limit;

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
                g_original_azimuth_values[i].start_head_offset = { p_start_offset_vec[0], p_start_offset_vec[1], p_start_offset_vec[2] };
                g_original_azimuth_values[i].end_head_offset = { p_end_offset_vec[0], p_end_offset_vec[1], p_end_offset_vec[2] };

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
                        angles_were_swapped ? "YES" : "NO"
                    );
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                        "[CabinWalk] Azimuth[%d] Start Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                        i,
                        g_original_azimuth_values[i].start_head_offset.x, g_original_azimuth_values[i].start_head_offset.y, g_original_azimuth_values[i].start_head_offset.z,
                        p_start_offset_vec[0], p_start_offset_vec[1], p_start_offset_vec[2]
                    );
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_DEBUG, log_buffer);

                    g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer),
                        "[CabinWalk] Azimuth[%d] End Offset: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)",
                        i,
                        g_original_azimuth_values[i].end_head_offset.x, g_original_azimuth_values[i].end_head_offset.y, g_original_azimuth_values[i].end_head_offset.z,
                        p_end_offset_vec[0], p_end_offset_vec[1], p_end_offset_vec[2]
                    );
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
        // 1. Restore Base Head Offset
        float *p_base_head_offset_x = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset);
        float *p_base_head_offset_y = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset + 4);
        float *p_base_head_offset_z = (float *)((char *)camera_object + Offsets::g_offsets.base_head_offset + 8);
        *p_base_head_offset_x = g_original_base_head_offset.x;
        *p_base_head_offset_y = g_original_base_head_offset.y;
        *p_base_head_offset_z = g_original_base_head_offset.z;

        // 2. Restore Mouse Limits
        float *p_mouse_left_limit = (float *)((char *)camera_object + Offsets::g_offsets.mouse_left_limit_offset);
        float *p_mouse_right_limit = (float *)((char *)camera_object + Offsets::g_offsets.mouse_right_limit_offset);
        *p_mouse_left_limit = g_original_mouse_left_limit;
        *p_mouse_right_limit = g_original_mouse_right_limit;

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

} // namespace SPF_CabinWalk::CameraHookManager
