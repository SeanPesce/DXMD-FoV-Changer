//Some parts of this injection technique were made with help from atom0s from the CheatEngine forums
#pragma once

#include <windows.h>

#define MakeJump(f, t) (DWORD64)(((DWORD64)t - (DWORD64)f) - 5)
#define CalcDist(f, t) (DWORD64)((DWORD64)t - (DWORD64)f)
DWORD64 dwFOVChangerJumpBack = 0;
DWORD64 dwHandFOVChangerJumpBack = 0;

//In-line ASM isn't supported in Visual x64... wtf... I guess I have to manually calculate and copy all the hex bytes...
/*void __declspec(naked) __stdcall AccessFOVModifier(void)
{
	__asm
	{
		// Do your inline asm here..
		//Original instruction: F3 0F59 40 38  - mulss xmm0,[rax+38]
		//mulss xmm0, [FOV_Modifier]

	}
}*/


void injectFOVChanger(BYTE* new_btCodeAddr)
{
	// Do your signature scanning here..
	BYTE* btCodeAddr = new_btCodeAddr;

	// Unprotect the memory to write your jump..
	DWORD dwOldProtect = NULL;
	::VirtualProtect(btCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// Store the original data from the memory here if you want to restore it later etc..
	//Original instruction: F3 0F59 40 38  - mulss xmm0,[rax+38]

	// Write the jump to the code cave..
	BYTE* btJmpCave = (BYTE*)btCodeAddr;
	memcpy((void*)btJmpCave, &fp_FOV_modifier_Jump[0], 24);
	*(DWORD64*)((DWORD64)btJmpCave + 10) = (DWORD64)(&fp_FOV_modifier_CodeCave[0] + 0x10);

	// Nop any extra data needed here..
	//memset((BYTE*)(btJmpCave + 5), 0x90, 0); //Not needed here

	// Calculate the return jump..
	dwFOVChangerJumpBack = (DWORD64)(btJmpCave + 0x18);

	::VirtualProtect((DWORD64*)((DWORD64)&fp_FOV_modifier_CodeCave[0]), 2000, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	//jmp dwFOVChangerJumpBack
	*(DWORD64*)(&fp_FOV_modifier_CodeCave[72]) = dwFOVChangerJumpBack;

	// If you wish; restore the memory protection here..
	//::VirtualProtect(btCodeAddr, 1000, dwOldProtect, NULL);
}

void injectHandFOVChanger(BYTE* new_btCodeAddr)
{
	// Do your signature scanning here..
	BYTE* btCodeAddr = new_btCodeAddr;

	// Unprotect the memory to write your jump..
	DWORD dwOldProtect = NULL;
	::VirtualProtect(btCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	// Store the original data from the memory here if you want to restore it later etc..

	// Write the jump to the code cave..
	BYTE* btJmpCave = (BYTE*)btCodeAddr;
	memcpy((void*)btJmpCave, &fp_Hands_FOV_modifier_Jump[0], 22);
	*(DWORD64*)((DWORD64)btJmpCave + 10) = (DWORD64)(&fp_FOV_Hands_modifier_CodeCave[0] + 0x10);

	// Nop any extra data needed here..
	//memset((BYTE*)(btJmpCave + 5), 0x90, 0); //Not needed here

	// Calculate the return jump..
	dwHandFOVChangerJumpBack = (DWORD64)(btJmpCave + 0x16);

	::VirtualProtect((DWORD64*)((DWORD64)&fp_FOV_Hands_modifier_CodeCave[0]), 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	//jmp dwHandFOVChangerJumpBack
	*(DWORD64*)(&fp_FOV_Hands_modifier_CodeCave[104]) = dwHandFOVChangerJumpBack;

	// If you wish; restore the memory protection here..
	//::VirtualProtect(btCodeAddr, 1000, dwOldProtect, NULL);
}

void injectNopAtAddress(BYTE* new_btCodeAddr, int nops)
{
	// Do your signature scanning here..
	BYTE* btNopCodeAddr = new_btCodeAddr;

	// Unprotect the memory to write your nops..
	DWORD dwOldProtect = NULL;
	::VirtualProtect(btNopCodeAddr, 1000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	BYTE* btCamJmpCave1 = (BYTE*)btNopCodeAddr;

	// Nop the data here..
	memset((BYTE*)(btCamJmpCave1), 0x90, nops);

	// If you wish; restore the memory protection here..
}