#pragma once
#include "Animation/Keyframe.hpp"
#include <SPF_TelemetryData.h> // For SPF_FVector
#include <vector>
#include <algorithm> // For std::sort

namespace SPF_CabinWalk::Animation
{
    /**
     * @class Track
     * @brief Manages a sequence of Keyframes for a single animatable property.
     * @tparam T The type of the value being animated (e.g., float, SPF_FVector).
     */
    template <typename T>
    class Track
    {
    private:
        std::vector<Keyframe<T>> m_keyframes;

    public:
        Track() = default;

        /**
         * @brief Adds a keyframe to the track, maintaining sorted order by progress.
         * @param keyframe The Keyframe object to add.
         */
        void AddKeyframe(const Keyframe<T>& keyframe)
        {
            m_keyframes.push_back(keyframe);
            std::sort(m_keyframes.begin(), m_keyframes.end(),
                      [](const Keyframe<T>& a, const Keyframe<T>& b) {
                          return a.progress < b.progress;
                      });
        }

        /**
         * @brief Checks if the track contains any keyframes.
         * @return True if the track is empty, false otherwise.
         */
        bool IsEmpty() const
        {
            return m_keyframes.empty();
        }

        /**
         * @brief Evaluates the track at a specific progress point, returning the interpolated value.
         * @param current_progress The current progress of the animation sequence (0.0 to 1.0).
         * @param default_value A default value to return if the track is empty.
         * @return The interpolated value at the given progress point.
         */
        T Evaluate(float current_progress, T default_value) const
        {
            if (m_keyframes.empty())
            {
                return default_value;
            }

            // If progress is before the first keyframe, return the first keyframe's value
            if (current_progress <= m_keyframes.front().progress)
            {
                return m_keyframes.front().value;
            }

            // If progress is after the last keyframe, return the last keyframe's value
            if (current_progress >= m_keyframes.back().progress)
            {
                return m_keyframes.back().value;
            }

            // Find the two keyframes to interpolate between
            auto it_end = std::upper_bound(m_keyframes.begin(), m_keyframes.end(), current_progress,
                                          [](float prog, const Keyframe<T>& kf) {
                                              return prog < kf.progress;
                                          });
            auto it_start = std::prev(it_end);

            const Keyframe<T>& start_kf = *it_start;
            const Keyframe<T>& end_kf = *it_end;

            // Calculate progress between the two keyframes (local progress)
            float duration_between_keyframes = end_kf.progress - start_kf.progress;
            if (duration_between_keyframes == 0.0f)
            {
                return start_kf.value;
            }

            float local_progress = (current_progress - start_kf.progress) / duration_between_keyframes;
            float eased_progress = end_kf.easing_function(local_progress);

            // Interpolate the value
            return lerp(start_kf.value, end_kf.value, eased_progress);
        }

    private:
        // Generic linear interpolation function (needs to be specialized for custom types like SPF_FVector)
        T lerp(const T& a, const T& b, float t) const
        {
            return a + (b - a) * t;
        }
    };

    // Specialization for SPF_FVector
    template <>
    inline SPF_FVector Track<SPF_FVector>::lerp(const SPF_FVector& a, const SPF_FVector& b, float t) const
    {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t
        };
    }

    // Specialization for float (already covered by generic, but explicit for clarity/potential future needs)
    template <>
    inline float Track<float>::lerp(const float& a, const float& b, float t) const
    {
        return a + (b - a) * t;
    }

} // namespace SPF_CabinWalk::Animation