#include "Animation/AnimationSequence.hpp"
#include "Animation/Easing/Easing.hpp" // For Easing functions
#include <iostream> // For debugging, remove later

namespace SPF_CabinWalk::Animation
{
    AnimationSequence::AnimationSequence()
        : m_duration_ms(0), m_is_playing(false), m_current_elapsed_time_ms(0)
    {
        // Initialize tracks
        m_position_x_track = std::make_unique<Track<float>>();
        m_position_y_track = std::make_unique<Track<float>>();
        m_position_z_track = std::make_unique<Track<float>>();
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
        if (!m_is_playing || !camera_api)
        {
            return false;
        }

        m_current_elapsed_time_ms += delta_time_ms;

        if (m_current_elapsed_time_ms >= m_duration_ms)
        {
            m_current_elapsed_time_ms = m_duration_ms; // Clamp to end
            m_is_playing = false;
        }

        // Calculate the overall progress of the sequence from 0.0 to 1.0
        float current_progress = (m_duration_ms == 0)
                                     ? 1.0f
                                     : static_cast<float>(m_current_elapsed_time_ms) / static_cast<float>(m_duration_ms);

        float current_x = m_position_x_track->Evaluate(current_progress, m_initial_camera_state.position.x);
        float current_y = m_position_y_track->Evaluate(current_progress, m_initial_camera_state.position.y);
        float current_z = m_position_z_track->Evaluate(current_progress, m_initial_camera_state.position.z);
        
        float current_yaw = m_rotation_yaw_track->Evaluate(current_progress, m_initial_camera_state.rotation.x);
        float current_pitch = m_rotation_pitch_track->Evaluate(current_progress, m_initial_camera_state.rotation.y);
        
        // float current_roll = m_rotation_roll_track->Evaluate(current_progress, m_initial_camera_state.rotation.z); // If roll is animated

        camera_api->SetInteriorSeatPos(current_x, current_y, current_z);
        camera_api->SetInteriorHeadRot(current_yaw, current_pitch);

        return m_is_playing;
    }

} // namespace SPF_CabinWalk::Animation