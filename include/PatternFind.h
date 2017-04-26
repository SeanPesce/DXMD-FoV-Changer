#pragma once

#include <Windows.h>

#define PFAPI __stdcall
#define MAX_PATTERN 255

#define PF_NONE 0
#define PF_INVALID 1
#define PF_NOT_FOUND 2
#define PF_OVERFLOW 3

struct PFSEARCH {
	DWORD dwLength;
	char szMask[MAX_PATTERN + 1];
	BYTE lpbData[MAX_PATTERN];
	LPVOID lpvResult;
};

BOOL GetModuleSize(HMODULE hModule, LPVOID* lplpBase, LPDWORD lpdwSize);
DWORD PFAPI FindPattern(char *szPattern, PFSEARCH *ppf, LPVOID lpvBase, DWORD dwSize);
int WINAPI AOBScanWalkRegions(char* aobPattern, bool useCESettings);