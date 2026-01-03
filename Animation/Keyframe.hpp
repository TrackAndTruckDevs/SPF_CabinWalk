#pragma once
#include "Animation/Easing/Easing.hpp" // For easing function types
#include <cstdint> // For uint64_t

namespace SPF_CabinWalk::Animation
{
    /**
     * @brief Type definition for an easing function.
     * Takes a float (progress 0.0-1.0) and returns a float (eased value 0.0-1.0).
     */
    using EasingFunction = float (*)(float);

    /**
     * @struct Keyframe
     * @brief Represents a single point in an animation track with a specific value and easing.
     * @tparam T The type of the value stored in the keyframe (e.g., float, SPF_FVector).
     */
    template <typename T>
    struct Keyframe
    {
        float progress;              // Progress point for this keyframe (0.0 to 1.0).
        T value;                     // The target value at this keyframe.
        EasingFunction easing_function; // The easing function to use when interpolating TO this keyframe from the previous one.

        // Constructor
        Keyframe(float prog, T val, EasingFunction easing = Easing::linear)
            : progress(prog), value(val), easing_function(easing) {}
    };

} // namespace SPF_CabinWalk::Animation