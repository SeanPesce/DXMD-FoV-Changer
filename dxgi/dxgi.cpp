// Author: Sean Pesce

#include "../include/DXMD_FoV_Changer.h"
#include "../include/PatternFind.h"
#include "../include/InjectAssembly.h"
#include "../include/sp/file.h"

float initFOV        = 0.0f;
float init_Hands_FOV = 0.0f;
const char *log_file = ".\\retail\\DXMD_FOV.log";
const char *cfg_file = ".\\retail\\DXMD_FOV.ini";

void LoadOriginalDll();
int InitSettings();
DWORD WINAPI FOVChangerThread(LPVOID param);

const char *lib_name = "dxgi";
#ifndef SP_WIN7_BUILD
#define DXGI_EXPORT_COUNT_ 20
LPCSTR mImportNames[DXGI_EXPORT_COUNT_] = { "ApplyCompatResolutionQuirking", "CompatString", "CompatValue", "CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2", "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice", "DXGID3D10ETWRundown", "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers", "DXGIDumpJournal", "DXGIGetDebugInterface1", "DXGIReportAdapterConfiguration", "DXGIRevertToSxS", "PIXBeginCapture", "PIXEndCapture", "PIXGetCaptureState", "SetAppCompatStringPointer", "UpdateHMDEmulationStatus" };
#else
#define DXGI_EXPORT_COUNT_ 50
LPCSTR mImportNames[DXGI_EXPORT_COUNT_] = { "CheckETWTLS", "CompatString", "CompatValue", "CreateDXGIFactory", "CreateDXGIFactory1", "D3DKMTCloseAdapter", "D3DKMTCreateAllocation", "D3DKMTCreateContext", "D3DKMTCreateDevice", "D3DKMTCreateSynchronizationObject", "D3DKMTDestroyAllocation", "D3DKMTDestroyContext", "D3DKMTDestroyDevice", "D3DKMTDestroySynchronizationObject", "D3DKMTEscape", "D3DKMTGetContextSchedulingPriority", "D3DKMTGetDeviceState", "D3DKMTGetDisplayModeList", "D3DKMTGetMultisampleMethodList", "D3DKMTGetRuntimeData", "D3DKMTGetSharedPrimaryHandle", "D3DKMTLock", "D3DKMTOpenAdapterFromHdc", "D3DKMTOpenResource", "D3DKMTPresent", "D3DKMTQueryAdapterInfo", "D3DKMTQueryAllocationResidency", "D3DKMTQueryResourceInfo", "D3DKMTRender", "D3DKMTSetAllocationPriority", "D3DKMTSetContextSchedulingPriority", "D3DKMTSetDisplayMode", "D3DKMTSetDisplayPrivateDriverFormat", "D3DKMTSetGammaRamp", "D3DKMTSetVidPnSourceOwner", "D3DKMTSignalSynchronizationObject", "D3DKMTUnlock", "D3DKMTWaitForSynchronizationObject", "D3DKMTWaitForVerticalBlankEvent", "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice", "DXGID3D10ETWRundown", "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers", "DXGIDumpJournal", "DXGIReportAdapterConfiguration", "DXGIRevertToSxS", "OpenAdapter10", "OpenAdapter10_2", "SetAppCompatStringPointer" };
#endif // SP_WIN7_BUILD

HINSTANCE mHinst = 0, mHinstDLL = 0;
extern "C" UINT_PTR mProcs[DXGI_EXPORT_COUNT_] = { 0 };


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    mHinst = hinstDLL;
    if (fdwReason == DLL_PROCESS_ATTACH) {
        sp::util::file::write_text(log_file, "Attached to process");
        InitSettings();
        sp::util::file::append_text(log_file, "Finished loading settings.");
        if (!mHinstDLL) {
            // No chain loaded; get original DLL from system directory
            LoadOriginalDll();
        }
        if (!mHinstDLL) {
            sp::util::file::append_text(log_file, "Failed to load original DLL. Exiting...");
            return FALSE;
        }
        sp::util::file::append_text(log_file, "Loading exported funcs...");
        for (int i = 0; i < DXGI_EXPORT_COUNT_; i++) {
            mProcs[i] = (UINT_PTR)GetProcAddress(mHinstDLL, mImportNames[i]);
        }
        lpvDXMDBase = NULL;
        dwDXMDSize = 0;
        sp::util::file::append_text(log_file, "Obtaining module base & size...");
        GetModuleSize(GetModuleHandle(NULL), &lpvDXMDBase, &dwDXMDSize); // Obtain DXMD base address & size
        dxmdStartAddr = (DWORD64*)lpvDXMDBase; // Starting address of Deus Ex: Mankind Divided (used as starting address for static pointers)
        btdxmdStartAddr = (BYTE*)lpvDXMDBase;

        sp::util::file::append_text(log_file, "Initializing keybinds & additional settings...");
        initKeybindsAndSettings();
        sp::util::file::append_text(log_file, "Finished reading keybinds & settings.");

        // Initialize thread(s)
        sp::util::file::append_text(log_file, "Initializing FoV changer thread...");
        hInitFOVChangerThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&FOVChangerThread, 0, 0, 0);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        sp::util::file::append_text(log_file, "Detaching from process.");
        FreeLibrary(mHinstDLL);
    }
    return TRUE;
}

