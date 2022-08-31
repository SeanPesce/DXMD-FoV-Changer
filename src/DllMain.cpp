// Author: Sean Pesce

#include "../include/DXMD_FoV_Changer.h"
#include "../include/InjectAssembly.h"


const char *cfg_file = ".\\DXMD_FoV.ini";
sp::io::ps_ostream debug;

// Configurable settings
std::string log_file = ".\\DXMD_Mod.log";

void load_original_dll();
int get_dll_chain();
void init_settings();
HANDLE async_thread_handle = NULL;
DWORD WINAPI async_thread(LPVOID param);

const char *lib_name = "version";
#define DLL_EXPORT_COUNT_ 15
LPCSTR import_names[DLL_EXPORT_COUNT_] = { "GetFileVersionInfoA", "GetFileVersionInfoByHandle", "GetFileVersionInfoExW", "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW", "GetFileVersionInfoW", "VerFindFileA", "VerFindFileW", "VerInstallFileA", "VerInstallFileW", "VerLanguageNameA", "VerLanguageNameW", "VerQueryValueA", "VerQueryValueW" };

HINSTANCE dll_instance = NULL;
HINSTANCE dll_chain_instance = NULL;
extern "C" UINT_PTR export_locs[DLL_EXPORT_COUNT_] = { 0 };

extern std::string calculate_file_md5(std::string& fpath, size_t read_sz = 1048576 /* 1MB */);
extern BOOL get_module_size(HMODULE hmodule, LPVOID* lplp_base, PDWORD64 lpdw_size);

float init_fov = 0.0f;
float init_hands_fov = 0.0f;


BOOL WINAPI DllMain(HINSTANCE hinst_dll, DWORD fdw_reason, LPVOID lpv_reserved)
{
    dll_instance = hinst_dll;
    if (fdw_reason == DLL_PROCESS_ATTACH)
    {
        // Set working directory
        SetCurrentDirectory(sp::env::lib_dir().c_str());

        // Check that config file exists
        std::ifstream cfg_check(cfg_file);
        if (!cfg_check.good())
        {
            std::string err_msg = "Failed to find mod configuration file:\n" + std::string(cfg_file) + "\n\nCurrent directory:\n";
            err_msg += std::filesystem::current_path().string();
            err_msg += "\n\nPlease report this issue to the developer:\nhttps://github.com/SeanPesce/DXMD-FoV-Changer/issues";
            MessageBox(NULL, err_msg.c_str(), "ERROR", MB_OK | MB_SETFOREGROUND | MB_TOPMOST | MB_APPLMODAL);
            ExitProcess(SP_ERR_FILE_NOT_FOUND);
        }

        char cfg_str[MAX_PATH];
        GetPrivateProfileString("DLL", "LogFile", log_file.c_str(), cfg_str, MAX_PATH, cfg_file);
        log_file = cfg_str;

        debug = sp::io::ps_ostream("DXMD Mod Debug");
        debug.set_log_file(log_file);
        if (GetPrivateProfileInt("DLL", "Debug", 0, cfg_file))
        {
            debug.start();
        }

        debug.print("Writing log to " + log_file + "\n");

        debug.print("\n+----------------------------+\r\n|    DXMD FoV Changer Mod    |\r\n|     Author: Sean Pesce     |\r\n+----------------------------+\r\nCompiled: " __DATE__ "  " __TIME__ "\r\n\r\n");

        // Get current date/time
        struct tm time_local;
        time_t time_now = std::time(NULL);
        errno_t time_result = localtime_s(&time_local, &time_now);
        std::ostringstream oss;
        oss << std::put_time(&time_local, "%Y-%m-%d %H:%M:%S");
        std::string time_str = oss.str();
        debug.print("Attached to process at " + time_str + "\n");

        dxmd_base = NULL;
        dxmd_size = 0;
        debug.print("Obtaining module base & size...\n");
        get_module_size(GetModuleHandle(NULL), (LPVOID*)&dxmd_base, &dxmd_size); // Obtain DXMD.exe base address & size
        debug.print("    Base: " + std::to_string((unsigned long long)dxmd_base) + "\n");
        debug.print("    Size: " + std::to_string(dxmd_size) + "\n");
        get_dll_chain();

        debug.print("Finished loading settings.\n");
        if (!dll_chain_instance)
        {
            // No chain loaded; get original DLL from system directory
            load_original_dll();
        }
        if (!dll_chain_instance)
        {
            debug.print("Failed to load original DLL. Exiting...\n");
            MessageBox(NULL, ("Failed to load original " + std::string(lib_name) + ".dll").c_str(), "ERROR", MB_OK);
            return FALSE;
        }
        debug.print("Loading exported funcs...\n");
        for (int i = 0; i < DLL_EXPORT_COUNT_; i++)
        {
            export_locs[i] = (UINT_PTR)GetProcAddress(dll_chain_instance, import_names[i]);
            std::stringstream strstr;
            strstr << std::hex << export_locs[i];
            debug.print("    " + std::string(import_names[i]) + " @ " + strstr.str() + "\n");
        }

        debug.print("Initializing keybinds & additional settings...\n");
        init_settings();
        debug.print("Finished reading keybinds & settings.\n");

        // Initialize thread(s)
        debug.print("Initializing async thread...\n");
        async_thread_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&async_thread, 0, 0, 0);
    }
    else if (fdw_reason == DLL_PROCESS_DETACH)
    {
        debug.print("Detaching from process.\n");
        FreeLibrary(dll_chain_instance);
    }
    return TRUE;
}


