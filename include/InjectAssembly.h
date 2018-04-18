// Author: Sean Pesce
// Sections of the code in this file are based on code written by atom0s from the CheatEngine forums.
#pragma once

#include <Windows.h>

#define make_jump(f, t) (DWORD64)(((DWORD64)t - (DWORD64)f) - 5)
#define calc_dist(f, t) (DWORD64)((DWORD64)t - (DWORD64)f)
DWORD64 fov_changer_ret      = 0;
DWORD64 hand_fov_changer_ret = 0;


void inject_fov_changer(BYTE* new_code_addr)
{
    BYTE* code_addr = new_code_addr;

    // Unprotect memory
    DWORD old_protect = NULL;
    ::VirtualProtect(code_addr, 1000, PAGE_EXECUTE_READWRITE, &old_protect);

    // Original instruction: F3 0F59 40 38  - mulss xmm0,[rax+38]

    // Write jump to code cave
    BYTE* jmp_cave = (BYTE*)code_addr;
    memcpy((void*)jmp_cave, &fp_fov_modifier_jump[0], 24);
    *(DWORD64*)((DWORD64)jmp_cave + 10) = (DWORD64)(&fp_fov_modifier_code_cave[0] + 0x10);

    // Calculate return jump
    fov_changer_ret = (DWORD64)(jmp_cave + 0x18);

    ::VirtualProtect((DWORD64*)((DWORD64)&fp_fov_modifier_code_cave[0]), 2000, PAGE_EXECUTE_READWRITE, &old_protect);
    // jmp fov_changer_ret
    *(DWORD64*)(&fp_fov_modifier_code_cave[72]) = fov_changer_ret;
}

void inject_hand_fov_changer(BYTE* new_code_addr)
{
    BYTE* code_addr = new_code_addr;

    // Unprotect memory
    DWORD old_protect = NULL;
    ::VirtualProtect(code_addr, 1000, PAGE_EXECUTE_READWRITE, &old_protect);

    // Write jump to code cave
    BYTE* jmp_cave = (BYTE*)code_addr;
    memcpy((void*)jmp_cave, &fp_hands_fov_modifier_jump[0], 22);
    *(DWORD64*)((DWORD64)jmp_cave + 10) = (DWORD64)(&fp_fov_hands_modifier_code_cave[0] + 0x10);

    // Calculate return jump
    hand_fov_changer_ret = (DWORD64)(jmp_cave + 0x16);

    ::VirtualProtect((DWORD64*)((DWORD64)&fp_fov_hands_modifier_code_cave[0]), 1000, PAGE_EXECUTE_READWRITE, &old_protect);
    // jmp hand_fov_changer_ret
    *(DWORD64*)(&fp_fov_hands_modifier_code_cave[104]) = hand_fov_changer_ret;
}
