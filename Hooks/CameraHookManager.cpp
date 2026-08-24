#include "Hooks/CameraHookManager.hpp"

#include "Animation/AnimationController.hpp"
#include "Animation/StandingAnimController.hpp"
#include "Hooks/Offsets.hpp"
#include "SPF_CabinWalk.hpp"
#include "SPF_Hooks_API.h"
#include "SPF_Logger_API.h"
#include "SPF_TelemetryData.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

namespace SPF_CabinWalk::CameraHookManager {
// =================================================================================================
// Internal State
// =================================================================================================

using UpdateCameraFromInput_t = void (*)(long long camera_object, float delta_time);
using CacheExteriorSoundAngleRange_t = void (*)(long long camera_object);

static UpdateCameraFromInput_t o_UpdateCameraFromInput = nullptr;
static bool g_hook_registered = false;

static AnimationController::CameraPosition g_current_camera_pos = AnimationController::CameraPosition::Driver;
static AnimationController::CameraPosition g_previous_camera_pos = AnimationController::CameraPosition::Driver;

static AzimuthBackup g_original_azimuth_values[20];
static uint32_t g_azimuth_backup_count = 0;
static bool g_driver_pivot_saved = false;
static SPF_FVector g_driver_seat_pivot = {0};
static float g_original_mouse_left_limit = 0.0f;
static float g_original_mouse_right_limit = 0.0f;
static float g_original_mouse_up_limit = 0.0f;
static float g_original_mouse_down_limit = 0.0f;

static long long g_last_camera_object = 0;

// =================================================================================================
// Forward Declarations
// =================================================================================================

struct CameraPivot {
  float x, y, z;
};

static CameraPivot* GetPivot(long long camera_object) {
  return reinterpret_cast<CameraPivot*>(reinterpret_cast<char*>(camera_object) + Offsets::g_offsets.camera_pivot_offset);
}

static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time);
static void BackupAndModifyAzimuths(long long camera_object);
static void RestoreAzimuths(long long camera_object);
static void ZeroAzimuths(long long camera_object);

static bool CacheExteriorSound(long long camera_object) {
  if (!Offsets::g_offsets.pfnCacheExteriorSoundAngleRange) {
    return false;
  }

  auto pfn_cache = reinterpret_cast<CacheExteriorSoundAngleRange_t>(Offsets::g_offsets.pfnCacheExteriorSoundAngleRange);
  pfn_cache(camera_object);
  return true;
}

// =================================================================================================
// Public Functions
// =================================================================================================

bool Initialize(const SPF_Hooks_API* hooks_api, const char* plugin_name) {
  if (!hooks_api || !plugin_name) {
    return false;
  }

  if (g_hook_registered) {
    return true;
  }

  bool result =
    hooks_api->Hook_Register(plugin_name, "CabinWalk_UpdateCameraFromInput_Hook", "Cabin Walk Camera Update Hook", (void*)&Detour_UpdateCameraFromInput, (void**)&o_UpdateCameraFromInput, Offsets::G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE, true);

  if (result) {
    g_hook_registered = true;
  }

  return result;
}

void SetCurrentCameraPosition(AnimationController::CameraPosition new_pos) { g_current_camera_pos = new_pos; }

// =================================================================================================
// Internal Hook Implementation
// =================================================================================================

static void Detour_UpdateCameraFromInput(long long camera_object, float delta_time) {
  g_last_camera_object = camera_object;

  if (g_current_camera_pos == g_previous_camera_pos) {
    if (!g_driver_pivot_saved && g_current_camera_pos == AnimationController::CameraPosition::Driver && g_ctx.cameraAPI) {
      float sx = 0.0f, sy = 0.0f, sz = 0.0f;
      if (g_ctx.cameraAPI->Cam_GetInteriorSeatPos(&sx, &sy, &sz)) {
        g_driver_seat_pivot = {sx, sy, sz};
        g_driver_pivot_saved = true;
      }
    }

    if (o_UpdateCameraFromInput) {
      o_UpdateCameraFromInput(camera_object, delta_time);
    }
  } else {
    if (g_previous_camera_pos != AnimationController::CameraPosition::Driver) {
      RestoreAzimuths(camera_object);
    }

    switch (g_current_camera_pos) {
      case AnimationController::CameraPosition::Passenger:
        BackupAndModifyAzimuths(camera_object);
        break;

      case AnimationController::CameraPosition::Standing:
      case AnimationController::CameraPosition::SofaSit1:
      case AnimationController::CameraPosition::SofaLie:
      case AnimationController::CameraPosition::SofaSit2:
        ZeroAzimuths(camera_object);
        break;

      case AnimationController::CameraPosition::Driver:
      default:
        break;
    }

    g_previous_camera_pos = g_current_camera_pos;

    if (o_UpdateCameraFromInput) {
      o_UpdateCameraFromInput(camera_object, delta_time);
    }
  }

  if ((g_current_camera_pos == AnimationController::CameraPosition::Standing || g_current_camera_pos == AnimationController::CameraPosition::SofaSit1 || g_current_camera_pos == AnimationController::CameraPosition::SofaLie ||
       g_current_camera_pos == AnimationController::CameraPosition::SofaSit2) &&
      g_ctx.cameraAPI) {
    float yaw = 0.0f;
    float pitch = 0.0f;
    g_ctx.cameraAPI->Cam_GetInteriorHeadRot(&yaw, &pitch);

    const float wrap_threshold = std::numbers::pi;
    const float wrap_value = 2.0f * std::numbers::pi;

    if (yaw > wrap_threshold) {
      g_ctx.cameraAPI->Cam_SetInteriorHeadRot(yaw - wrap_value, pitch);
    } else if (yaw < -wrap_threshold) {
      g_ctx.cameraAPI->Cam_SetInteriorHeadRot(yaw + wrap_value, pitch);
    }
  }
}

