#include "Easing.hpp"
#include <cmath> // For std::pow

namespace SPF_CabinWalk::Easing
{
    // Linear interpolation (no easing)
    float linear(float t)
    {
        return t;
    }

    // Quadratic easing functions (t^2)
    float easeInQuad(float t)
    {
        return t * t;
    }

    float easeOutQuad(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    float easeInOutQuad(float t)
    {
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }

    // Cubic easing functions (t^3)
    float easeInCubic(float t)
    {
        return t * t * t;
    }

    float easeOutCubic(float t)
    {
        return 1.0f - std::pow(1.0f - t, 3.0f);
    }

    float easeInOutCubic(float t)
    {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // Quartic easing functions (t^4)
    float easeInQuart(float t)
    {
        return t * t * t * t;
    }

    float easeOutQuart(float t)
    {
        return 1.0f - std::pow(1.0f - t, 4.0f);
    }

    float easeInOutQuart(float t)
    {
        return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
    }

    // Quintic easing functions (t^5)
    float easeInQuint(float t)
    {
        return t * t * t * t * t;
    }

    float easeOutQuint(float t)
    {
        return 1.0f - std::pow(1.0f - t, 5.0f);
    }

    float easeInOutQuint(float t)
    {
        return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
    }

    // Exponential easing functions (2^t)
    float easeInExpo(float t)
    {
        return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
    }

    float easeOutExpo(float t)
    {
        return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    }

    float easeInOutExpo(float t)
    {
        return t == 0.0f
                   ? 0.0f
               : t == 1.0f
                   ? 1.0f
               : t < 0.5f
                   ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                   : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
    }

} // namespace SPF_CabinWalk::Easing