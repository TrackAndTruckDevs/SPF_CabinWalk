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

    namespace CameraPositions
    {
        // This file stores absolute coordinates for TARGET positions within the cabin's coordinate space.
        // The animation system will interpolate from a dynamically read start position
        // (e.g., the driver's current seat position) to these fixed target positions.

        const Transform PASSENGER_SEAT_TARGET = {
            {0.95f, 0.0f, -0.03f}, // Absolute position {x, y, z} in cabin space.
            {0.03f, 0.03f, 0.0f}  // Target rotation {yaw, pitch, roll}.
        };

        const Transform STANDING_POSITION_TARGET = {
            {0.5f, 0.200f, 0.25f}, // Absolute position {x, y, z} in cabin space.
            {-0.17f, -0.3f, 0.0f}      // Target rotation {yaw, pitch, roll}.
        };

        // TODO: Add other absolute target positions like Standing, Bed, etc.

    } // namespace CameraPositions
} // namespace SPF_CabinWalk::Animation