extern "C" void GetFileVersionInfoA_wrapper();
extern "C" void GetFileVersionInfoByHandle_wrapper();
extern "C" void GetFileVersionInfoExW_wrapper();
extern "C" void GetFileVersionInfoSizeA_wrapper();
extern "C" void GetFileVersionInfoSizeExW_wrapper();
extern "C" void GetFileVersionInfoSizeW_wrapper();
extern "C" void GetFileVersionInfoW_wrapper();
extern "C" void VerFindFileA_wrapper();
extern "C" void VerFindFileW_wrapper();
extern "C" void VerInstallFileA_wrapper();
extern "C" void VerInstallFileW_wrapper();
extern "C" void VerLanguageNameA_wrapper();
extern "C" void VerLanguageNameW_wrapper();
extern "C" void VerQueryValueA_wrapper();
extern "C" void VerQueryValueW_wrapper();


// Loads the original DLL from the default system directory
void load_original_dll()
{
    debug.print("Loading original DLL...\n");
    char buffer[MAX_PATH];

    // Get path to system directory
    debug.print("    Getting system directory...\n");
    GetSystemDirectory(buffer, MAX_PATH);

    // Append DLL name
    strcat_s(buffer, (std::string("\\") + lib_name + ".dll").c_str());
    debug.print(std::string("    Sys dir: ") + buffer + "\n");

    // Try to load the system DLL, if pointer empty
    if (!dll_chain_instance) dll_chain_instance = LoadLibrary(buffer);

    // Debug
    if (!dll_chain_instance)
    {
        debug.print("Failed to load original DLL. Exiting...\n");
        MessageBox(NULL, ("Failed to load original " + std::string(lib_name) + ".dll").c_str(), "ERROR", MB_OK);
        ExitProcess(SP_ERR_FILE_NOT_FOUND); // Exit
    }
    debug.print("    Loaded original DLL.\n");
}


// Parses configuration file for DLL intialization settings
int get_dll_chain()
{
    debug.print("Initializing DLL settings...\n");
    char dll_chain_buffer[MAX_PATH];
    // Check settings file for DLL chain
    debug.print("DLL chain:\n");
    GetPrivateProfileString("DLL", "Chain", NULL, dll_chain_buffer, MAX_PATH, cfg_file);

    if (dll_chain_buffer[0] != '\0')
    {
        // Found DLL_Chain entry in settings file
        debug.print(std::string("    \"") + dll_chain_buffer + std::string("\"\n"));
        dll_chain_instance = LoadLibrary(dll_chain_buffer);
        if (!dll_chain_instance)
        {
            // Failed to load wrapper DLL
            debug.print("Failed to load chain.\n");
            return 2; // Return 2 if given DLL could not be loaded
        }
    }
    else
    {
        debug.print("    No chain specified.\n");
        return 1; // Return 1 if config file or DLL_Chain entry could not be located
    }
    return 0; // Success
}

/**
 *  Determine keybinds and global configuration settings for the plugin
 */
