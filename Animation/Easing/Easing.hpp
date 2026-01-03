#pragma once

namespace SPF_CabinWalk::Easing
{
    // Linear interpolation (no easing)
    float linear(float t);

    // Quadratic easing functions (t^2)
    float easeInQuad(float t);
    float easeOutQuad(float t);
    float easeInOutQuad(float t);

    // Cubic easing functions (t^3)
    float easeInCubic(float t);
    float easeOutCubic(float t);
    float easeInOutCubic(float t);

    // Quartic easing functions (t^4)
    float easeInQuart(float t);
    float easeOutQuart(float t);
    float easeInOutQuart(float t);

    // Quintic easing functions (t^5)
    float easeInQuint(float t);
    float easeOutQuint(float t);
    float easeInOutQuint(float t);

    // Exponential easing functions (2^t)
    float easeInExpo(float t);
    float easeOutExpo(float t);
    float easeInOutExpo(float t);

    // TODO: Consider implementing Circ, Back, Elastic, Bounce functions.
    // TODO: Consider implementing Bezier curves for maximum flexibility.
} // namespace SPF_CabinWalk::Easing