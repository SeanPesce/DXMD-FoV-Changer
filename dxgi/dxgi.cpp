// Author: Sean Pesce

#include "DXMD_FoV_Changer.h"
#include "PatternFind.h"
#include "InjectAssembly.h"

float initFOV = 0.0;
float init_Hands_FOV = 0.0;

HINSTANCE mHinst = 0, mHinstDLL = 0;
extern "C" UINT_PTR mProcs[20] = {0};

void LoadOriginalDll();
int InitSettings();
DWORD WINAPI FOVChangerThread(LPVOID param);

LPCSTR mImportNames[] = {"ApplyCompatResolutionQuirking", "CompatString", "CompatValue", "CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2", "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice", "DXGID3D10ETWRundown", "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers", "DXGIDumpJournal", "DXGIGetDebugInterface1", "DXGIReportAdapterConfiguration", "DXGIRevertToSxS", "PIXBeginCapture", "PIXEndCapture", "PIXGetCaptureState", "SetAppCompatStringPointer", "UpdateHMDEmulationStatus"};
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
	mHinst = hinstDLL;
	if ( fdwReason == DLL_PROCESS_ATTACH ) {
		InitSettings();
		if (!mHinstDLL)
		{
			// No chain was loaded; get original DLL from system directory
			LoadOriginalDll();
		}
		if ( !mHinstDLL )
			return ( FALSE );
		for ( int i = 0; i < 20; i++ )
			mProcs[ i ] = (UINT_PTR)GetProcAddress( mHinstDLL, mImportNames[ i ] );
		lpvDXMDBase = NULL;
		dwDXMDSize = 0;
		GetModuleSize(GetModuleHandle(NULL), &lpvDXMDBase, &dwDXMDSize); // Obtain DXMD base address & size
		dxmdStartAddr = (DWORD64*)lpvDXMDBase; //Starting address of Deus Ex: Mankind Divided (can be used for static pointers)
		btdxmdStartAddr = (BYTE*)lpvDXMDBase;

		initKeybindsAndSettings();

		//Initiate thread(s)
		hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&FOVChangerThread, 0, 0, 0);
	} else if ( fdwReason == DLL_PROCESS_DETACH ) {
		FreeLibrary( mHinstDLL );
	}
	return ( TRUE );
}

extern "C" void ApplyCompatResolutionQuirking_wrapper();
extern "C" void CompatString_wrapper();
extern "C" void CompatValue_wrapper();
extern "C" void CreateDXGIFactory_wrapper();
extern "C" void CreateDXGIFactory1_wrapper();
extern "C" void CreateDXGIFactory2_wrapper();
extern "C" void DXGID3D10CreateDevice_wrapper();
extern "C" void DXGID3D10CreateLayeredDevice_wrapper();
extern "C" void DXGID3D10ETWRundown_wrapper();
extern "C" void DXGID3D10GetLayeredDeviceSize_wrapper();
extern "C" void DXGID3D10RegisterLayers_wrapper();
extern "C" void DXGIDumpJournal_wrapper();
extern "C" void DXGIGetDebugInterface1_wrapper();
extern "C" void DXGIReportAdapterConfiguration_wrapper();
extern "C" void DXGIRevertToSxS_wrapper();
extern "C" void PIXBeginCapture_wrapper();
extern "C" void PIXEndCapture_wrapper();
extern "C" void PIXGetCaptureState_wrapper();
extern "C" void SetAppCompatStringPointer_wrapper();
extern "C" void UpdateHMDEmulationStatus_wrapper();


// Loads the original DLL from the default system directory
//	Function originally written by Michael Koch
void LoadOriginalDll()
{



	char buffer[MAX_PATH];

	// Get path to system dir and to dxgi.dll
	GetSystemDirectory(buffer, MAX_PATH);

	// Append DLL name
	strcat_s(buffer, "\\dxgi.dll");

	// Try to load the system's dxgi.dll, if pointer empty
	if (!mHinstDLL) mHinstDLL = LoadLibrary(buffer);

	// Debug
	if (!mHinstDLL)
	{
		OutputDebugString("PROXYDLL: Original dxgi.dll not loaded ERROR ****\r\n");
		ExitProcess(0); // Exit the hard way
	}
}


