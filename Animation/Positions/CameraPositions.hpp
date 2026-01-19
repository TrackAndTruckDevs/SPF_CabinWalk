#pragma once

#include <SPF_TelemetryData.h>

namespace SPF_CabinWalk::Animation
{
    /**
     * @struct Transform
     * @brief Represents a combination of position and rotation in 3D space.
     */
    struct Transform
    {
        SPF_FVector position;
        SPF_FVector rotation; // .x = yaw, .y = pitch, .z = roll
    };

    // All specific camera position data is now loaded from settings.json via g_ctx.settings.

} // namespace SPF_CabinWalk::Animation