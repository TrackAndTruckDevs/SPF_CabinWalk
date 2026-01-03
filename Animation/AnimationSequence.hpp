#pragma once
#include "Animation/Track.hpp"
#include <memory> // For std::unique_ptr
#include <SPF_Camera_API.h>
#include <vector>

namespace SPF_CabinWalk::Animation
{
    /**
     * @brief Structure to hold the current interpolated state of the camera.
     */
    struct CurrentCameraState
    {
        SPF_FVector position;
        SPF_FVector rotation; // .x = yaw, .y = pitch, .z = roll
    };

    /**
     * @class AnimationSequence
     * @brief Manages multiple animation tracks over a shared timeline to produce a camera animation.
     */
    class AnimationSequence
    {
    private:
        // Individual tracks for each animatable property of the camera
        std::unique_ptr<Track<float>> m_position_x_track;
        std::unique_ptr<Track<float>> m_position_y_track;
        std::unique_ptr<Track<float>> m_position_z_track;
        std::unique_ptr<Track<float>> m_rotation_yaw_track;
        std::unique_ptr<Track<float>> m_rotation_pitch_track;
        std::unique_ptr<Track<float>> m_rotation_roll_track; // Roll might not be used, but included for completeness

        uint64_t m_duration_ms; // Total duration of the animation sequence

        // State variables for the current playback
        bool m_is_playing;
        uint64_t m_current_elapsed_time_ms;
        
        // Initial state of the camera when the animation started
        CurrentCameraState m_initial_camera_state;

    public:
        AnimationSequence();

        /**
         * @brief Initializes the animation sequence. Must be called before adding tracks.
         * @param duration_ms The total duration of this animation sequence in milliseconds.
         */
        void Initialize(uint64_t duration_ms);

        /**
         * @brief Adds a track for a specific property.
         * @tparam T The type of the property being animated.
         * @param track_ptr A unique_ptr to the configured Track object.
         * @param property_name A string identifier for the property (e.g., "position.x", "rotation.yaw").
         * @details This method needs a more robust way to map property_name to the correct member track.
         * For now, direct access or specific Add...Track methods might be simpler.
         */
        // void AddTrack(std::unique_ptr<Track<T>> track_ptr, const std::string& property_name);

        // Specific AddTrack methods for each property type
        void AddPositionXTrack(std::unique_ptr<Track<float>> track) { m_position_x_track = std::move(track); }
        void AddPositionYTrack(std::unique_ptr<Track<float>> track) { m_position_y_track = std::move(track); }
        void AddPositionZTrack(std::unique_ptr<Track<float>> track) { m_position_z_track = std::move(track); }
        void AddRotationYawTrack(std::unique_ptr<Track<float>> track) { m_rotation_yaw_track = std::move(track); }
        void AddRotationPitchTrack(std::unique_ptr<Track<float>> track) { m_rotation_pitch_track = std::move(track); }
        void AddRotationRollTrack(std::unique_ptr<Track<float>> track) { m_rotation_roll_track = std::move(track); }

        /**
         * @brief Starts the animation sequence from the beginning.
         * @param initial_state The state of the camera when the animation is initiated.
         */
        void Start(const CurrentCameraState& initial_state);

        /**
         * @brief Updates the animation state based on elapsed time.
         * @param delta_time_ms The time elapsed since the last frame in milliseconds.
         * @param camera_api A pointer to the SPF Camera API for applying changes.
         * @return True if the animation is still playing, false if it has finished.
         */
        bool Update(uint64_t delta_time_ms, const SPF_Camera_API* camera_api);

        /**
         * @brief Checks if the animation is currently playing.
         * @return True if playing, false otherwise.
         */
        bool IsPlaying() const { return m_is_playing; }

        /**
         * @brief Gets the total duration of the animation sequence.
         * @return The duration in milliseconds.
         */
        uint64_t GetDuration() const { return m_duration_ms; }
    };

} // namespace SPF_CabinWalk::Animation