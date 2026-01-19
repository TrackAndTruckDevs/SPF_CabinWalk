#pragma once

#include <cstdint>
#include <SPF_Hooks_API.h>

namespace SPF_CabinWalk::Offsets
{
    // =================================================================================================
    // Signatures
    // =================================================================================================

    // Signature for the camera update function we hook. This is the main entry point for our modifications.
    const char *const G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 70 20 55 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 E8 33 DB";

    // Signature to find where the game loads the pointer to the array of `azimuth_range` structs and the element count.
    // This is searched in `UpdateCameraFromInput` as it's the function that iterates the whole array.
    const char *const G_AZIMUTH_ARRAY_AND_COUNT_PATTERN = "41 0f 57 c5 48 8b 8f";

    // Signature for the beginning of the UpdateInteriorCamera function. This is used as a base address
    // to search for all the detailed offsets below.
    const char *const G_UPDATE_INTERIOR_CAMERA_SIGNATURE = "48 83 EC 38 F3 0F 10 2D ? ? ? ? 4C 8B C2";

    // ---- Signatures within UpdateInteriorCamera ----

    // Finds `XORPS XMM2, XMM5` followed by `MOVSS XMM1, dword ptr [RDX + 0x10]`.
    // Used to get `start_azimuth_offset` (0x10).
    const char *const G_START_AZIMUTH_SIGNATURE = "0f 57 d5 f3 0f 10";

    // Finds `XORPS XMM3, XMM3` followed by `MOVSS XMM0, dword ptr [RDX + 0x14]`.
    // Used to get `end_azimuth_offset` (0x14).
    const char *const G_END_AZIMUTH_SIGNATURE = "0f 57 db f3 0f 10";

    // Finds `CMP byte ptr [RCX + ?], 0x0` followed by `MOVZX EAX, byte ptr [RDX + 0x18]`.
    // Used to get `azimuth_outside_flag_offset` (0x18).
    const char* const G_AZIMUTH_OUTSIDE_FLAG_SIGNATURE = "80 B9 ?? ?? ?? ?? ?? 0F B6";

    // Finds the entire block that reads both start and end head offsets to ensure uniqueness.
    // Used to find `start_head_offset_x_offset` (from `MOVSD XMM0, qword ptr [RDX + 0x3C]`)
    // and `end_head_offset_x_offset` (from `MOVSD XMM0, qword ptr [RDX + 0x48]`).
    const char *const G_HEAD_OFFSETS_SIGNATURE = "4C 8D 1D ? ? ? ? F2 0F 10 42 ? 89 44 24 08 8B 42 ? F2 0F 11 04 24 F2 0F 10 42 ?";

    // Finds `MOV EAX, R10D`, `MOV EBX, 0x494`, `CMOVZ EAX, EBX`.
    // Used to get `camera_pivot_offset` (0x494), which is the base position around which the camera rotates.
    const char *const G_CAMERA_PIVOT_SIGNATURE = "41 8B C2 BB ? ? ? ? 0F 44 C3";

    // Signature for the CacheExteriorSoundAngleRange function.
    // This function calculates and caches the angular range for outside sounds.
    const char *const G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE = "48 83 EC 48 44 0F B6 91 ? ? ? ? 45 84 D2 ? ? F3 0F 10 81 ? ? ? ? 0F 28";

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

        uint32_t azimuth_array_offset; // Offset to the pointer of the azimuth_range array
        uint32_t azimuth_count_offset; // Offset to the count of azimuth_range elements

        // --- Dynamically Found Offsets (from Azimuth Struct) ---
        uint32_t start_azimuth_offset;       // Offset within an azimuth struct to the start angle
        uint32_t end_azimuth_offset;         // Offset within an azimuth struct to the end angle
        uint32_t azimuth_outside_flag_offset; // Offset within an azimuth struct to the outside flag
        uint32_t start_head_offset_x_offset; // Offset within an azimuth struct to the start head_offset vector
        uint32_t end_head_offset_x_offset;   // Offset within an azimuth struct to the end head_offset vector

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
