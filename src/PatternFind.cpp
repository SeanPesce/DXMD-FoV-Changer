// Author: Sean Pesce
// Sections of the code in this file were written by Franc[e]sco (from ccplz.net) and/or SteveAndrew (from the Cheat Engine forums)

#include "PatternFind.h"
#include <dbghelp.h>
#include <psapi.h>
#include "stdio.h"
#include <Windows.h>
#pragma  comment(lib, "dbghelp")
#pragma  comment(lib, "psapi")

#define _DXMD_DBG_BUFF_SIZE_ 260

BOOL GetModuleSize(HMODULE hModule, LPVOID* lplpBase, PDWORD64 lpdwSize) {
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

VOID PFAPI SearchPattern(PFSEARCH *ppf, LPVOID lpvBase, DWORD64 dwSize) {

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

DWORD PFAPI FindPattern(char *szPattern, PFSEARCH *ppf, LPVOID lpvBase, DWORD64 dwSize) {

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

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
// The following functions are based on a template by SteveAndrew on the CE forums: http://forum.cheatengine.org/viewtopic.php?p=5582089&sid=c996eadf821a0abc3524f51537313878
HMODULE hInst;
DWORD ProcessorArch, PageSize;
bool CheatEngineScanSettings = false;
char *dbg = new char[_DXMD_DBG_BUFF_SIZE_];
LPFN_ISWOW64PROCESS fnIsWow64Process;

bool Compare(const BYTE *pData, const BYTE *bMask, const char *szMask)
{
    for (;*szMask;++szMask, ++pData, ++bMask)
        if (*szMask == 'x' && *pData != *bMask) return 0;
    return (*szMask) == NULL;
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
    sprintf_s(dbg, _DXMD_DBG_BUFF_SIZE_, "PageSize: %X, CPU Arch: %X", PageSize, ProcessorArch);
    OutputDebugStringA(dbg);
}

unsigned long long WINAPI AOBScanWalkRegions(char* aobPattern, bool useCESettings)
{
    Initialize();
    
    CheatEngineScanSettings = useCESettings;

    DWORD64 lpAddr = (DWORD64)GetModuleHandle(NULL);
    MEMORY_BASIC_INFORMATION mi;
    for (lpAddr; lpAddr<0x7FFFFFFFFFFFFFF; lpAddr += PageSize)
    {
        SIZE_T vq = VirtualQuery((void*)lpAddr, &mi, PageSize);
        if (vq == ERROR_INVALID_PARAMETER || vq == 0) break;

        // Skip mapped memory, like the default CE settings (usually emulator memory)
        // Other two are MEM_IMAGE and MEM_PRIVATE
        if (CheatEngineScanSettings == true && mi.Type == MEM_MAPPED) continue;
        
        if (mi.Protect == PAGE_READONLY || mi.Protect == PAGE_READWRITE)
        {
            sprintf_s(dbg, _DXMD_DBG_BUFF_SIZE_, "baseAddr: %p; allocBase: %p; Protection: %x; Type: %x", mi.BaseAddress, mi.AllocationBase,
                mi.Protect, mi.Type);
            OutputDebugStringA(dbg);

            // AoB Scan this region
            PFSEARCH pfS; // Pattern search struct
            DWORD aobResult = FindPattern(aobPattern, &pfS, (LPVOID)lpAddr, (DWORD64)mi.RegionSize); // Perform search
            DWORD* btAOBAddr = (DWORD*)pfS.lpvResult;
            if ((unsigned long long)btAOBAddr != 0) {
                // @TODO: if address found is itself, break and retry 10 times. If still not found, return 0
                if ((unsigned long long)btAOBAddr != (unsigned long long)&aobPattern) {
                    return (unsigned long long)btAOBAddr;
                }

            }
        }
    }
    FreeLibraryAndExitThread(hInst, 0);
}

#undef _DXMD_DBG_BUFF_SIZE_