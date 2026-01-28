#include "Hooks/Offsets.hpp"
#include "SPF_CabinWalk.hpp" // For g_ctx to log errors

namespace SPF_CabinWalk::Offsets
{
    // The single, global instance of the plugin's offsets.
    Offsets g_offsets;

    bool Find(const SPF_Hooks_API* hooks_api)
    {
        if (!hooks_api)
        {
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Hooks API is null, cannot find offsets.");
            }
            return false;
        }

        // Part A: Find offsets in UpdateCameraFromInput
        uintptr_t update_cam_fn_address = hooks_api->Hook_FindPattern(G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE);
        if (!update_cam_fn_address)
        {
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE.");
            }
            return false;
        }

        uintptr_t array_pattern_address = hooks_api->Hook_FindPatternFrom(G_AZIMUTH_ARRAY_AND_COUNT_PATTERN, update_cam_fn_address, 2048);
        if (!array_pattern_address)
        {
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_AZIMUTH_ARRAY_AND_COUNT_PATTERN.");
            }
            return false;
        }
        
        g_offsets.azimuth_array_offset = *(uint32_t *)(array_pattern_address + 7);
        g_offsets.azimuth_count_offset = *(uint32_t *)(array_pattern_address + 14);

        // Part B: Find offsets in UpdateInteriorCamera using a chained search
        uintptr_t interior_cam_fn_address = hooks_api->Hook_FindPattern(G_UPDATE_INTERIOR_CAMERA_SIGNATURE);
        if (!interior_cam_fn_address)
        {
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_UPDATE_INTERIOR_CAMERA_SIGNATURE.");
            }
            return false;
        }

        uintptr_t last_pattern_addr = interior_cam_fn_address;

        // --- Find start_azimuth_offset (0x10) ---
        last_pattern_addr = hooks_api->Hook_FindPatternFrom(G_START_AZIMUTH_SIGNATURE, last_pattern_addr, 200);
        if (!last_pattern_addr) { g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_START_AZIMUTH_SIGNATURE."); return false; }
        g_offsets.start_azimuth_offset = *(uint8_t*)(last_pattern_addr + 7);

        // --- Find end_azimuth_offset (0x14) ---
        last_pattern_addr = hooks_api->Hook_FindPatternFrom(G_END_AZIMUTH_SIGNATURE, last_pattern_addr, 50);
        if (!last_pattern_addr) { g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_END_AZIMUTH_SIGNATURE."); return false; }
        g_offsets.end_azimuth_offset = *(uint8_t*)(last_pattern_addr + 7);

        // --- Find azimuth_outside_flag_offset (0x18) ---
        uintptr_t outside_flag_addr = hooks_api->Hook_FindPatternFrom(G_AZIMUTH_OUTSIDE_FLAG_SIGNATURE, interior_cam_fn_address, 200);
        if (!outside_flag_addr) { g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_AZIMUTH_OUTSIDE_FLAG_SIGNATURE."); return false; }
        g_offsets.azimuth_outside_flag_offset = *(uint8_t*)(outside_flag_addr + 10);

        // --- Find head_offsets (0x3C and 0x48) ---
        uintptr_t pattern_addr = hooks_api->Hook_FindPatternFrom(G_HEAD_OFFSETS_SIGNATURE, last_pattern_addr, 100);
        if (!pattern_addr) { g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_HEAD_OFFSETS_SIGNATURE."); return false; }
        g_offsets.start_head_offset_x_offset = *(uint8_t*)(pattern_addr + 11);
        g_offsets.end_head_offset_x_offset = *(uint8_t*)(pattern_addr + 28);

        // --- Find camera_pivot_offset (0x494) ---
        pattern_addr = hooks_api->Hook_FindPatternFrom(G_CAMERA_PIVOT_SIGNATURE, interior_cam_fn_address, 1024);
        if (!pattern_addr) { g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_BASE_HEAD_OFFSET_SIGNATURE."); return false; }
        g_offsets.camera_pivot_offset = *(uint32_t*)(pattern_addr + 4);

        // --- Find CacheExteriorSoundAngleRange function pointer ---
        g_offsets.pfnCacheExteriorSoundAngleRange = hooks_api->Hook_FindPattern(G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE);
        if (!g_offsets.pfnCacheExteriorSoundAngleRange)
        {
            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE.");
            }
            return false;
        }

        // All patterns found, now log the results.
        if (g_ctx.loggerHandle)
        {
            char log_buffer[512];
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), 
                "[Offsets] All offsets found dynamically. "
                "start_azimuth: 0x%X, end_azimuth: 0x%X, azimuth_outside_flag: 0x%X, "
                "azimuth_array: 0x%X, azimuth_count: 0x%X, "
                "start_head_x: 0x%X, end_head_x: 0x%X, "
                "pivot: 0x%X, "
                "CacheExtSoundFn: 0x%llX",
                g_offsets.start_azimuth_offset, g_offsets.end_azimuth_offset, g_offsets.azimuth_outside_flag_offset,
                g_offsets.azimuth_array_offset, g_offsets.azimuth_count_offset,
                g_offsets.start_head_offset_x_offset, g_offsets.end_head_offset_x_offset,
                g_offsets.camera_pivot_offset, 
                g_offsets.pfnCacheExteriorSoundAngleRange
            );
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }

        return true;
    }
}