#ifndef SP_WIN7_BUILD

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

#else

// Win7-specific exports
extern "C" void CheckETWTLS_wrapper();
extern "C" void D3DKMTCloseAdapter_wrapper();
extern "C" void D3DKMTCreateAllocation_wrapper();
extern "C" void D3DKMTCreateContext_wrapper();
extern "C" void D3DKMTCreateDevice_wrapper();
extern "C" void D3DKMTCreateSynchronizationObject_wrapper();
extern "C" void D3DKMTDestroyAllocation_wrapper();
extern "C" void D3DKMTDestroyContext_wrapper();
extern "C" void D3DKMTDestroyDevice_wrapper();
extern "C" void D3DKMTDestroySynchronizationObject_wrapper();
extern "C" void D3DKMTEscape_wrapper();
extern "C" void D3DKMTGetContextSchedulingPriority_wrapper();
extern "C" void D3DKMTGetDeviceState_wrapper();
extern "C" void D3DKMTGetDisplayModeList_wrapper();
extern "C" void D3DKMTGetMultisampleMethodList_wrapper();
extern "C" void D3DKMTGetRuntimeData_wrapper();
extern "C" void D3DKMTGetSharedPrimaryHandle_wrapper();
extern "C" void D3DKMTLock_wrapper();
extern "C" void D3DKMTOpenAdapterFromHdc_wrapper();
extern "C" void D3DKMTOpenResource_wrapper();
extern "C" void D3DKMTPresent_wrapper();
extern "C" void D3DKMTQueryAdapterInfo_wrapper();
extern "C" void D3DKMTQueryAllocationResidency_wrapper();
extern "C" void D3DKMTQueryResourceInfo_wrapper();
extern "C" void D3DKMTRender_wrapper();
extern "C" void D3DKMTSetAllocationPriority_wrapper();
extern "C" void D3DKMTSetContextSchedulingPriority_wrapper();
extern "C" void D3DKMTSetDisplayMode_wrapper();
extern "C" void D3DKMTSetDisplayPrivateDriverFormat_wrapper();
extern "C" void D3DKMTSetGammaRamp_wrapper();
extern "C" void D3DKMTSetVidPnSourceOwner_wrapper();
extern "C" void D3DKMTSignalSynchronizationObject_wrapper();
extern "C" void D3DKMTUnlock_wrapper();
extern "C" void D3DKMTWaitForSynchronizationObject_wrapper();
extern "C" void D3DKMTWaitForVerticalBlankEvent_wrapper();
extern "C" void OpenAdapter10_wrapper();
extern "C" void OpenAdapter10_2_wrapper();

#endif // SP_WIN7_BUILD

// Loads the original DLL from the default system directory
//    (Original function by Michael Koch)
void LoadOriginalDll()
{
    sp::util::file::append_text(log_file, "Loading original DLL...");
    char buffer[MAX_PATH];

    // Get path to system directory
    sp::util::file::append_text(log_file, "    Getting system directory...");
    GetSystemDirectory(buffer, MAX_PATH);

    // Append DLL name
    strcat_s(buffer, (std::string("\\") + lib_name + ".dll").c_str());
    sp::util::file::append_text(log_file, std::string("    Sys dir: ") + buffer);

    // Try to load the system DLL, if pointer empty
    if (!mHinstDLL) mHinstDLL = LoadLibrary(buffer);

    // Debug
    if (!mHinstDLL) {
        sp::util::file::append_text(log_file, "Failed to load original DLL. Exiting...");
        ExitProcess(0); // Exit
    }
    sp::util::file::append_text(log_file, "    Loaded original DLL.");
}


