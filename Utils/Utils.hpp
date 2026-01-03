#pragma once

namespace SPF_CabinWalk::Utils
{
    /**
     * @brief Performs linear interpolation between two values.
     * @param a The starting value.
     * @param b The ending value.
     * @param t The interpolation factor (typically between 0.0 and 1.0).
     * @return The interpolated value.
     */
    inline float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }
}
