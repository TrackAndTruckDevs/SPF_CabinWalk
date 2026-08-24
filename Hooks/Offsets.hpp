#pragma once

#include <cstdint>
#include <SPF_Hooks_API.h>

namespace SPF_CabinWalk::Offsets {
// =================================================================================================
// Signatures
// =================================================================================================

// Signature for the camera update function we hook.
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraFromInput[1408750d0]) ---/
* 1408750d0  48 8B C4                      MOV RAX,RSP
* 1408750d3  48 89 58 18                   MOV qword ptr [RAX + 0x18],RBX
* 1408750d7  55                            PUSH RBP
* 1408750d8  57                            PUSH RDI
* 1408750d9  41 56                         PUSH R14
* 1408750db  48 8D 68 A8                   LEA RBP,[RAX + -0x58]
* 1408750df  48 81 EC 40 01 00 00          SUB RSP,0x140
* 1408750e6  48 89 70 08                   MOV qword ptr [RAX + 0x8],RSI
* 1408750ea  45 33 F6                      XOR R14D,R14D
*/
const char* const G_UPDATE_CAMERA_FROM_INPUT_SIGNATURE = "[MOV r64, r64] [MOV [r64+off8], r64] [PUSH r64] [PUSH r64] [PUSH R8-R15] [LEA r64, [r64+off8]] [SUB r64, imm32] [MOV [r64+off8], r64] 45 [XOR r32, r32]";

// Signature for the beginning of the UpdateInteriorCamera function. This is used as a base address
// to search for the detailed offset below.
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCamera[1408774b0]) ---/
* 1408774b0  48 83 EC 38                   SUB RSP,0x38
* 1408774b4  F3 0F 10 2D 64 42 B6 01       MOVSS XMM5,dword ptr [0x1423db720]
* 1408774bc  4C 8B C2                      MOV R8,RDX
* 1408774bf  0F 29 74 24 20                MOVAPS xmmword ptr [RSP + 0x20],XMM6
* 1408774c4  48 85 D2                      TEST RDX,RDX
*/
const char* const G_UPDATE_INTERIOR_CAMERA_SIGNATURE = "[SUB r64, imm8] [MOVSS xmm, [rip+off32]] [MOV r64, r64] [MOVAPS [r64+off8], xmm] [TEST r64, r64]";

// Finds `MOV EAX, R10D`, `MOV EBX, 0x4a4`, `CMOVZ EAX, EBX`.
// Used to get `camera_pivot_offset` (0x4a4), which is the base position around which the camera rotates.
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCamera[1408774b0]) ---/
* 14087758c  41 8B C2                      MOV EAX,R10D
* 14087758f  BB A4 04 00 00                MOV EBX,0x4a4
* 140877594  0F 44 C3                      CMOVZ EAX,EBX
*/
const char* const G_CAMERA_PIVOT_SIGNATURE = "41 [MOV r32, r32] [MOV r32, imm32] [CMOVE r32, r32]";

// Signature for the CacheExteriorSoundAngleRange function.
// This function calculates and caches the angular range for outside sounds.
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(CacheExteriorSoundAngleRange[1408743d0]) ---/
* 1408743d0  80 B9 48 05 00 00 00          CMP byte ptr [RCX + 0x548],0x0
* 1408743d7  74 0D                         JZ 0x1408743e6
* 1408743d9  F3 0F 10 81 AC 05 00 00       MOVSS XMM0,dword ptr [RCX + 0x5ac]
* 1408743e1  0F 28 C8                      MOVAPS XMM1,XMM0
* 1408743e4  EB 10                         JMP 0x1408743f6
*/
const char* const G_CACHE_EXTERIOR_SOUND_ANGLE_RANGE_SIGNATURE = "[CMP byte ptr [r64+off32], imm8] [JE rel8] [MOVSS xmm, [r64+off32]] ? ? ? [JMP rel8]";

// =================================================================================================
// Offsets Structure
// =================================================================================================

/**
 * @brief Holds all memory offsets used by the plugin.
 * @details These are found dynamically at runtime by the Find() function.
 */
struct Offsets {
  // --- Dynamically Found Offsets (from Camera Object) ---
  uint32_t camera_pivot_offset;  // Offset to the base position vector around which the camera rotates.

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
bool Find(const SPF_Hooks_API* hooks_api);

}  // namespace SPF_CabinWalk::Offsets
