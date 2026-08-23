#include "Hooks/Offsets.hpp"

#include "SPF_CabinWalk.hpp"  // For g_ctx to log errors
#include "SPF_Hooks_API.h"
#include "SPF_Logger_API.h"

#include <cstdint>

namespace SPF_CabinWalk::Offsets {
// The single, global instance of the plugin's offsets.
Offsets g_offsets;

bool Find(const SPF_Hooks_API* hooks_api) {
  if (!hooks_api) {
    if (g_ctx.loggerHandle) {
      g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Hooks API is null, cannot find offsets.");
    }
    return false;
  }

  // Part A: Find the UpdateInteriorCamera function and camera pivot offset.
  uintptr_t interior_cam_fn_address = hooks_api->Hook_FindPattern(G_UPDATE_INTERIOR_CAMERA_SIGNATURE);
  if (!interior_cam_fn_address) {
    if (g_ctx.loggerHandle) {
      g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_UPDATE_INTERIOR_CAMERA_SIGNATURE.");
    }
    return false;
  }

  uintptr_t pattern_addr = hooks_api->Hook_FindPatternFrom(G_CAMERA_PIVOT_SIGNATURE, interior_cam_fn_address, 1024);
  if (!pattern_addr) {
    if (g_ctx.loggerHandle) {
      g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_CAMERA_PIVOT_SIGNATURE.");
    }
    return false;
  }

  g_offsets.camera_pivot_offset = *(uint32_t*)(pattern_addr + 4);

  // --- Find CacheExteriorSoundAngleRange function pointer ---
  g_offsets.pfnCacheExteriorSoundAngleRange = hooks_api->Hook_FindPattern(G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE);
  if (!g_offsets.pfnCacheExteriorSoundAngleRange) {
    if (g_ctx.loggerHandle) {
      g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "[Offsets] Could not find G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE.");
    }
    return false;
  }

  // All patterns found, now log the results.
  if (g_ctx.loggerHandle) {
    char log_buffer[512];
    g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "[Offsets] Offsets found dynamically. pivot: 0x%X, CacheExtSoundFn: 0x%llX", g_offsets.camera_pivot_offset, g_offsets.pfnCacheExteriorSoundAngleRange);
    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
  }

  return true;
}
}  // namespace SPF_CabinWalk::Offsets
