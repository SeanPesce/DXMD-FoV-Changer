// DXMD_FoV_Changer.cpp : Defines the exported functions for the DLL application.
// Made by SeanP

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>
#include "DXMD_FoV_Changer.h"
#include "PatternFind.h"
#include "InjectAssembly.h"
using namespace std;

HANDLE hModule;


#define IDR_INI1 101
HMODULE dllModule;
float initFOV = 0.0;
float init_Hands_FOV = 0.0;

DWORD WINAPI FOVChangerThread(LPVOID param)
{
	//Sleep(1000);

	bRunningFOVChanger = true;

	//Find Address of main first person FOV modifier (normally 1.0 max)
	////////////AOB Scan//////
	PFSEARCH pfS_FOV_Mod_Instr; // This is the pattern search struct
	FindPattern("F3 0F 59 40 38 48 8D 44 24 60 48 0F 46 C1 F3 44 0F 59 00 F3 41 0F 58 C0", &pfS_FOV_Mod_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
	fp_FOV_modifier_Instruction = (DWORD64)(BYTE*)pfS_FOV_Mod_Instr.lpvResult;
	////////Finished AOB Scan////////////

	//Inject code to access FOV modifier:
	injectFOVChanger((BYTE*)fp_FOV_modifier_Instruction);

	//Find Address of instruction that modifies first person hands
	////////////AOB Scan//////
	PFSEARCH pfS_hands_FOV_Instr; // This is the pattern search struct
	FindPattern("F3 0F 59 0D ? ? ? ? FF 90 F0 01 00 00", &pfS_hands_FOV_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
	fp_Hands_FOV_modifier_Instruction = (DWORD64)(BYTE*)pfS_hands_FOV_Instr.lpvResult;
	////////Finished AOB Scan////////////
	injectHandFOVChanger((BYTE*)fp_Hands_FOV_modifier_Instruction);

	Sleep(10);
	//Set starting FOV based on player's preferred FOV
	*(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
	if (*(float*)ptr_FOV_Modifier > 2.345) {
		*(float*)ptr_FOV_Modifier = 2.345;
	}else if (*(float*)ptr_FOV_Modifier < 0.1) {
		*(float*)ptr_FOV_Modifier = 0.1;
	}

	//Correct initFOV if given value wasn't valid
	initFOV = (*(float*)ptr_FOV_Modifier * default_Hands_FOV) / default_FOV_Modifier;

	while (bRunningFOVChanger)
	{
		
		if (GetAsyncKeyState(FOV_Up_Key) & 1)
		{
			//Increase FOV
			if (*(float*)ptr_FOV_Modifier <= 2.345) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier + 0.1;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if (GetAsyncKeyState(FOV_Down_Key) & 1) {
			//Decrease FOV (minimum = 0.1)
			if (*(float*)ptr_FOV_Modifier >= 0.2) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier - 0.1;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if (GetAsyncKeyState(Hands_FOV_Up_Key) & 1) {
			//Increase Hands FOV
			current_Hands_FOV = current_Hands_FOV + 5;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (GetAsyncKeyState(Hands_FOV_Down_Key) & 1) {
			//Decrease Hands FOV
			if (current_Hands_FOV >= 5) {
				current_Hands_FOV = current_Hands_FOV - 5;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if(GetAsyncKeyState(Restore_Preferred_FOV_Key) & 1){
			//Set both FoVs back to player's given defaults
			*(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
			current_Hands_FOV = init_Hands_FOV;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if(GetAsyncKeyState(Reset_FOV_Key) & 1){
			//Set both FoVs back to default game-supported settings
			current_Hands_FOV = default_Hands_FOV;
			*(float*)ptr_FOV_Modifier = default_FOV_Modifier;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}

		//Adjust hands FOV
		*(float*)ptr_fp_Hands_FOV_Base_Modifier = current_Hands_FOV / (*(float*)ptr_FOV_Modifier);
		Sleep(5);
	}

	return 0;
}


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	
	if (reason == DLL_PROCESS_ATTACH)
	{
		dllModule = hInst;

		hOriginalDll = NULL;
		hThisInstance = NULL;

		hThisInstance = (HINSTANCE)hModule;

		//dwBase = Module("DXMD.exe");  //From old method (d3d9)
		lpvDXMDBase = NULL;
		dwDXMDSize = 0;
		GetModuleSize(GetModuleHandle(NULL), &lpvDXMDBase, &dwDXMDSize); // Obtain DXMD base address & size
		dxmdStartAddr = (int*)lpvDXMDBase; //Starting address of Deus Ex: Mankind Divided
		btdxmdStartAddr = (BYTE*)lpvDXMDBase;

		//hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		//if (hProcess == NULL)
		//	::ExitProcess(0);


		///////////////Determine keybinds and config settings/////////////
		//
		//Determine whether or not to play beeps when enabling/disabling mods:
		char onOff[2];
		GetPrivateProfileString("Sounds", "Enable", "", onOff, 2, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsSounds;
		hsSounds << std::hex << onOff;
		hsSounds >> enableBeeps;
		
		if (enableBeeps != 1 && enableBeeps != 0) {
			//If beep value is not valid, set to default (0, disabled)
			enableBeeps = 0;
		}
		if (enableBeeps == 1) {
			Beep(523, 200);
		}
		//Get "Increase FOV" keybind
		char keybind[3];
		GetPrivateProfileString("IncreaseFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsIncreaseFOV;
		hsIncreaseFOV << std::hex << keybind;
		hsIncreaseFOV >> FOV_Up_Key;
		//Get "Decrease FOV" keybind
		GetPrivateProfileString("DecreaseFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsDecreaseFOV;
		hsDecreaseFOV << std::hex << keybind;
		hsDecreaseFOV >> FOV_Down_Key;
		//Get "Increase First Person Hands FOV" keybind
		GetPrivateProfileString("IncreaseHandsFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsIncreaseHandsFOV;
		hsIncreaseHandsFOV << std::hex << keybind;
		hsIncreaseHandsFOV >> Hands_FOV_Up_Key;
		//Get "Decrease First Person Hands FOV" keybind
		GetPrivateProfileString("DecreaseHandsFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsDecreaseHandsFOV;
		hsDecreaseHandsFOV << std::hex << keybind;
		hsDecreaseHandsFOV >> Hands_FOV_Down_Key;
		//Get "Restore Player's Given Default FOV" keybind
		GetPrivateProfileString("RestorePreferredFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsRestorePreferredFOV;
		hsRestorePreferredFOV << std::hex << keybind;
		hsRestorePreferredFOV >> Restore_Preferred_FOV_Key;
		//Get "Restore game-supported FOV" keybind
		GetPrivateProfileString("RestoreGameSupportedFOV", "Key", "", keybind, 3, ".\\retail\\DXMD_FOV.ini");
		std::stringstream hsResetFOV;
		hsResetFOV << std::hex << keybind;
		hsResetFOV >> Reset_FOV_Key;
		//Get Default FOV
		initFOV = (float)GetPrivateProfileInt("DefaultFOV", "FOV", 90, ".\\retail\\DXMD_FOV.ini");
		//Get Default Hands FOV
		init_Hands_FOV = (float)(GetPrivateProfileInt("DefaultHandsFOV", "FOV", 70, ".\\retail\\DXMD_FOV.ini"));
		if (init_Hands_FOV < 5.0) {
			init_Hands_FOV = 0.1;
		}
		current_Hands_FOV = init_Hands_FOV;

		//Initiate thread(s)
		hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&FOVChangerThread, 0, 0, 0);
	}
	return TRUE;
}