// Parses DXMD_FOV.ini for intialization settings
int InitSettings()
{
    sp::util::file::append_text(log_file, "Initializing DLL settings...");
    char dll_chain_buffer[128];
    // Check settings file for DLL chain
    sp::util::file::append_text(log_file, "DLL chain:");
    GetPrivateProfileString("dxgi", "DLL_Chain", NULL, dll_chain_buffer, 128, cfg_file);

    if (dll_chain_buffer[0] != '\0') { // Found DLL_Chain entry in settings file
        sp::util::file::append_text(log_file, std::string("    \"") + dll_chain_buffer + std::string("\""));
        mHinstDLL = LoadLibrary(dll_chain_buffer);
        if (!mHinstDLL) {
            // Failed to load wrapper DLL
            sp::util::file::append_text(log_file, "Failed to load chain.");
            return 2; // Return 2 if given DLL could not be loaded
        }
    } else {
        sp::util::file::append_text(log_file, "    No chain specified.");
        return 1; // Return 1 if config file or DLL_Chain entry could not be located
    }
    return 0; // Return 0 on success
}

/**
 *  Determine keybinds and global configuration settings for the plugin
 */
void initKeybindsAndSettings()
{
    // Determine whether or not to play beeps when enabling/disabling mods:
    char onOff[2];
    sp::util::file::append_text(log_file, "    Checking beep setting...");
    GetPrivateProfileString("Sounds", "Enable", "0", onOff, 2, cfg_file);
    std::stringstream hsSounds;
    hsSounds << std::hex << onOff;
    hsSounds >> enableBeeps;
    sp::util::file::append_text(log_file, std::string("    Beeps enabled? ") + std::to_string(enableBeeps));


    if (enableBeeps != 1 && enableBeeps != 0) {
        // If beep value is not valid, set to default (0, disabled)
        enableBeeps = 0;
    }
    if (enableBeeps == 1) {
        Beep(523, 200);
    }
    // Get "Increase FoV" keybind
    char keybind[3];
    sp::util::file::append_text(log_file, "    Getting \"Increase FoV\" keybind...");
    GetPrivateProfileString("FOV", "IncreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsIncreaseFOV;
    hsIncreaseFOV << std::hex << keybind;
    hsIncreaseFOV >> FOV_Up_Key;
    // Get "Decrease FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Decrease FoV\" keybind...");
    GetPrivateProfileString("FOV", "DecreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsDecreaseFOV;
    hsDecreaseFOV << std::hex << keybind;
    hsDecreaseFOV >> FOV_Down_Key;
    // Get "Increase First Person Hands FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Increase hands FoV\" keybind...");
    GetPrivateProfileString("FOV", "IncreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsIncreaseHandsFOV;
    hsIncreaseHandsFOV << std::hex << keybind;
    hsIncreaseHandsFOV >> Hands_FOV_Up_Key;
    // Get "Decrease First Person Hands FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Decrease hands FoV\" keybind...");
    GetPrivateProfileString("FOV", "DecreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsDecreaseHandsFOV;
    hsDecreaseHandsFOV << std::hex << keybind;
    hsDecreaseHandsFOV >> Hands_FOV_Down_Key;
    // Get "Restore Player's Given Default FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Restore preferred FoV\" keybind...");
    GetPrivateProfileString("FOV", "RestorePreferredFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsRestorePreferredFOV;
    hsRestorePreferredFOV << std::hex << keybind;
    hsRestorePreferredFOV >> Restore_Preferred_FOV_Key;
    // Get "Restore game-supported FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Restore game-supported FoV\" keybind...");
    GetPrivateProfileString("FOV", "RestoreGameSupportedFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream hsResetFOV;
    hsResetFOV << std::hex << keybind;
    hsResetFOV >> Reset_FOV_Key;
    // Get Default FoV
    sp::util::file::append_text(log_file, "    Getting \"Default FoV\" pref...");
    initFOV = (float)GetPrivateProfileInt("FOV", "DefaultFOV", 90, cfg_file);
    // Get Default Hands FoV
    sp::util::file::append_text(log_file, "    Getting \"Default hands FoV\" pref...");
    init_Hands_FOV = (float)(GetPrivateProfileInt("FOV", "DefaultHandsFOV", 70, cfg_file));
    if (init_Hands_FOV < 5.0f) {
        sp::util::file::append_text(log_file, "Invalid hands FoV. Setting minimum...");
        init_Hands_FOV = 0.1f;
    }
}


void initFOVModifiers()
{
    fp_FOV_modifier_Instruction = 0;
    sp::util::file::append_text(log_file, "Initializing FoV modifiers...");

    // Find Address of main first person FoV modifier (normally 1.0f max)
    // AoB Scan
    sp::util::file::append_text(log_file, "    Scanning for FoV modifier instruction...");
    PFSEARCH pfS_FOV_Mod_Instr; // Pattern search struct
    while (fp_FOV_modifier_Instruction == 0) {
        FindPattern("F3 0F 59 40 38 48 8D 44 24 60 48 0F 46 C1 F3 44 0F 59 00 F3 41 0F 58 C0", &pfS_FOV_Mod_Instr, lpvDXMDBase, dwDXMDSize);
        fp_FOV_modifier_Instruction = (DWORD64)(BYTE*)pfS_FOV_Mod_Instr.lpvResult;
    }

    // Inject code to access FoV modifier:
    sp::util::file::append_text(log_file, "    Injecting FoV changer code...");
    injectFOVChanger((BYTE*)fp_FOV_modifier_Instruction);
    
    // Find address of instruction that modifies first person hands
    // AoB Scan
    PFSEARCH pfS_hands_FOV_Instr;
    sp::util::file::append_text(log_file, "    Scanning for hands FoV modifier instruction...");
    FindPattern("F3 0F 59 0D ? ? ? ? FF 90 F0 01 00 00", &pfS_hands_FOV_Instr, lpvDXMDBase, dwDXMDSize);
    fp_Hands_FOV_modifier_Instruction = (DWORD64)(BYTE*)pfS_hands_FOV_Instr.lpvResult;

    sp::util::file::append_text(log_file, "    Injecting hands FoV changer code...");
    injectHandFOVChanger((BYTE*)fp_Hands_FOV_modifier_Instruction);
    
    // Set starting FoV based on player's preferred FoV
    sp::util::file::append_text(log_file, "Setting initial FoV...");
    *(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
    if (*(float*)ptr_FOV_Modifier > 2.345f) {
        *(float*)ptr_FOV_Modifier = 2.345f;
    } else if (*(float*)ptr_FOV_Modifier < 0.1f) {
        *(float*)ptr_FOV_Modifier = 0.1f;
    }
    // Correct initFOV if given value wasn't valid
    initFOV = (*(float*)ptr_FOV_Modifier * default_Hands_FOV) / default_FOV_Modifier;
    
}


DWORD WINAPI FOVChangerThread(LPVOID param)
{
    bRunningFOVChanger = true;

    initFOVModifiers();

    sp::util::file::append_text(log_file, "Starting FoV modifier loop...");
    while (bRunningFOVChanger)
    {

        if (FOV_Up_Key != 0 && GetAsyncKeyState(FOV_Up_Key) & 1) {
            // Increase FoV
            if (*(float*)ptr_FOV_Modifier <= 2.345f) {
                *(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier + 0.1f;
                if (enableBeeps == 1) {
                    Beep(523, 100);
                }
            }
        } else if (FOV_Down_Key != 0 && GetAsyncKeyState(FOV_Down_Key) & 1) {
            // Decrease FoV (minimum = 0.1f)
            if (*(float*)ptr_FOV_Modifier >= 0.2f) {
                *(float*)ptr_FOV_Modifier = *(float*)ptr_FOV_Modifier - 0.1f;
                if (enableBeeps == 1) {
                    Beep(523, 100);
                }
            }
        } else if (Hands_FOV_Up_Key != 0 && GetAsyncKeyState(Hands_FOV_Up_Key) & 1) {
            // Increase Hands FoV
            current_Hands_FOV = current_Hands_FOV + 5;
            if (enableBeeps == 1) {
                Beep(523, 100);
            }
        } else if (Hands_FOV_Down_Key != 0 && GetAsyncKeyState(Hands_FOV_Down_Key) & 1) {
            // Decrease Hands FoV
            if (current_Hands_FOV >= 5.0f) {
                current_Hands_FOV = current_Hands_FOV - 5;
                if (enableBeeps == 1) {
                    Beep(523, 100);
                }
            }
        } else if (Restore_Preferred_FOV_Key != 0 && GetAsyncKeyState(Restore_Preferred_FOV_Key) & 1) {
            // Set both FoVs back to player's preferred defaults
            *(float*)ptr_FOV_Modifier = (default_FOV_Modifier * initFOV) / default_Hands_FOV;
            current_Hands_FOV = init_Hands_FOV;
            if (enableBeeps == 1) {
                Beep(523, 100);
            }
        } else if (Reset_FOV_Key != 0 && GetAsyncKeyState(Reset_FOV_Key) & 1) {
            // Set both FoVs back to default game-supported settings
            current_Hands_FOV = default_Hands_FOV;
            *(float*)ptr_FOV_Modifier = default_FOV_Modifier;
            if (enableBeeps == 1) {
                Beep(523, 100);
            }
        }

        // Adjust hands FoV
        *(float*)ptr_fp_Hands_FOV_Base_Modifier = current_Hands_FOV / (*(float*)ptr_FOV_Modifier);
        Sleep(5);
    }
    return 0;
}