void init_settings()
{
    // Log game executable MD5 for debugging issues
    std::string exe_md5 = calculate_file_md5(sp::env::exe_path());
    debug.print(sp::env::exe_name() + " MD5: " + exe_md5 + "\n");

    // Crowd-sourced hashes:
    debug.print("Game version: ");
    if (exe_md5 == "a227bc2145d592f0f945df9b882f96d8")
    {
        debug.print("DXMD v1.19-801.0 Steam\n");
    }
    else if (exe_md5 == "3745fa30bf3f607a58775f818c5e0ac0")
    {
        debug.print("DXMD v1.19-801.0 GoG\n");
    }
    else if (exe_md5 == "c1a85abd61e3d31db179801a27f56e12")
    {
        debug.print("DXMD v1.19-801.0 (unknown platform)\n");
    }
    else if (exe_md5 == "a47cbe45e694dd57ec0d141fd7854589") 
    {
        debug.print("Breach v1.15-758.0 Steam\n");
    }
    else
    {
        debug.print("Unknown\n");
    }

    // Determine whether or not to play beeps when enabling/disabling mods:
    enable_beeps = GetPrivateProfileInt("Sounds", "Enable", 0, cfg_file);
    debug.print("Beeps enabled? " + std::to_string(enable_beeps) + "\n");

    if (enable_beeps != 1 && enable_beeps != 0)
    {
        // If beep value is not valid, set to default (0, disabled)
        enable_beeps = 0;
    }
    if (enable_beeps == 1)
    {
        Beep(523, 200);
    }

    std::stringstream sstream;
    char keybind[3];

    // "Increase FoV" keybind
    GetPrivateProfileString("FOV", "IncreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> fov_up_key;
    sstream.str("");
    sstream.clear();
    debug.print("IncreaseFOV_Keybind=" + std::to_string(fov_up_key) + "\n");

    // "Decrease FoV" keybind
    GetPrivateProfileString("FOV", "DecreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> fov_down_key;
    sstream.str("");
    sstream.clear();
    debug.print("DecreaseFOV_Keybind=" + std::to_string(fov_down_key) + "\n");

    // "Increase First Person Hands FoV" keybind
    GetPrivateProfileString("FOV", "IncreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> hands_fov_up_key;
    sstream.str("");
    sstream.clear();
    debug.print("IncreaseHandsFOV_Keybind=" + std::to_string(hands_fov_up_key) + "\n");

    // "Decrease First Person Hands FoV" keybind
    GetPrivateProfileString("FOV", "DecreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> hands_fov_down_key;
    sstream.str("");
    sstream.clear();
    debug.print("DecreaseHandsFOV_Keybind=" + std::to_string(hands_fov_down_key) + "\n");

    // "Restore Player's Given Default FoV" keybind
    GetPrivateProfileString("FOV", "RestorePreferredFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> restore_preferred_fov_key;
    sstream.str("");
    sstream.clear();
    debug.print("RestorePreferredFOV_Keybind=" + std::to_string(restore_preferred_fov_key) + "\n");

    // "Restore game-supported FoV" keybind
    GetPrivateProfileString("FOV", "RestoreGameSupportedFOV_Keybind", "0", keybind, 3, cfg_file);
    sstream << std::hex << keybind;
    sstream >> reset_fov_key;
    sstream.str("");
    sstream.clear();
    debug.print("RestoreGameSupportedFOV_Keybind=" + std::to_string(reset_fov_key) + "\n");

    // Default FoV value
    init_fov = (float)GetPrivateProfileInt("FOV", "DefaultFOV", 90, cfg_file);
    debug.print("Default FoV=" + std::to_string(init_fov) + "\n");

    // Default hands FoV value
    init_hands_fov = (float)GetPrivateProfileInt("FOV", "DefaultHandsFOV", 70, cfg_file);
    if (init_hands_fov < 5.0f)
    {
        debug.print("Invalid hands FoV. Setting minimum...\n");
        init_hands_fov = 5.0f;
    }
    debug.print("Default Hands FoV=" + std::to_string(init_hands_fov) + "\n");
}


void init_modifiers()
{
    fp_fov_modifier_instruction = 0;
    debug.print("Initializing FoV modifiers...\n");

    // Find Address of main first person FoV modifier (normally 1.0f max)
    // AoB Scan
    debug.print("    Scanning for FoV modifier instruction...\n");
    PFSEARCH fov_mod_search_res; // Pattern search struct
    while (fp_fov_modifier_instruction == 0)
    {
        FindPattern("F3 0F 59 40 38 48 8D 44 24 60 48 0F 46 C1 F3 44 0F 59 00 F3 41 0F 58 C0", &fov_mod_search_res, dxmd_base, dxmd_size);
        fp_fov_modifier_instruction = (DWORD64)(BYTE*)fov_mod_search_res.lpvResult;
    }

    // Inject code to access FoV modifier:
    debug.print("    Injecting FoV changer code...\n");
    inject_fov_changer((BYTE*)fp_fov_modifier_instruction);

    // Find address of instruction that modifies first person hands
    // AoB Scan
    PFSEARCH hands_fov_mod_search_res;
    debug.print("    Scanning for hands FoV modifier instruction...\n");
    FindPattern("F3 0F 59 0D ? ? ? ? FF 90 F0 01 00 00", &hands_fov_mod_search_res, dxmd_base, dxmd_size);
    fp_hands_fov_modifier_instruction = (DWORD64)(BYTE*)hands_fov_mod_search_res.lpvResult;

    debug.print("    Injecting hands FoV changer code...\n");
    inject_hand_fov_changer((BYTE*)fp_hands_fov_modifier_instruction);

    // Set starting FoV based on player's preferred FoV
    debug.print("Setting initial FoV...\n");
    *(float*)ptr_fov_modifier = (default_fov_modifier * init_fov) / default_hands_fov;
    if (*(float*)ptr_fov_modifier > 2.345f)
    {
        *(float*)ptr_fov_modifier = 2.345f;
    }
    else if (*(float*)ptr_fov_modifier < 0.1f)
    {
        *(float*)ptr_fov_modifier = 0.1f;
    }
    // Correct init_fov if given value wasn't valid
    init_fov = (*(float*)ptr_fov_modifier * default_hands_fov) / default_fov_modifier;
}


DWORD WINAPI async_thread(LPVOID param)
{
    running = true;
    init_modifiers();

    debug.print("Starting asynchronous FoV modifier loop...\n");
    while (running)
    {

        if (fov_up_key != 0 && GetAsyncKeyState(fov_up_key) & 1) {
            // Increase FoV
            if (*(float*)ptr_fov_modifier <= 2.345f) {
                *(float*)ptr_fov_modifier = *(float*)ptr_fov_modifier + 0.1f;
                if (enable_beeps == 1) {
                    Beep(523, 100);
                }
            }
        }
        else if (fov_down_key != 0 && GetAsyncKeyState(fov_down_key) & 1) {
            // Decrease FoV (minimum = 0.1f)
            if (*(float*)ptr_fov_modifier >= 0.2f) {
                *(float*)ptr_fov_modifier = *(float*)ptr_fov_modifier - 0.1f;
                if (enable_beeps == 1) {
                    Beep(523, 100);
                }
            }
        }
        else if (hands_fov_up_key != 0 && GetAsyncKeyState(hands_fov_up_key) & 1) {
            // Increase Hands FoV
            current_hands_fov = current_hands_fov + 5;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        }
        else if (hands_fov_down_key != 0 && GetAsyncKeyState(hands_fov_down_key) & 1) {
            // Decrease Hands FoV
            if (current_hands_fov >= 5.0f) {
                current_hands_fov = current_hands_fov - 5;
                if (enable_beeps == 1) {
                    Beep(523, 100);
                }
            }
        }
        else if (restore_preferred_fov_key != 0 && GetAsyncKeyState(restore_preferred_fov_key) & 1) {
            // Set both FoVs back to player's preferred defaults
            *(float*)ptr_fov_modifier = (default_fov_modifier * init_fov) / default_hands_fov;
            current_hands_fov = init_hands_fov;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        }
        else if (reset_fov_key != 0 && GetAsyncKeyState(reset_fov_key) & 1) {
            // Set both FoVs back to default game-supported settings
            current_hands_fov = default_hands_fov;
            *(float*)ptr_fov_modifier = default_fov_modifier;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        }

        // Adjust hands FoV
        *(float*)ptr_fp_hands_fov_base_modifier = current_hands_fov / (*(float*)ptr_fov_modifier);
        Sleep(20);
    }
    return 0;
}

// This function was developed based on code from Franc[e]sco (from ccplz.net)
BOOL get_module_size(HMODULE hmodule, LPVOID* lplp_base, PDWORD64 lpdw_size)
{
    if (hmodule == GetModuleHandle(NULL))
    {
        PIMAGE_NT_HEADERS img_nt_hdrs = ImageNtHeader((PVOID)hmodule);

        if (img_nt_hdrs == NULL)
        {
            return FALSE;
        }

        *lplp_base = (LPVOID)hmodule;
        *lpdw_size = img_nt_hdrs->OptionalHeader.SizeOfImage;
    }
    else
    {
        MODULEINFO  module_info;

        if (!GetModuleInformation(GetCurrentProcess(), hmodule, &module_info, sizeof(MODULEINFO)))
        {
            return FALSE;
        }

        *lplp_base = module_info.lpBaseOfDll;
        *lpdw_size = module_info.SizeOfImage;
    }
    return TRUE;
}


std::string calculate_file_md5(std::string& fpath, size_t read_sz)
{
    MD5 md5;
    uint8_t* buf = (uint8_t*)CoTaskMemAlloc(read_sz);
    if (!buf)
    {
        // Memory allocation failed
        MessageBox(NULL, "Failed to allocate memory for MD5 ingress buffer.", "ERROR", MB_OK);
        ExitProcess(SP_ERR_INSUFFICIENT_BUFFER);
    }

    std::fstream f;
    f.open(fpath, std::ios::in | std::ios::binary);

    while (true)
    {
        f.read((char*)buf, read_sz);
        size_t nbytes = f.gcount();
        if (!nbytes)
        {
            break;
        }

        md5.add((const void*)buf, nbytes);

        if (f.eof())
        {
            break;
        }
    }

    CoTaskMemFree(buf);
    return md5.getHash();
}
