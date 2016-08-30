#pragma once

void ExitInstance(void);

#pragma data_seg (".d3d9_shared")
HINSTANCE           hOriginalDll;
HINSTANCE           hThisInstance;
#pragma data_seg ()

HANDLE hInitGameDataThread;
HANDLE hInitFOVChangerThread;
HANDLE hProcess;

BOOL bRunningGameData;
BOOL bRunningFOVChanger;

DWORD64 dwBase;

LPVOID lpvDXMDBase; //Deus Ex: Mankind Divided Base Address
DWORD dwDXMDSize; //Deus Ex: Mankind Divided base memory size
int* dxmdStartAddr;
BYTE* btdxmdStartAddr;

//Mod Settings
int enableBeeps = false;

//Keybinds
int FOV_Up_Key = 0;
int FOV_Down_Key = 0;
int Hands_FOV_Up_Key = 0;
int Hands_FOV_Down_Key = 0;
int Reset_FOV_Key = 0;

//Space for manual ASM (hex bytes) code caves
//
//								   
uint8_t fp_FOV_modifier_Jump[24] = { 0xFF, 0x25, 0x04, 0x00, 0x00, 0x00, //[0:5] jmp QWORD PTR [rip+0x04]   aka    jmp fp_FOV_modifier_CodeCave[0]
										0x90, 0x90, 0x90, 0x90, //[6:9] Note: the 0x90 (nop)bytes are just a failsafe giving us extra space to work with and stop any nearby bytes from making the code confusing
										0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, //[10:17] These bytes will be overwritten with &fp_FOV_modifier_CodeCave[0]
										0x90, 0x90, 0x90, 0x90, 0x90, 0x90};  //[18:23] removing the end of the overwritten instruction(s)
//
//
uint8_t fp_FOV_modifier_CodeCave[84] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, //[0:15] Note: the 0x90 (nop) bytes are just a failsafe giving us extra space to work with and stop any nearby bytes from making the code confusing
											//0xF3, 0x48, 0x0F, 0x59, 0x05, 0x1F, 0x00, 0x00, 0x00, //[16:24] rex.W mulss xmm0,DWORD PTR [rip+0x1F]   aka   mulss xmm0, qword ptr[*ptr_FOV_Modifier]
											0xF3, 0x0F, 0x59, 0x05, 0x1C, 0x00, 0x00, 0x00, 0x90, //[16:24] mulss  xmm0,DWORD PTR [rip+0x1C]   aka   mulss xmm0, qword ptr[*ptr_FOV_Modifier]
											0x48, 0x8D, 0x44, 0x24, 0x60, //[25:29] lea rax,[rsp+60]
											0x48, 0x0F, 0x46, 0xC1, //[30:33] cmovbe rax,rcx
											0xF3, 0x44, 0x0F, 0x59, 0x00, //[34:38] mulss xmm8,[rax]
											0xF3, 0x41, 0x0F, 0x58, 0xC0, //[39:43] addss xmm0,xmm8
											0xFF, 0x25, 0x16, 0x00, 0x00, 0x00,  //[39:44] jmp QWORD PTR [rip+0x16]   aka    jmp fp_FOV_modifier_Instruction+5
											0x90, 0x90,	//[45:51] more extra space to work with
											0x00, 0x00, 0xA0, 0x3F, 0x00, 0x00, 0x00, 0x3F, //[52:59] These are the original bytes that are accessed for fp FOV (fp FOV = [52])
											0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, //[60:67] Continuation of the original bytes that are accessed for fp FOV
											0x90, 0x90, 0x90, 0x90, 0x90, 0x90,  //[68:73] more extra space to work with
											0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, //[74:81] These bytes will be overwritten with fp_FOV_modifier_Instruction+5 (aka the jumpback address)
											0x90, 0x90 };   //[82:83] more extra space to work with

