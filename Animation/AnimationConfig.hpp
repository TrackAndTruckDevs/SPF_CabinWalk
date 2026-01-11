#pragma once
#include <cstdint>

namespace SPF_CabinWalk::Animation::Config
{
    // Default animation durations in milliseconds.
    constexpr uint64_t DRIVER_TO_PASSENGER_DURATION = 3500000;
    constexpr uint64_t PASSENGER_TO_DRIVER_DURATION = 3500000;
    constexpr uint64_t DRIVER_TO_STANDING = 6000000;
    constexpr uint64_t PASSENGER_TO_STANDING = 5000000;
    constexpr uint64_t STANDING_TO_DRIVER = 6000000;
    constexpr uint64_t DRIVER_TO_DRIVER_STANDING_MS = 6000000;

    namespace Stances
    {
        constexpr uint64_t CROUCH_DURATION = 1250000;
        constexpr uint64_t TIPTOE_DURATION = 1100000;
        constexpr float CROUCH_DEPTH = 0.5f;
        constexpr float TIPTOE_HEIGHT = 0.17f;

        namespace Triggers
        {
            // The pitch angle (in radians) the camera must be held at or beyond to trigger a stance change.
            constexpr float CROUCH_DOWN_ANGLE = -0.7f;
            constexpr float STAND_UP_ANGLE = 0.3f;
            constexpr float TIPTOE_UP_ANGLE = 0.5f;
            constexpr float STAND_DOWN_ANGLE = -0.3f;

            // The time in milliseconds the camera must be held in the trigger zone before the stance change occurs.
            constexpr uint64_t HOLD_TIME_MS = 1000000;
        }
    }

    namespace Walking
    {
        constexpr float MAX_Z = 0.65f;
        constexpr float MIN_Z = -0.55f;
        constexpr float STEP_AMOUNT = 0.35f;
        constexpr float BOB_AMOUNT = 0.02f;
        
        constexpr uint64_t STEP_DURATION = 250000;
        
        namespace FirstStep
        {
            constexpr uint64_t BASE_TURN_DURATION = 100000;
            constexpr uint64_t EXTRA_TURN_DURATION_PER_PI = 400000;
        }
    }

} // namespace SPF_CabinWalk::Animation::Config
