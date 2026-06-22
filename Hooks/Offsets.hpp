#pragma once

#include <cstdint>
#include <SPF_Hooks_API.h>

namespace SPF_CabinWalk::Offsets
{
    // =================================================================================================
    // Signatures
    // =================================================================================================

    // Signature for the camera update function we hook.
    // Gidra v1.60 Fun UpdateCameraFromInput (140875120)
    const char *const G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE = "48 ? ? 48 89 ? ? ? ? ? ? 48 8d ? ? 48 ? ? ? ? ? ? 48 89 ? ? 45";

    // Signature for the beginning of the UpdateInteriorCamera function. This is used as a base address
    // to search for the detailed offset below.
    // Gidra v1.60 Fun UpdateInteriorCamera (140877500)
    const char *const G_UPDATE_INTERIOR_CAMERA_SIGNATURE = "48 ? ? ? F3 0F ? ? ? ? ? ? 4C ? ? 0f ? ? ? ? 48";

    // Finds `MOV EAX, R10D`, `MOV EBX, 0x494`, `CMOVZ EAX, EBX`.
    // Used to get `camera_pivot_offset` (0x494), which is the base position around which the camera rotates.
    const char *const G_CAMERA_PIVOT_SIGNATURE = "41 ? ? BB ? ? ? ? 0F";

    // Signature for the CacheExteriorSoundAngleRange function.
    // This function calculates and caches the angular range for outside sounds.
    const char *const G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE = "80 b9 ? ? ? ? ? 74 ? f3 0f ? ? ? ? ? ? 0f";

    // =================================================================================================
    // Offsets Structure
    // =================================================================================================

    /**
     * @brief Holds all memory offsets used by the plugin.
     * @details These are found dynamically at runtime by the Find() function.
     */
    struct Offsets
    {
        // --- Dynamically Found Offsets (from Camera Object) ---
        uint32_t camera_pivot_offset;      // Offset to the base position vector around which the camera rotates.

        // --- Dynamically Found Function Pointers ---
        uintptr_t pfnCacheExteriorSoundAngleRange;
    };

    /**
     * @brief The single global instance of the plugin's offsets.
     */
    extern Offsets g_offsets;

    /**
     * @brief Finds all necessary memory offsets using signature scanning.
     * @param hooks_api A pointer to the SPF Hooks API.
     * @return true if all offsets were found successfully, false otherwise.
     */
    bool Find(const SPF_Hooks_API *hooks_api);

} // namespace SPF_CabinWalk::Offsets
