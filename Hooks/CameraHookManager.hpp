#pragma once

#include <SPF_Hooks_API.h>

namespace SPF_CabinWalk::CameraHookManager
{
    /**
     * @brief Initializes the camera hook manager and registers the necessary hook.
     * @param hooks_api A pointer to the SPF Hooks API.
     * @return true if initialization and hook registration were successful, false otherwise.
     */
    bool Initialize(const SPF_Hooks_API *hooks_api, const char *plugin_name);

    /**
     * @brief Sets the desired state for the camera modification.
     * @details This function should be called when the camera is about to arrive at the passenger seat
     * or is about to leave it.
     * @param is_on_passenger_seat True if the camera is on the passenger seat, false otherwise.
     */
    void SetPassengerSeatState(bool is_on_passenger_seat);

} // namespace SPF_CabinWalk::CameraHookManager