static void BackupAndModifyAzimuths(long long camera_object) {
  if (!g_ctx.cameraAPI) {
    return;
  }

  CameraPivot* pivot = GetPivot(camera_object);
  pivot->x = g_ctx.settings.positions.passenger_seat.position.x;
  pivot->y = g_ctx.settings.positions.passenger_seat.position.y;
  pivot->z = g_ctx.settings.positions.passenger_seat.position.z;

  if (g_ctx.cameraAPI->Cam_GetInteriorRotationLimits(&g_original_mouse_left_limit, &g_original_mouse_right_limit, &g_original_mouse_up_limit, &g_original_mouse_down_limit)) {
    float new_left_limit = g_original_mouse_right_limit * -1.0f;
    float new_right_limit = g_original_mouse_left_limit * -1.0f;

    g_ctx.cameraAPI->Cam_SetInteriorRotationLimits(new_left_limit, new_right_limit, g_original_mouse_up_limit, g_original_mouse_down_limit);
  }

  const size_t azimuth_count = g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverridesCount();
  g_azimuth_backup_count = (azimuth_count < 20) ? (uint32_t)azimuth_count : 20;

  for (uint32_t i = 0; i < g_azimuth_backup_count; ++i) {
    AzimuthBackup& backup = g_original_azimuth_values[i];
    float start_head_x = 0.0f, start_head_y = 0.0f, start_head_z = 0.0f;
    float end_head_x = 0.0f, end_head_y = 0.0f, end_head_z = 0.0f;
    bool outside_flag = false;

    if (!g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideStartAzimuth(i, &backup.start) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideEndAzimuth(i, &backup.end) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideOutside(i, &outside_flag) ||
        !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideStartHeadOffset(i, &start_head_x, &start_head_y, &start_head_z) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideEndHeadOffset(i, &end_head_x, &end_head_y, &end_head_z)) {
      continue;
    }

    backup.outside_flag = outside_flag ? 1 : 0;
    backup.start_head_offset = {start_head_x, start_head_y, start_head_z};
    backup.end_head_offset = {end_head_x, end_head_y, end_head_z};

    float new_start = -backup.start;
    float new_end = -backup.end;
    bool angles_were_swapped = false;

    if (new_start > new_end) {
      float temp = new_start;
      new_start = new_end;
      new_end = temp;
      angles_were_swapped = true;
    }

    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartAzimuth(i, new_start);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndAzimuth(i, new_end);

    if (angles_were_swapped) {
      g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartHeadOffset(i, backup.end_head_offset.x * -1.0f, backup.end_head_offset.y, backup.end_head_offset.z);
      g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndHeadOffset(i, backup.start_head_offset.x * -1.0f, backup.start_head_offset.y, backup.start_head_offset.z);
    } else {
      g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartHeadOffset(i, backup.start_head_offset.x * -1.0f, backup.start_head_offset.y, backup.start_head_offset.z);
      g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndHeadOffset(i, backup.end_head_offset.x * -1.0f, backup.end_head_offset.y, backup.end_head_offset.z);
    }
  }

  CacheExteriorSound(camera_object);
}

static void RestoreAzimuths(long long camera_object) {
  if (!g_ctx.cameraAPI) {
    if (g_ctx.loadAPI && g_ctx.loggerHandle) g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_WARN, "[RestoreAzimuths] cameraAPI is null, aborting");
    return;
  }

  CameraPivot* pivot = GetPivot(camera_object);

  if (g_ctx.loadAPI && g_ctx.loggerHandle && g_ctx.formattingAPI) {
    char buf[256];
    g_ctx.formattingAPI->Fmt_Format(
      buf, sizeof(buf), "[RestoreAzimuths] before: pivot=(%.2f,%.2f,%.2f) -> driver=(%.2f,%.2f,%.2f)", pivot->x, pivot->y, pivot->z, g_driver_seat_pivot.x, g_driver_seat_pivot.y, g_driver_seat_pivot.z);
    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, buf);
  }

  pivot->x = g_driver_seat_pivot.x;
  pivot->y = g_driver_seat_pivot.y;
  pivot->z = g_driver_seat_pivot.z;

  g_ctx.cameraAPI->Cam_SetInteriorRotationLimits(g_original_mouse_left_limit, g_original_mouse_right_limit, g_original_mouse_up_limit, g_original_mouse_down_limit);

  const size_t current_count = g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverridesCount();
  const size_t restore_count = (g_azimuth_backup_count < current_count) ? g_azimuth_backup_count : current_count;

  for (uint32_t i = 0; i < restore_count; ++i) {
    const AzimuthBackup& backup = g_original_azimuth_values[i];
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartAzimuth(i, backup.start);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndAzimuth(i, backup.end);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideOutside(i, backup.outside_flag != 0);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartHeadOffset(i, backup.start_head_offset.x, backup.start_head_offset.y, backup.start_head_offset.z);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndHeadOffset(i, backup.end_head_offset.x, backup.end_head_offset.y, backup.end_head_offset.z);
  }

  CacheExteriorSound(camera_object);
}

