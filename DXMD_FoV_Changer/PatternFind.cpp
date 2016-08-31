//Most of the original code in this file was written by Franc[e]sco (from ccplz.net) or SteveAndrew (from the Cheat Engine forums)

#include "stdafx.h"
#include "PatternFind.h"
#include <dbghelp.h>
#include <psapi.h>
#include "stdio.h"
#include <windows.h>
#pragma  comment(lib, "dbghelp")
#pragma  comment(lib, "psapi")

BOOL GetModuleSize(HMODULE hModule, LPVOID* lplpBase, LPDWORD lpdwSize) {
	if (hModule == GetModuleHandle(NULL)) {
		PIMAGE_NT_HEADERS pImageNtHeaders = ImageNtHeader((PVOID)hModule);

		if (pImageNtHeaders == NULL)
			return FALSE;

		*lplpBase = (LPVOID)hModule;
		*lpdwSize = pImageNtHeaders->OptionalHeader.SizeOfImage;
	}
	else {
		MODULEINFO  ModuleInfo;

		if (!GetModuleInformation(GetCurrentProcess(), hModule, &ModuleInfo, sizeof(MODULEINFO)))
			return FALSE;

		*lplpBase = ModuleInfo.lpBaseOfDll;
		*lpdwSize = ModuleInfo.SizeOfImage;
	}
	return TRUE;
}

DWORD PFAPI GetPatternCB(char *szPattern) {
	DWORD cb = 0;
	bool first_nibble = false;
	for (DWORD i = 0; i<strlen(szPattern); i++) {
		char c = toupper(szPattern[i]);
		if (c != ' ') {
			if (c == '?') {
				if (!first_nibble) cb++;
				else return 0;
			}
			else {
				if (!isxdigit(c)) return 0;
				if (first_nibble) cb++;
				first_nibble ^= true;
			}
		}
	}
	if (first_nibble) return 0;
	return cb;
}

BOOL PFAPI GeneratePatternMask(char *szPattern, char *buffer) {
	bool first_nibble = false;
	for (DWORD i = 0; i<strlen(szPattern); i++) {
		char c = toupper(szPattern[i]);
		if (c != ' ') {
			if (c == '?') {
				if (!first_nibble) strcat_s(buffer, MAX_PATTERN, "?");
				else return FALSE;
			}
			else {
				if (!isxdigit(c)) return FALSE;
				if (first_nibble) strcat_s(buffer, MAX_PATTERN, "x");
				first_nibble ^= true;
			}
		}
	}
	if (first_nibble) return FALSE;
	return TRUE;
}

BOOL PFAPI GeneratePatternBytes(char *szPattern, LPBYTE buffer) {
	bool first_nibble = false;
	DWORD cb = 0;
	for (DWORD i = 0; i<strlen(szPattern); i++) {
		char c = toupper(szPattern[i]);
		if (c != ' ') {
			if (c == '?') {
				if (!first_nibble) {
					buffer[cb] = 0x00;
					cb++;
				}
				else return FALSE;
			}
			else {
				if (!isxdigit(c)) return FALSE;
				if (first_nibble) {
					char byte[3] = { 0 };
					byte[0] = szPattern[i - 1];
					byte[1] = c;
					byte[2] = '\0';
					buffer[cb] = (BYTE)strtol(byte, NULL, 16);
					cb++;
				}
				first_nibble ^= true;
			}
		}
	}
	if (first_nibble) return FALSE;
	return TRUE;
}

VOID PFAPI SearchPattern(PFSEARCH *ppf, LPVOID lpvBase, DWORD dwSize) {

	ppf->lpvResult = 0;
	DWORD64 dwBase = reinterpret_cast<DWORD64>(lpvBase);
	for (DWORD64 i = dwBase; i<dwBase + dwSize; i++) {
		bool found = true;
		for (DWORD64 j = 0; j<ppf->dwLength; j++) {
			if (ppf->szMask[j] == 'x') {
				if (*reinterpret_cast<BYTE*>(i + j) != ppf->lpbData[j]) {
					found = false;
					break;
				}
			}
		}
		if (found) {
			ppf->lpvResult = reinterpret_cast<LPVOID>(i);
			return;
		}
	}
}

DWORD PFAPI FindPattern(char *szPattern, PFSEARCH *ppf, LPVOID lpvBase, DWORD dwSize) {

	ZeroMemory(ppf, sizeof(PFSEARCH));
	bool invalid = false;

	ppf->dwLength = GetPatternCB(szPattern);
	invalid = invalid || !ppf->dwLength;
	invalid = invalid || !GeneratePatternMask(szPattern, ppf->szMask);
	invalid = invalid || !GeneratePatternBytes(szPattern, ppf->lpbData);

	if (invalid)
		return PF_INVALID;

	if (ppf->dwLength > MAX_PATTERN)
		return PF_OVERFLOW;

	SearchPattern(ppf, lpvBase, dwSize);
	if (!ppf->lpvResult)
		return PF_NOT_FOUND;

	return PF_NONE;
}

HANDLE hModule2;
DWORD64 Module(LPSTR ModuleName)
{
	hModule2 = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

	MODULEENTRY32 mEntry;
	mEntry.dwSize = sizeof(mEntry);
	do
		if (!strcmp(mEntry.szModule, ModuleName))
		{
			CloseHandle(hModule2);
			return (DWORD64)mEntry.modBaseAddr;
		}
	while (Module32Next(hModule2, &mEntry));
	return 0;
}

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
//64-bit version:
//these methods are based on a template made by SteveAndrew on the CE forums: http://forum.cheatengine.org/viewtopic.php?p=5582089&sid=c996eadf821a0abc3524f51537313878
HMODULE hInst;
DWORD ProcessorArch, PageSize;
bool CheatEngineScanSettings = false;
char *dbg = new char[260];
LPFN_ISWOW64PROCESS fnIsWow64Process;

