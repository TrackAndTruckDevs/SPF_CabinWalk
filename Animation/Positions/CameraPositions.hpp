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

        const Transform SOFA_SIT1_TARGET = {
            {0.5f, 0.0f, 0.8f}, // Placeholder position
            {0.0f, 0.0f, 0.0f}      // Placeholder rotation (facing left)
        };

        const Transform SOFA_LIE_TARGET = {
            {-0.15f, -0.25f, 1.25f}, // Placeholder position
            {-1.65f, 0.35f, 0.0f}  // Placeholder rotation (facing ceiling)
        };

        const Transform SOFA_SIT2_TARGET = {
            {0.2f, 0.0f, 0.8f},  // Placeholder position
            {0.0f, 0.0f, 0.0f}      // Placeholder rotation (facing left)
        };

        // TODO: Add other absolute target positions like Standing, Bed, etc.

    } // namespace CameraPositions
} // namespace SPF_CabinWalk::Animation