static void ZeroAzimuths(long long camera_object) {
  if (!g_ctx.cameraAPI) {
    return;
  }

  if (g_ctx.cameraAPI->Cam_GetInteriorRotationLimits(&g_original_mouse_left_limit, &g_original_mouse_right_limit, &g_original_mouse_up_limit, &g_original_mouse_down_limit)) {
    switch (g_current_camera_pos) {
      case AnimationController::CameraPosition::Standing:
        g_ctx.cameraAPI->Cam_SetInteriorRotationLimits(231.0f, -231.0f, g_original_mouse_up_limit, -80.0f);
        break;

      case AnimationController::CameraPosition::SofaSit1:
      case AnimationController::CameraPosition::SofaLie:
      case AnimationController::CameraPosition::SofaSit2:
        g_ctx.cameraAPI->Cam_SetInteriorRotationLimits(g_ctx.settings.sofa_limits.yaw_left, g_ctx.settings.sofa_limits.yaw_right, g_ctx.settings.sofa_limits.pitch_up, g_ctx.settings.sofa_limits.pitch_down);
        break;

      default:
        break;
    }
  }

  const size_t azimuth_count = g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverridesCount();
  g_azimuth_backup_count = (azimuth_count < 20) ? (uint32_t)azimuth_count : 20;

  for (uint32_t i = 0; i < g_azimuth_backup_count; ++i) {
    AzimuthBackup& backup = g_original_azimuth_values[i];
    float start_head_x = 0.0f, start_head_y = 0.0f, start_head_z = 0.0f;
    float end_head_x = 0.0f, end_head_y = 0.0f, end_head_z = 0.0f;
    bool outside_flag = false;

    if (!g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideStartAzimuth(i, &backup.start) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideEndAzimuth(i, &backup.end) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideOutside(i, &outside_flag) ||
        !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideStartHeadOffset(i, &start_head_x, &start_head_y, &start_head_z) || !g_ctx.cameraAPI->Cam_GetInteriorAzimuthOverrideEndHeadOffset(i, &end_head_x, &end_head_y, &end_head_z)) {
      continue;
    }

    backup.outside_flag = outside_flag ? 1 : 0;
    backup.start_head_offset = {start_head_x, start_head_y, start_head_z};
    backup.end_head_offset = {end_head_x, end_head_y, end_head_z};

    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartAzimuth(i, 0.0f);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndAzimuth(i, 0.0f);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideOutside(i, false);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideStartHeadOffset(i, 0.0f, 0.0f, 0.0f);
    g_ctx.cameraAPI->Cam_SetInteriorAzimuthOverrideEndHeadOffset(i, 0.0f, 0.0f, 0.0f);
  }

  CacheExteriorSound(camera_object);
}

void NotifySettingsUpdated() {
  if (g_current_camera_pos != AnimationController::CameraPosition::None && !SPF_CabinWalk::AnimationController::IsAnimating() && !SPF_CabinWalk::StandingAnimController::IsAnimating()) {
    g_previous_camera_pos = AnimationController::CameraPosition::None;
  }
}

void Shutdown() {
  if (g_ctx.loadAPI && g_ctx.loggerHandle && g_ctx.formattingAPI) {
    char buf[256];
    g_ctx.formattingAPI->Fmt_Format(buf, sizeof(buf), "[Shutdown] pos=%d prev=%d lastObj=%lld driverPivotSaved=%d", (int)g_current_camera_pos, (int)g_previous_camera_pos, g_last_camera_object, g_driver_pivot_saved ? 1 : 0);
    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, buf);
  }

  if (g_current_camera_pos != AnimationController::CameraPosition::Driver && g_last_camera_object != 0 && g_driver_pivot_saved) {
    RestoreAzimuths(g_last_camera_object);
    g_ctx.cameraAPI->Cam_SetInteriorSeatPos(g_driver_seat_pivot.x, g_driver_seat_pivot.y, g_driver_seat_pivot.z);
  }
  g_current_camera_pos = AnimationController::CameraPosition::Driver;
  g_previous_camera_pos = AnimationController::CameraPosition::Driver;
  g_last_camera_object = 0;
  AnimationController::Reset();
}
}  // namespace SPF_CabinWalk::CameraHookManager
