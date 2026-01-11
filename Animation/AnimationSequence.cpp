#include "Animation/AnimationSequence.hpp"
#include "Animation/Easing/Easing.hpp" // For Easing functions
#include <iostream> // For debugging, remove later

namespace SPF_CabinWalk::Animation
{
    AnimationSequence::AnimationSequence()
        : m_duration_ms(0), m_is_playing(false), m_current_elapsed_time_ms(0)
    {
        // Initialize position tracks
        m_position_x_track = std::make_unique<Track<float>>();
        m_position_y_track = std::make_unique<Track<float>>();
        m_position_z_track = std::make_unique<Track<float>>();
        // Initialize rotation tracks
        m_rotation_yaw_track = std::make_unique<Track<float>>();
        m_rotation_pitch_track = std::make_unique<Track<float>>();
        m_rotation_roll_track = std::make_unique<Track<float>>();
    }

    void AnimationSequence::Initialize(uint64_t duration_ms)
    {
        m_duration_ms = duration_ms;
        // The tracks are already initialized in the constructor.
    }

    void AnimationSequence::Start(const CurrentCameraState& initial_state)
    {
        m_initial_camera_state = initial_state;
        m_current_elapsed_time_ms = 0;
        m_is_playing = true;
        // std::cout << "AnimationSequence Started. Initial Pos: {" << initial_state.position.x << "," << initial_state.position.y << "," << initial_state.position.z << "}" << std::endl; // Debug
    }

    bool AnimationSequence::Update(uint64_t delta_time_ms, const SPF_Camera_API* camera_api)
    {
        if (!m_is_playing)
        {
            return false;
        }

        m_current_elapsed_time_ms += delta_time_ms;

        if (m_current_elapsed_time_ms >= m_duration_ms)
        {
            m_current_elapsed_time_ms = m_duration_ms;
            m_is_playing = false;
        }

        if (!camera_api)
        {
            return m_is_playing;
        }

        const float current_progress = (m_duration_ms == 0)
                                     ? 1.0f
                                     : static_cast<float>(m_current_elapsed_time_ms) / static_cast<float>(m_duration_ms);

        // Start with the initial state as the default
        SPF_FVector final_pos = m_initial_camera_state.position;
        SPF_FVector final_rot = m_initial_camera_state.rotation;

        // --- Absolute Position Tracks ---
        // If a track for an axis exists, it overrides the initial state's value.
        // If it doesn't exist, the initial state's value is kept.
        if (!m_position_x_track->IsEmpty())
        {
            final_pos.x = m_position_x_track->Evaluate(current_progress, m_initial_camera_state.position.x);
        }
        if (!m_position_y_track->IsEmpty())
        {
            final_pos.y = m_position_y_track->Evaluate(current_progress, m_initial_camera_state.position.y);
        }
        if (!m_position_z_track->IsEmpty())
        {
            final_pos.z = m_position_z_track->Evaluate(current_progress, m_initial_camera_state.position.z);
        }

        // --- Absolute Rotation Tracks ---
        if (!m_rotation_yaw_track->IsEmpty())
        {
            final_rot.x = m_rotation_yaw_track->Evaluate(current_progress, m_initial_camera_state.rotation.x);
        }
        if (!m_rotation_pitch_track->IsEmpty())
        {
            final_rot.y = m_rotation_pitch_track->Evaluate(current_progress, m_initial_camera_state.rotation.y);
        }
        if (!m_rotation_roll_track->IsEmpty())
        {
            final_rot.z = m_rotation_roll_track->Evaluate(current_progress, m_initial_camera_state.rotation.z);
        }
        
        // NOTE: Offset tracks are now IGNORED by this logic, and will be removed entirely.

        // Apply the final calculated state to the camera
        camera_api->SetInteriorSeatPos(final_pos.x, final_pos.y, final_pos.z);
        camera_api->SetInteriorHeadRot(final_rot.x, final_rot.y);

        return m_is_playing;
    }

} // namespace SPF_CabinWalk::Animation