bool Compare(const BYTE *pData, const BYTE *bMask, const char *szMask)
{
	for (;*szMask;++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask) return 0;
	return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char *szMask)
{
	for (DWORD i = 0; i<dwLen; i++)
		if (Compare((BYTE*)(dwAddress + i), bMask, szMask))  return (DWORD)(dwAddress + i);
	return 0;
}

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
	if (fnIsWow64Process != 0)
	{
		fnIsWow64Process((HANDLE)-1, &bIsWow64);
	}
	return bIsWow64;
}

void WINAPI Initialize()
{
	SYSTEM_INFO si;

	if (IsWow64())
		GetNativeSystemInfo(&si);
	else
		GetSystemInfo(&si);

	PageSize = si.dwPageSize;
	ProcessorArch = si.wProcessorArchitecture;
	sprintf(dbg, "PageSize: %X, CPU Arch: %X", PageSize, ProcessorArch);
	OutputDebugStringA(dbg);
}

int WINAPI AOBScanWalkRegions(char* aobPattern, bool useCESettings)
{
	Initialize();
	
	CheatEngineScanSettings = useCESettings;

	DWORD lpAddr = Module("DXMD.exe");
	MEMORY_BASIC_INFORMATION mi;
	for (lpAddr; lpAddr<0x7FFFFFFFFFFFFFF; lpAddr += PageSize)
	{
		DWORD vq = VirtualQuery((void*)lpAddr, &mi, PageSize);
		if (vq == ERROR_INVALID_PARAMETER || vq == 0) break;

		//Skip mapped memory (usually emulator memory, like CE's default settings :D) 
		//Other two are MEM_IMAGE, and MEM_PRIVATE 
		if (CheatEngineScanSettings == true && mi.Type == MEM_MAPPED) continue;
		
		if (mi.Protect == PAGE_READONLY || mi.Protect == PAGE_READWRITE)
		{
			sprintf(dbg, "baseAddr: %08x; allocBase: %08x; Protection: %x; Type: %x", mi.BaseAddress, mi.AllocationBase,
				mi.Protect, mi.Type);
			OutputDebugStringA(dbg);

			////////////AOB Scan this region//////
			PFSEARCH pfS; // This is the pattern search struct
			DWORD aobResult = FindPattern(aobPattern, &pfS, (LPVOID)lpAddr, (DWORD)mi.RegionSize); // Perform search
			DWORD* btAOBAddr = (DWORD*)pfS.lpvResult;
			if ((int)btAOBAddr != 0) {
				//@todo: if address found is itself, break and retry 10 times. If still not found, return 0

				if ((int)btAOBAddr != (int)&aobPattern) {
					return (int)btAOBAddr;
				}

			}
			////////Finished AOB Scan////////////
		}
	}

	FreeLibraryAndExitThread(hInst, 0);
}

/*extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hinstDLL;
		DisableThreadLibraryCalls(hInst);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)AOBScanReadOnlyMemory, 0, 0, 0);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}*/

/*
//32-bit version:
//next two methods are based on a template made by SteveAndrew on the CE forums: http://forum.cheatengine.org/viewtopic.php?p=5582089&sid=c996eadf821a0abc3524f51537313878
HMODULE hInst;
DWORD ProcessorArch, PageSize;
bool CheatEngineScanSettings = false;
char *dbg = new char[260];

void WINAPI Initialize()
{
	SYSTEM_INFO si;


	GetSystemInfo(&si);

	PageSize = si.dwPageSize;
	ProcessorArch = si.wProcessorArchitecture;
	sprintf(dbg, "PageSize: %X, CPU Arch: %X", PageSize, ProcessorArch);
	OutputDebugStringA(dbg);
}

int WINAPI AOBScanWalkRegions(char* aobPattern, bool useCESettings)
{
	Initialize();

	CheatEngineScanSettings = useCESettings;

	MEMORY_BASIC_INFORMATION mi;
	for (DWORD lpAddr = 0; lpAddr<0x7FFFFFFF; lpAddr += PageSize)
	{
		DWORD vq = VirtualQuery((void*)lpAddr, &mi, PageSize);
		if (vq == ERROR_INVALID_PARAMETER || vq == 0) break;

		//Skip mapped memory (usually emulator memory, like CE's default settings :D)
		//Other two are MEM_IMAGE, and MEM_PRIVATE
		if (CheatEngineScanSettings == true && mi.Type == MEM_MAPPED)
		{
			lpAddr += (mi.RegionSize - PageSize); //move past region
			continue;
		}

		if (mi.Protect == PAGE_READONLY || mi.Protect == PAGE_READWRITE)
		{
			sprintf(dbg, "baseAddr: %08x; allocBase: %08x; Size: %x; Protection: %x; Type: %x",
				mi.BaseAddress, mi.AllocationBase, mi.RegionSize, mi.Protect, mi.Type);
			OutputDebugStringA(dbg);

			////////////AOB Scan this region//////
			PFSEARCH pfAllGL; // This is the pattern search struct
			DWORD aobResult = FindPattern(aobPattern, &pfAllGL, (LPVOID)lpAddr, (DWORD)mi.RegionSize); // Perform search
			DWORD* btAOBAddr = (DWORD*)pfAllGL.lpvResult;
			if ((int)btAOBAddr != 0) {
				//@todo: if address found is itself, break and retry 10 times. If still not found, return 0

				if ((int)btAOBAddr != (int)&aobPattern) {
					return (int)btAOBAddr;
				}
				
			}
			////////Finished AOB Scan////////////

		}
		lpAddr += (mi.RegionSize - PageSize); //move past region
	}

	FreeLibraryAndExitThread(hInst, 0);
}*/