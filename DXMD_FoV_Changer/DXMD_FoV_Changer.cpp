// DXMD_FoV_Changer.cpp : Defines the exported functions for the DLL application.
//
/*
#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>
#include "DXMD_FoV_Changer.h"
#include "PatternFind.h"
using namespace std;

HANDLE hModule;


#define IDR_INI1 101
HMODULE dllModule;

DWORD WINAPI FOVChangerThread(LPVOID param)
{
	
	bRunningFOVChanger = true;

	while (bRunningFOVChanger)
	{

		if (GetAsyncKeyState(FOV_Up_Key) & 1)
		{

			if (enableBeeps == 1) {
				Beep(523, 200);
			}

		}
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

		Beep(523, 200);
		//dwBase = Module("DXMD.exe");  //From old method (d3d9)

		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
		if (hProcess == NULL)
			::ExitProcess(0);


		///////////////Determine keybinds and config settings/////////////
		//
		//Determine whether or not to play beeps when enabling/disabling mods:
		char onOff[2];
		GetPrivateProfileString("Sounds", "Enable", "", onOff, 2, ".\\DXMD_FOV.ini");
		std::stringstream hsSounds;
		hsSounds << std::hex << onOff;
		hsSounds >> enableBeeps;
		if (enableBeeps != 1 && enableBeeps != 0) {
			//If beep value is not valid, set to default (0, disabled)
			enableBeeps = 0;
		}
		//Get "Increase FOV" keybind
		char keybind[3];
		GetPrivateProfileString("IncreaseFOV", "Key", "", keybind, 3, ".\\DXMD_FOV.ini");
		std::stringstream hsIncreaseFOV;
		hsIncreaseFOV << std::hex << keybind;
		hsIncreaseFOV >> FOV_Up_Key;

		//Initiate threads
		//hInitGameDataThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitGameDataThread, 0, 0, 0);
		hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&FOVChangerThread, 0, 0, 0);
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Thread, NULL, 0, NULL);
	}
	return TRUE;
}

*/


/*Old way (d3d9 wrapper)
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	LPVOID lpDummy = lpReserved;
	lpDummy = NULL;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: InitInstance(hModule); break;
	case DLL_PROCESS_DETACH: ExitInstance(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return TRUE;
}

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
	if (!hOriginalDll) LoadOriginalDll();

	typedef IDirect3D9 *(WINAPI* D3D9_Type)(UINT SDKVersion);
	D3D9_Type D3DCreate9_fn = (D3D9_Type)GetProcAddress(hOriginalDll, "Direct3DCreate9");

	if (!D3DCreate9_fn)
	{
		::ExitProcess(0);
	}

	IDirect3D9 *pIDirect3D9_orig = D3DCreate9_fn(SDKVersion);
	return pIDirect3D9_orig;
}

DWORD Module(LPSTR ModuleName)
{
	hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);
	do
		if (!strcmp(mEntry.szModule, ModuleName))
		{
			CloseHandle(hModule);
			return (DWORD)mEntry.modBaseAddr;
		}
	while (Module32Next(hModule, &mEntry));
	return 0;
}

void LoadOriginalDll(void)
{
	TCHAR buffer[MAX_PATH];
	::GetSystemDirectory(buffer, MAX_PATH);
	strcat_s(buffer, "\\d3d9.dll");

	if (!hOriginalDll) hOriginalDll = ::LoadLibrary(buffer);

	if (!hOriginalDll)
	{
		::ExitProcess(0);
	}
}

HRESULT InitGameDataThread()
{
	bRunningGameData = true;

	Sleep(1000);

	lpvDXMDBase = NULL;
	dwDXMDSize = 0;
	GetModuleSize(GetModuleHandle(NULL), &lpvDXMDBase, &dwDXMDSize); // Obtain DXMD base address & size
	dxmdStartAddr = (int*)lpvDXMDBase; //Starting address of Deus Ex: Mankind Divided
	btdxmdStartAddr = (BYTE*)lpvDXMDBase;

	//Values that change will be constantly updated in this loop:
	while (bRunningGameData) {

		Sleep(1);
	}

	return 0;
}

HRESULT InitFOVChangerThread()
{
	bRunningFOVChanger = true;

	while (bRunningFOVChanger)
	{

		if (GetAsyncKeyState(FOV_Up_Key) & 1)
		{

			if (enableBeeps == 1) {
				Beep(523, 200);
			}

		}
		Sleep(1);
	}
	return 0;
}

void ExitInstance()
{
	if (hOriginalDll)
	{
		::FreeLibrary(hOriginalDll);
		hOriginalDll = NULL;
	}

	bRunningFOVChanger = false;
	CloseHandle(InitFOVChangerThread);

	bRunningGameData = false;
	CloseHandle(hInitGameDataThread);
}

void InitInstance(HANDLE hModule)
{

	hOriginalDll = NULL;
	hThisInstance = NULL;

	hThisInstance = (HINSTANCE)hModule;

	dwBase = Module("DXMD.exe");

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	if (hProcess == NULL)
		::ExitProcess(0);


	///////////////Determine keybinds and config settings/////////////
	//
	//Determine whether or not to play beeps when enabling/disabling mods:
	char onOff[2];
	GetPrivateProfileString("Sounds", "Enable", "", onOff, 2, ".\\DXMD_FOV.ini");
	std::stringstream hsSounds;
	hsSounds << std::hex << onOff;
	hsSounds >> enableBeeps;
	if (enableBeeps != 1 && enableBeeps != 0) {
		//If beep value is not valid, set to default (0, disabled)
		enableBeeps = 0;
	}
	//Get "Increase FOV" keybind
	char keybind[3];
	GetPrivateProfileString("IncreaseFOV", "Key", "", keybind, 3, ".\\DXMD_FOV.ini");
	std::stringstream hsIncreaseFOV;
	hsIncreaseFOV << std::hex << keybind;
	hsIncreaseFOV >> FOV_Up_Key;

	//Initiate threads
	hInitGameDataThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitGameDataThread, 0, 0, 0);
	hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitFOVChangerThread, 0, 0, 0);
}*/