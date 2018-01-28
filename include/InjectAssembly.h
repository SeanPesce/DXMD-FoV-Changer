// Author: Sean Pesce
// Sections of the code in this file are based on code written by atom0s from the CheatEngine forums.
#pragma once

#include <Windows.h>

#define MakeJump(f, t) (DWORD64)(((DWORD64)t - (DWORD64)f) - 5)
#define CalcDist(f, t) (DWORD64)((DWORD64)t - (DWORD64)f)
DWORD64 dwFOVChangerJumpBack     = 0;
DWORD64 dwHandFOVChangerJumpBack = 0;


void injectFOVChanger(BYTE* new_btCodeAddr)
{
    BYTE* btCodeAddr = new_btCodeAddr;

    // Unprotect memory
    DWORD dwOldProtect = NULL;
    ::VirtualProtect(btCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    // Original instruction: F3 0F59 40 38  - mulss xmm0,[rax+38]

    // Write jump to code cave
    BYTE* btJmpCave = (BYTE*)btCodeAddr;
    memcpy((void*)btJmpCave, &fp_FOV_modifier_Jump[0], 24);
    *(DWORD64*)((DWORD64)btJmpCave + 10) = (DWORD64)(&fp_FOV_modifier_CodeCave[0] + 0x10);

    // Calculate return jump
    dwFOVChangerJumpBack = (DWORD64)(btJmpCave + 0x18);

    ::VirtualProtect((DWORD64*)((DWORD64)&fp_FOV_modifier_CodeCave[0]), 2000, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    // jmp dwFOVChangerJumpBack
    *(DWORD64*)(&fp_FOV_modifier_CodeCave[72]) = dwFOVChangerJumpBack;
}

void injectHandFOVChanger(BYTE* new_btCodeAddr)
{
    BYTE* btCodeAddr = new_btCodeAddr;

    // Unprotect memory
    DWORD dwOldProtect = NULL;
    ::VirtualProtect(btCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    // Write jump to code cave
    BYTE* btJmpCave = (BYTE*)btCodeAddr;
    memcpy((void*)btJmpCave, &fp_Hands_FOV_modifier_Jump[0], 22);
    *(DWORD64*)((DWORD64)btJmpCave + 10) = (DWORD64)(&fp_FOV_Hands_modifier_CodeCave[0] + 0x10);

    // Calculate return jump
    dwHandFOVChangerJumpBack = (DWORD64)(btJmpCave + 0x16);

    ::VirtualProtect((DWORD64*)((DWORD64)&fp_FOV_Hands_modifier_CodeCave[0]), 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    // jmp dwHandFOVChangerJumpBack
    *(DWORD64*)(&fp_FOV_Hands_modifier_CodeCave[104]) = dwHandFOVChangerJumpBack;
}


void injectNopAtAddress(BYTE* new_btCodeAddr, int nops)
{
    BYTE* btNopCodeAddr = new_btCodeAddr;

    // Unprotect memory
    DWORD dwOldProtect = NULL;
    ::VirtualProtect(btNopCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    BYTE* btCamJmpCave1 = (BYTE*)btNopCodeAddr;
    memset((BYTE*)(btCamJmpCave1), 0x90, nops);
}