uint8_t fp_Hands_FOV_modifier_Jump[22] = { 0xFF, 0x25, 0x04, 0x00, 0x00, 0x00, //[0:5] jmp QWORD PTR [rip+0x04]  , where [rip+0x04] = [10:17]
										0x90, 0x90, 0x90, 0x90, //[6:9] Note: the 0x90 (nop)bytes are just a failsafe giving us extra space to work with and stop any nearby bytes from making the code confusing
										0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, //[10:17] These bytes will be overwritten with &fp_Hands_FOV_modifier_CodeCave[0]
										0x90, 0x90, 0x90, 0x90 };  //[18:21] removing the end of the overwritten instruction(s)

uint8_t fp_FOV_Hands_modifier_CodeCave[116] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, //[0:15] Note: the 0x90 (nop) bytes are just a failsafe giving us extra space to work with and stop any nearby bytes from making the code confusing
												0x57,    //[16] push rdi
												0x48, 0xBF, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //[17:26] mov rdi,5
												0x48, 0x3B, 0xBB, 0xDC, 0x01, 0x00, 0x00, //[27:33] cmp rdi,[rbx+000001DC]
												0x5F, //[34] pop rdi
												0x74, 0xA, //[35:36] je [rip+0xA]
												0xF3, 0x0F, 0x59, 0x0D, 0x23, 0x00, 0x00, 0x00, //[37:44] mulss  xmm1,DWORD PTR [rip+0x23]  , where [rip+0x23] = [80:87]
												0xEB, 0x8, //[45:46]  jmp [rip+8]
												0xF3, 0x0F, 0x59, 0x0D, 0x25, 0x00, 0x00, 0x00, //[47:54] mulss  xmm1,DWORD PTR [rip+0x25]  , where [rip+0x25] = [92:99] 
												0xFF, 0x90, 0xF0, 0x01, 0x00, 0x00,	//[55:60] call qword ptr [rax+000001F0]
												0x48, 0x8B, 0x03,	//[61:63] mov rax,[rbx]
												0xB2, 0x01, //[64:65] mov dl,01 
												0x48, 0x89, 0xD9, //[66:68] mov rcx,rbx
												0xFF, 0x25, 0x1D, 0x00, 0x00, 0x00,  //[69:74] jmp QWORD PTR [rip+0x1D]   , where [rip+0x1D] = [104:111] 
												0x90, 0x90, 0x90, 0x90, 0x90, //[75:79] more extra space to work with
												0xE0, 0x2E, 0x65, 0x42, 0x35, 0xFA, 0x8E, 0x3C, //[80:87] //Default FOV Base modifier (57.29577637)
												0x90, 0x90, 0x90, 0x90, //[88:91] more extra space to work with
												0xE0, 0x2E, 0x65, 0x42, 0x35, 0xFA, 0x8E, 0x3C, //[92:99] //Dynamic FOV Base modifier (controlled by player)
												0x90, 0x90, 0x90, 0x90, //[100:103] more extra space to work with
												0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, //[104:111] jump back address will be stored here
												0x90, 0x90, 0x90, 0x90 //[112:115] more extra space to work with
												};

//FOV Values
float default_FOV_Base_Modifier = 57.29577637;
//float default_FOV_Base_Modifier2 = 1.189791016;
float default_FOV_Modifier = 1.25;
float default_Hands_FOV = 68.17;
DWORD64* ptr_FOV_Modifier = (DWORD64*)((int)(&fp_FOV_modifier_CodeCave[52]));
DWORD64* ptr_fp_Hands_FOV_Base_Modifier = (DWORD64*)((int)(&fp_FOV_Hands_modifier_CodeCave[92]));
float current_Hands_FOV = default_Hands_FOV;

//Addresses for lines of ASM code that will be changed (or have additional code injected at its location)
DWORD64 fp_FOV_modifier_Instruction = 0;
DWORD64 fp_Hands_FOV_modifier_Instruction = 0;