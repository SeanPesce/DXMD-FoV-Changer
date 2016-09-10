// DXMD_FoV_Changer.cpp : Defines the exported functions for the DLL application.
// Made by SeanP

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>
#include <exception>
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

	bRunningFOVChanger = true;

	initFOVModifiers();
	initGameStatReaders();
	initHUDModifiers();
	initRegenModifiers();

	while (bRunningFOVChanger)
	{
		
		if (FOV_Up_Key != 0 && GetAsyncKeyState(FOV_Up_Key) & 1)
		{
			//Increase FOV
			if (*(float*)ptr_FOV_Modifier <= 2.345) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier + 0.1;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if (FOV_Down_Key != 0 && GetAsyncKeyState(FOV_Down_Key) & 1) {
			//Decrease FOV (minimum = 0.1)
			if (*(float*)ptr_FOV_Modifier >= 0.2) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier - 0.1;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if (Hands_FOV_Up_Key != 0 && GetAsyncKeyState(Hands_FOV_Up_Key) & 1) {
			//Increase Hands FOV
			current_Hands_FOV = current_Hands_FOV + 5;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (Hands_FOV_Down_Key != 0 && GetAsyncKeyState(Hands_FOV_Down_Key) & 1) {
			//Decrease Hands FOV
			if (current_Hands_FOV >= 5) {
				current_Hands_FOV = current_Hands_FOV - 5;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if(Restore_Preferred_FOV_Key != 0 && GetAsyncKeyState(Restore_Preferred_FOV_Key) & 1){
			//Set both FoVs back to player's preferred defaults
			*(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
			current_Hands_FOV = init_Hands_FOV;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if(Reset_FOV_Key != 0 && GetAsyncKeyState(Reset_FOV_Key) & 1){
			//Set both FoVs back to default game-supported settings
			current_Hands_FOV = default_Hands_FOV;
			*(float*)ptr_FOV_Modifier = default_FOV_Modifier;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (hudScale_modifier_Instruction_block != 0 && Increase_HudScale_Key != 0 && GetAsyncKeyState(Increase_HudScale_Key) && HUD_Enabled & 1) {
			//Increase HUD Scale
			*(float*)hudScale_modifier_Address = *(float*)hudScale_modifier_Address + 0.1;
			current_HUD_Scale = *(float*)hudScale_modifier_Address;
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (hudScale_modifier_Instruction_block != 0 && Decrease_HudScale_Key != 0 && GetAsyncKeyState(Decrease_HudScale_Key) && HUD_Enabled & 1) {
			//Decrease HUD Scale (Minimum = 0.0)
			if (current_HUD_Scale >= 0.1) {
				*(float*)hudScale_modifier_Address = *(float*)hudScale_modifier_Address - 0.1;
				current_HUD_Scale = *(float*)hudScale_modifier_Address;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}else{
				*(float*)hudScale_modifier_Address = 0.0;
				current_HUD_Scale = *(float*)hudScale_modifier_Address;
				if (enableBeeps == 1) {
					Beep(523, 200);
				}
			}
		}else if (hudScale_modifier_Instruction_block != 0 && Toggle_HUD_Key != 0 && GetAsyncKeyState(Toggle_HUD_Key) & 1) {
			if (HUD_Enabled) {
				//Turn off HUD
				*(float*)hudScale_modifier_Address = 0.0;
				saveHudStates();
				turnOffHudStates();
				HUD_Enabled = false;
			}else{
				//Turn on HUD
				*(float*)hudScale_modifier_Address = current_HUD_Scale;
				restoreHudStates();
				HUD_Enabled = true;
			}
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (hudScale_modifier_Instruction_block != 0 && Reset_HudScale_Key != 0 && GetAsyncKeyState(Reset_HudScale_Key) & 1) {
			//Restore game default HUD Scale
			*(float*)hudScale_modifier_Address = default_HUD_Scale;
			current_HUD_Scale = *(float*)hudScale_modifier_Address;
			if (!HUD_Enabled) {
				restoreHudStates();
				HUD_Enabled = true;
			}
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (hudScale_modifier_Instruction_block != 0 && Restore_Preferred_HudScale_Key != 0 && GetAsyncKeyState(Restore_Preferred_HudScale_Key) & 1) {
			//Restore player's preferred HUD Scale
			*(float*)hudScale_modifier_Address = preferred_HUD_Scale;
			current_HUD_Scale = *(float*)hudScale_modifier_Address;
			if (!HUD_Enabled) {
				restoreHudStates();
				HUD_Enabled = true;
			}
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (Toggle_Regen_Key != 0 && GetAsyncKeyState(Toggle_Regen_Key) & 1) {
			if (regen_Enabled) {
				//Disable health and energy regeneration
				injectNopAtAddress((BYTE*)regen_Instruction, 5);
				regen_Enabled = false;
			}else{
				//Enable health and energy regeneration
				memcpy((void*)regen_Instruction, &regen_Timer_Instruction[0], 5);
				regen_Enabled = true;
			}
			if (enableBeeps == 1) {
				Beep(523, 200);
			}
		}else if (Check_Pacifist_Key != 0 && GetAsyncKeyState(Check_Pacifist_Key) & 1) {
			//Checking Pacifist Achievement status
			if (*(DWORD*)stat_killCount == 0) {
				//No one has died; pacifist achievement is still available
				Beep(523, 200);
			}else{
				//Alert player that achievement was failed
				Beep(423, 200);
				Sleep(100);
				Beep(423, 200);
				Sleep(100);
				Beep(423, 200);
			}
		}else if(monitorPacifist && noKills && *(DWORD*)stat_killCount > 0){
			//Monitoring Pacifist Achievement status
			
			//Someone died, Pacifist achievement is no longer available
			noKills = false;
			//Alert player that achievement was failed
			Beep(423, 200);
			Sleep(100);
			Beep(423, 200);
			Sleep(100);
			Beep(423, 200);
		}else if(monitorPacifist && !noKills && *(DWORD*)stat_killCount == 0){
			//Save was loaded; Pacifist achievement is available yet again
			noKills = true;
			//Beep(523, 200);
		}

		//Adjust hands FOV
		*(float*)ptr_fp_Hands_FOV_Base_Modifier = current_Hands_FOV / (*(float*)ptr_FOV_Modifier);
		Sleep(1);
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

		lpvDXMDBase = NULL;
		dwDXMDSize = 0;
		GetModuleSize(GetModuleHandle(NULL), &lpvDXMDBase, &dwDXMDSize); // Obtain DXMD base address & size
		dxmdStartAddr = (DWORD64*)lpvDXMDBase; //Starting address of Deus Ex: Mankind Divided (can be used for static pointers)
		btdxmdStartAddr = (BYTE*)lpvDXMDBase;

		initKeybindsAndSettings();

		//Initiate thread(s)
		hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&FOVChangerThread, 0, 0, 0);
	}
	return TRUE;
}

/**
 * Determine keybinds and global configuration settings for the plugin
 */
void initKeybindsAndSettings() {
	
	//Determine whether or not to play beeps when enabling/disabling mods:
	char onOff[2];
	GetPrivateProfileString("Sounds", "Enable", "0", onOff, 2, ".\\retail\\DXMD_FOV.ini");
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
	GetPrivateProfileString("FOV", "IncreaseFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsIncreaseFOV;
	hsIncreaseFOV << std::hex << keybind;
	hsIncreaseFOV >> FOV_Up_Key;
	//Get "Decrease FOV" keybind
	GetPrivateProfileString("FOV", "DecreaseFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsDecreaseFOV;
	hsDecreaseFOV << std::hex << keybind;
	hsDecreaseFOV >> FOV_Down_Key;
	//Get "Increase First Person Hands FOV" keybind
	GetPrivateProfileString("FOV", "IncreaseHandsFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsIncreaseHandsFOV;
	hsIncreaseHandsFOV << std::hex << keybind;
	hsIncreaseHandsFOV >> Hands_FOV_Up_Key;
	//Get "Decrease First Person Hands FOV" keybind
	GetPrivateProfileString("FOV", "DecreaseHandsFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsDecreaseHandsFOV;
	hsDecreaseHandsFOV << std::hex << keybind;
	hsDecreaseHandsFOV >> Hands_FOV_Down_Key;
	//Get "Restore Player's Given Default FOV" keybind
	GetPrivateProfileString("FOV", "RestorePreferredFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsRestorePreferredFOV;
	hsRestorePreferredFOV << std::hex << keybind;
	hsRestorePreferredFOV >> Restore_Preferred_FOV_Key;
	//Get "Restore game-supported FOV" keybind
	GetPrivateProfileString("FOV", "RestoreGameSupportedFOV_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsResetFOV;
	hsResetFOV << std::hex << keybind;
	hsResetFOV >> Reset_FOV_Key;
	//Get Default FOV
	initFOV = (float)GetPrivateProfileInt("FOV", "DefaultFOV", 90, ".\\retail\\DXMD_FOV.ini");
	//Get Default Hands FOV
	init_Hands_FOV = (float)(GetPrivateProfileInt("FOV", "DefaultHandsFOV", 70, ".\\retail\\DXMD_FOV.ini"));
	if (init_Hands_FOV < 5.0) {
		init_Hands_FOV = 0.1;
	}
	current_Hands_FOV = init_Hands_FOV;
	//Determine whether to enable HUD by default
	int tmp_enableHUD;
	GetPrivateProfileString("HUD", "EnableHud", "", onOff, 2, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsEnableHUD;
	hsEnableHUD << std::hex << onOff;
	hsEnableHUD >> tmp_enableHUD;
	if (tmp_enableHUD == 0) {
		HUD_Enabled = false;
	}
	//Get preferred starting HUD Scale
	char hudScaleValue[10];
	GetPrivateProfileString("HUD", "PreferredHudScale", "1.0", hudScaleValue, 10, ".\\retail\\DXMD_FOV.ini");
	try {
		/*//@todo: delete this test code
		std::ofstream writer("test.txt");
		writer << "hudScaleValue: " << hudScaleValue << std::endl;
		writer.close();
		//end of test code*/
		preferred_HUD_Scale = std::stof(hudScaleValue);
		if (preferred_HUD_Scale <= 0.0) {
			preferred_HUD_Scale = 0.0;
		}
	}
	catch (exception& prefHudScaleException) {
		preferred_HUD_Scale = default_HUD_Scale;
	}
	//Get "Toggle Hud" Keybind
	GetPrivateProfileString("HUD", "ToggleHud_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsToggleHUD;
	hsToggleHUD << std::hex << keybind;
	hsToggleHUD >> Toggle_HUD_Key;
	//Get "Increase HUD Scale" Keybind
	GetPrivateProfileString("HUD", "IncreaseHudScale_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsHUDScaleUp;
	hsHUDScaleUp << std::hex << keybind;
	hsHUDScaleUp >> Increase_HudScale_Key;
	//Get "Decrease HUD Scale" Keybind
	GetPrivateProfileString("HUD", "DecreaseHudScale_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsHUDScaleDown;
	hsHUDScaleDown << std::hex << keybind;
	hsHUDScaleDown >> Decrease_HudScale_Key;
	//Get "Restore Player's Given Default HUD Scale" Keybind
	GetPrivateProfileString("HUD", "RestorePreferredHudScale_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsRestorePreferredHUDScale;
	hsRestorePreferredHUDScale << std::hex << keybind;
	hsRestorePreferredHUDScale >> Restore_Preferred_HudScale_Key;
	//Get "Restore game default HUD Scale" Keybind
	GetPrivateProfileString("HUD", "RestoreGameSupportedHudScale_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsRestoreGameSupportedHUDScale;
	hsRestoreGameSupportedHUDScale << std::hex << keybind;
	hsRestoreGameSupportedHUDScale >> Reset_HudScale_Key;
	//Get "Toggle Health & Energy Regeneration" Keybind
	GetPrivateProfileString("Challenges", "ToggleRegen_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsToggleRegen;
	hsToggleRegen << std::hex << keybind;
	hsToggleRegen >> Toggle_Regen_Key;
	//Determine whether or not to monitor Pacifist achievement:
	int iOnOff = 0;
	GetPrivateProfileString("Achievements", "monitorPacifist", "0", onOff, 2, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsMonitorPacifist;
	hsMonitorPacifist << std::hex << onOff;
	hsMonitorPacifist >> iOnOff;
	if (iOnOff == 1) {
		monitorPacifist = true;
	}
	//Get "Check Pacifist Achievement Status" Keybind
	GetPrivateProfileString("Achievements", "checkPacifistStatus_Keybind", "0", keybind, 3, ".\\retail\\DXMD_FOV.ini");
	std::stringstream hsCheckPacifist;
	hsCheckPacifist << std::hex << keybind;
	hsCheckPacifist >> Check_Pacifist_Key;
}

void initFOVModifiers() {

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

	//Set starting FOV based on player's preferred FOV
	*(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
	if (*(float*)ptr_FOV_Modifier > 2.345) {
		*(float*)ptr_FOV_Modifier = 2.345;
	}
	else if (*(float*)ptr_FOV_Modifier < 0.1) {
		*(float*)ptr_FOV_Modifier = 0.1;
	}
	//Correct initFOV if given value wasn't valid
	initFOV = (*(float*)ptr_FOV_Modifier * default_Hands_FOV) / default_FOV_Modifier;
}

void initGameStatReaders() {
	//Find Address of instruction that accesses statistics struct
	////////////AOB Scan//////
	PFSEARCH pfS_stats_Instr; // This is the pattern search struct
	FindPattern("89 83 80 00 00 00 89 43 60 48 89 43 78 48 89 43 70 48 89 43 68 89 83 A8 00 00 00 89 83 88 00 00 00", &pfS_stats_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
	DWORD64 stats_Instruction_block = (DWORD64)(BYTE*)pfS_stats_Instr.lpvResult;
	////////Finished AOB Scan////////////
	memcpy(&stats_modifier_Instruction_orig[0], (void*)stats_Instruction_block, 33); //back up original bytes
	ptr_statsStart = (DWORD64*)(stats_Instruction_block + 17);
	injectStatsGetter((BYTE*)stats_Instruction_block);
	while (*(DWORD64*)ptr_statsStart == 0) {
		Sleep(5);
	}
	ptr_statsStart = (DWORD64*)(*ptr_statsStart);
	memcpy((void*)stats_Instruction_block, &stats_modifier_Instruction_orig[0], 33);//restore original bytes
	memset((void*)ptr_statsStart, 0, 0xA8);//Initalize stats to zero, as the initialization was skipped by the code injection
	stat_killCount = (DWORD64*)((DWORD64)ptr_statsStart + 0x80); //Get Kill Count stat pointer (if this is more than zero, Pacifist achievement was lost)
}

void initHUDModifiers() {
	//Find Address of instruction that modifies HUD Scale
	////////////AOB Scan//////
	PFSEARCH pfS_hudScaleMod_Instr; // This is the pattern search struct
	FindPattern("74 0A F3 0F 10 41 24 C3", &pfS_hudScaleMod_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
	hudScale_modifier_Instruction_block = (DWORD64)(BYTE*)pfS_hudScaleMod_Instr.lpvResult;
	////////Finished AOB Scan////////////

	DWORD64 tempHudScalePtr = 0;

	if (hudScale_modifier_Instruction_block != 0) {
		//For DX12 non-beta build (9/8/16 patch):

		//Store original bytes for restoration later:
		memcpy(&hudScale_modifier_Instruction_orig[0], (void*)hudScale_modifier_Instruction_block, 21);

		hudScale_modifier_Address = hudScale_modifier_Instruction_block + 21;
		injectHudScaleModifierGetter((BYTE*)hudScale_modifier_Instruction_block);
		while (*(DWORD64*)hudScale_modifier_Address == 14757395258967641292) {
			//while(hudScale_modifier_Address == CC CC CC CC CC CC CC CC)
			Sleep(2);
		}
		tempHudScalePtr = hudScale_modifier_Address;
		//Get HUD Scale modifier
		hudScale_modifier_Address = (*(DWORD64*)hudScale_modifier_Address) + 0x24;
		//Restore padding bytes between functions
		*(DWORD64*)tempHudScalePtr = 14757395258967641292;
		//Restore original HUD code:
		memcpy((void*)hudScale_modifier_Instruction_block, &hudScale_modifier_Instruction_orig[0], 21);

		//Initialize HUD values:
		*(float*)hudScale_modifier_Address = preferred_HUD_Scale;
		current_HUD_Scale = *(float*)hudScale_modifier_Address;
		saveHudStates();
		if (!HUD_Enabled) {
			*(float*)hudScale_modifier_Address = 0.0;
			turnOffHudStates();
		}
	}
	else {
		//For DX12 beta build (9/8/16 patch):

		//Try different AOB to find Address of instruction that modifies HUD Scale
		////////////AOB Scan//////
		FindPattern("74 07 F3 0F 10 41 24", &pfS_hudScaleMod_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
		hudScale_modifier_Instruction_block = (DWORD64)(BYTE*)pfS_hudScaleMod_Instr.lpvResult;
		////////Finished AOB Scan////////////

		if (hudScale_modifier_Instruction_block != 0) {
			//Store original bytes for restoration later:
			memcpy(&hudScale_modifier_Instruction_orig_beta[0], (void*)hudScale_modifier_Instruction_block, 19);

			hudScale_modifier_Address = hudScale_modifier_Instruction_block + 19;
			injectHudScaleModifierGetter_beta((BYTE*)hudScale_modifier_Instruction_block);
			while (*(DWORD64*)hudScale_modifier_Address == 14757395258967641292) {
				//while(hudScale_modifier_Address == CC CC CC CC CC CC CC CC)
				Sleep(2);
			}
			tempHudScalePtr = hudScale_modifier_Address;
			//Get HUD Scale modifier
			hudScale_modifier_Address = (*(DWORD64*)hudScale_modifier_Address) + 0x24;
			//Restore padding bytes between functions
			*(DWORD64*)tempHudScalePtr = 14757395258967641292;
			//Restore original HUD code:
			memcpy((void*)hudScale_modifier_Instruction_block, &hudScale_modifier_Instruction_orig_beta[0], 19);

			//Initialize HUD values:
			*(float*)hudScale_modifier_Address = preferred_HUD_Scale;
			current_HUD_Scale = *(float*)hudScale_modifier_Address;
			saveHudStates();
			if (!HUD_Enabled) {
				*(float*)hudScale_modifier_Address = 0.0;
				turnOffHudStates();
			}
		}
	}
}

void initRegenModifiers() {
	//Find Address of instruction that regenerates health and energy
	////////////AOB Scan//////
	PFSEARCH pfS_RegenTimer_Instr; // This is the pattern search struct
	FindPattern("F3 0F 11 43 4C 41 0F 97 D0", &pfS_RegenTimer_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
	regen_Instruction = (DWORD64)(BYTE*)pfS_RegenTimer_Instr.lpvResult;
	////////Finished AOB Scan////////////
}

void saveHudStates() {
	current_Crosshair_Value = *(BYTE*)(hudScale_modifier_Address - 0x153);
	current_Interaction_Prompts_Value = *(BYTE*)(hudScale_modifier_Address - 0x2A3);
	current_Pickup_Prompts_Value = *(BYTE*)(hudScale_modifier_Address - 0x63);
	current_Cover_Prompts_Value = *(BYTE*)(hudScale_modifier_Address - 0x273);
	current_Cover2Cov_Line_Value = *(BYTE*)(hudScale_modifier_Address - 0x243);
	current_Damage_Indicator_Value = *(BYTE*)(hudScale_modifier_Address - 0x93);
	current_Threat_Indicator_Value = *(BYTE*)(hudScale_modifier_Address - 0x33);
}

void restoreHudStates() {
	 *(BYTE*)(hudScale_modifier_Address - 0x153) = current_Crosshair_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x2A3) = current_Interaction_Prompts_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x63) = current_Pickup_Prompts_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x273) = current_Cover_Prompts_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x243) = current_Cover2Cov_Line_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x93) = current_Damage_Indicator_Value;
	 *(BYTE*)(hudScale_modifier_Address - 0x33) = current_Threat_Indicator_Value;
}

void turnOffHudStates() {
	*(BYTE*)(hudScale_modifier_Address - 0x153) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x2A3) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x63) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x273) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x243) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x93) = 0;
	*(BYTE*)(hudScale_modifier_Address - 0x33) = 0;
}