// Parses dxgi.ini for intialization settings
int InitSettings()
{


	char dll_chain_buffer[128];

	// Check settings file for DLL chain
	GetPrivateProfileString("dxgi", "DLL_Chain", NULL, dll_chain_buffer, 128, ".\\retail\\DXMD_FOV.ini");

	if (dll_chain_buffer[0] != '\0') // Found DLL_Chain entry in settings file
	{
		mHinstDLL = LoadLibrary(dll_chain_buffer);
		if (!mHinstDLL)
		{
			// Failed to load next wrapper DLL
			OutputDebugString("PROXYDLL: Failed to load chained DLL; loading original from system directory instead...\r\n");
			return 2; // Return 2 if given DLL could not be loaded
		}
	}
	else
	{
		OutputDebugString("PROXYDLL: No DLL chain specified; loading original from system directory...\r\n");
		return 1; // Return 1 if dxgi.ini or DLL_Chain entry could not be located
	}
	return 0; // Return 0 on success
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
}


void initFOVModifiers() {
	fp_FOV_modifier_Instruction = 0;

	//Find Address of main first person FOV modifier (normally 1.0 max)
	////////////AOB Scan//////
	PFSEARCH pfS_FOV_Mod_Instr; // This is the pattern search struct
	while (fp_FOV_modifier_Instruction == 0) {
		FindPattern("F3 0F 59 40 38 48 8D 44 24 60 48 0F 46 C1 F3 44 0F 59 00 F3 41 0F 58 C0", &pfS_FOV_Mod_Instr, lpvDXMDBase, dwDXMDSize); // Perform search
		fp_FOV_modifier_Instruction = (DWORD64)(BYTE*)pfS_FOV_Mod_Instr.lpvResult;
	}
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


DWORD WINAPI FOVChangerThread(LPVOID param)
{

	bRunningFOVChanger = true;

	initFOVModifiers();

	while (bRunningFOVChanger)
	{

		if (FOV_Up_Key != 0 && GetAsyncKeyState(FOV_Up_Key) & 1)
		{
			//Increase FOV
			if (*(float*)ptr_FOV_Modifier <= 2.345) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier + 0.1;
				if (enableBeeps == 1) {
					Beep(523, 100);
				}
			}
		}
		else if (FOV_Down_Key != 0 && GetAsyncKeyState(FOV_Down_Key) & 1) {
			//Decrease FOV (minimum = 0.1)
			if (*(float*)ptr_FOV_Modifier >= 0.2) {
				*(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier - 0.1;
				if (enableBeeps == 1) {
					Beep(523, 100);
				}
			}
		}
		else if (Hands_FOV_Up_Key != 0 && GetAsyncKeyState(Hands_FOV_Up_Key) & 1) {
			//Increase Hands FOV
			current_Hands_FOV = current_Hands_FOV + 5;
			if (enableBeeps == 1) {
				Beep(523, 100);
			}
		}
		else if (Hands_FOV_Down_Key != 0 && GetAsyncKeyState(Hands_FOV_Down_Key) & 1) {
			//Decrease Hands FOV
			if (current_Hands_FOV >= 5) {
				current_Hands_FOV = current_Hands_FOV - 5;
				if (enableBeeps == 1) {
					Beep(523, 100);
				}
			}
		}
		else if (Restore_Preferred_FOV_Key != 0 && GetAsyncKeyState(Restore_Preferred_FOV_Key) & 1) {
			//Set both FoVs back to player's preferred defaults
			*(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
			current_Hands_FOV = init_Hands_FOV;
			if (enableBeeps == 1) {
				Beep(523, 100);
			}
		}
		else if (Reset_FOV_Key != 0 && GetAsyncKeyState(Reset_FOV_Key) & 1) {
			//Set both FoVs back to default game-supported settings
			current_Hands_FOV = default_Hands_FOV;
			*(float*)ptr_FOV_Modifier = default_FOV_Modifier;
			if (enableBeeps == 1) {
				Beep(523, 100);
			}
		}

		//Adjust hands FOV
		*(float*)ptr_fp_Hands_FOV_Base_Modifier = current_Hands_FOV / (*(float*)ptr_FOV_Modifier);
		Sleep(1);
	}

	return 0;
}
