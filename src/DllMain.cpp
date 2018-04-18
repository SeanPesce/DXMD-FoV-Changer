// Author: Sean Pesce

#include "../include/DXMD_FoV_Changer.h"
#include "../include/PatternFind.h"
#include "../include/InjectAssembly.h"
#include "../include/sp/file.h"

float init_fov       = 0.0f;
float init_hands_fov = 0.0f;
const char *log_file = ".\\retail\\DXMD_FOV.log";
const char *cfg_file = ".\\retail\\DXMD_FOV.ini";

void load_original_dll();
int get_dll_chain();
DWORD WINAPI fov_changer_thread(LPVOID param);

const char *lib_name = "version";
#define DLL_EXPORT_COUNT_ 15
LPCSTR import_names[DLL_EXPORT_COUNT_] = { "GetFileVersionInfoA", "GetFileVersionInfoByHandle", "GetFileVersionInfoExW", "GetFileVersionInfoSizeA", "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW", "GetFileVersionInfoW", "VerFindFileA", "VerFindFileW", "VerInstallFileA", "VerInstallFileW", "VerLanguageNameA", "VerLanguageNameW", "VerQueryValueA", "VerQueryValueW" };

HINSTANCE dll_instance = 0, dll_chain_instance = 0;
extern "C" UINT_PTR export_locs[DLL_EXPORT_COUNT_] = { 0 };


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    dll_instance = hinstDLL;
    if (fdwReason == DLL_PROCESS_ATTACH) {
        sp::util::file::write_text(log_file, "+--------------------+\r\n|    DXMD FoV Mod    |\r\n| Author: Sean Pesce |\r\n+--------------------+\r\nCompiled: " __DATE__ "  " __TIME__ "\r\n\r\nAttached to process.");
        get_dll_chain();
        sp::util::file::append_text(log_file, "Finished loading settings.");
        if (!dll_chain_instance) {
            // No chain loaded; get original DLL from system directory
            load_original_dll();
        }
        if (!dll_chain_instance) {
            sp::util::file::append_text(log_file, "Failed to load original DLL. Exiting...");
            return FALSE;
        }
        sp::util::file::append_text(log_file, "Loading exported funcs...");
        for (int i = 0; i < DLL_EXPORT_COUNT_; i++) {
            export_locs[i] = (UINT_PTR)GetProcAddress(dll_chain_instance, import_names[i]);
        }
        dxmd_base = NULL;
        dxmd_size = 0;
        sp::util::file::append_text(log_file, "Obtaining module base & size...");
        GetModuleSize(GetModuleHandle(NULL), &dxmd_base, &dxmd_size); // Obtain DXMD base address & size

        sp::util::file::append_text(log_file, "Initializing keybinds & additional settings...");
        init_settings();
        sp::util::file::append_text(log_file, "Finished reading keybinds & settings.");

        // Initialize thread(s)
        sp::util::file::append_text(log_file, "Initializing FoV changer thread...");
        fov_changer_thread_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&fov_changer_thread, 0, 0, 0);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        sp::util::file::append_text(log_file, "Detaching from process.");
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
    sp::util::file::append_text(log_file, "Loading original DLL...");
    char buffer[MAX_PATH];

    // Get path to system directory
    sp::util::file::append_text(log_file, "    Getting system directory...");
    GetSystemDirectory(buffer, MAX_PATH);

    // Append DLL name
    strcat_s(buffer, (std::string("\\") + lib_name + ".dll").c_str());
    sp::util::file::append_text(log_file, std::string("    Sys dir: ") + buffer);

    // Try to load the system DLL, if pointer empty
    if (!dll_chain_instance) dll_chain_instance = LoadLibrary(buffer);

    // Debug
    if (!dll_chain_instance) {
        sp::util::file::append_text(log_file, "Failed to load original DLL. Exiting...");
        ExitProcess(0); // Exit
    }
    sp::util::file::append_text(log_file, "    Loaded original DLL.");
}


// Parses DXMD_FOV.ini for intialization settings
int get_dll_chain()
{
    sp::util::file::append_text(log_file, "Initializing DLL settings...");
    char dll_chain_buffer[128];
    // Check settings file for DLL chain
    sp::util::file::append_text(log_file, "DLL chain:");
    GetPrivateProfileString("DLL", "DLL_Chain", NULL, dll_chain_buffer, 128, cfg_file);

    if (dll_chain_buffer[0] != '\0') { // Found DLL_Chain entry in settings file
        sp::util::file::append_text(log_file, std::string("    \"") + dll_chain_buffer + std::string("\""));
        dll_chain_instance = LoadLibrary(dll_chain_buffer);
        if (!dll_chain_instance) {
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
void init_settings()
{
    // Determine whether or not to play beeps when enabling/disabling mods:
    char toggle[2];
    sp::util::file::append_text(log_file, "    Checking beep setting...");
    GetPrivateProfileString("Sounds", "Enable", "0", toggle, 2, cfg_file);
    std::stringstream beeps_stream;
    beeps_stream << std::hex << toggle;
    beeps_stream >> enable_beeps;
    sp::util::file::append_text(log_file, std::string("    Beeps enabled? ") + std::to_string(enable_beeps));

    if (enable_beeps != 1 && enable_beeps != 0) {
        // If beep value is not valid, set to default (0, disabled)
        enable_beeps = 0;
    }
    if (enable_beeps == 1) {
        Beep(523, 200);
    }
    // Get "Increase FoV" keybind
    char keybind[3];
    sp::util::file::append_text(log_file, "    Getting \"Increase FoV\" keybind...");
    GetPrivateProfileString("FOV", "IncreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream increase_fov_stream;
    increase_fov_stream << std::hex << keybind;
    increase_fov_stream >> fov_up_key;
    // Get "Decrease FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Decrease FoV\" keybind...");
    GetPrivateProfileString("FOV", "DecreaseFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream decrease_fov_stream;
    decrease_fov_stream << std::hex << keybind;
    decrease_fov_stream >> fov_down_key;
    // Get "Increase First Person Hands FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Increase hands FoV\" keybind...");
    GetPrivateProfileString("FOV", "IncreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream increase_hands_fov_stream;
    increase_hands_fov_stream << std::hex << keybind;
    increase_hands_fov_stream >> hands_fov_up_key;
    // Get "Decrease First Person Hands FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Decrease hands FoV\" keybind...");
    GetPrivateProfileString("FOV", "DecreaseHandsFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream decrease_hands_fov_stream;
    decrease_hands_fov_stream << std::hex << keybind;
    decrease_hands_fov_stream >> hands_fov_down_key;
    // Get "Restore Player's Given Default FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Restore preferred FoV\" keybind...");
    GetPrivateProfileString("FOV", "RestorePreferredFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream restore_preferred_fov_stream;
    restore_preferred_fov_stream << std::hex << keybind;
    restore_preferred_fov_stream >> restore_preferred_fov_key;
    // Get "Restore game-supported FoV" keybind
    sp::util::file::append_text(log_file, "    Getting \"Restore game-supported FoV\" keybind...");
    GetPrivateProfileString("FOV", "RestoreGameSupportedFOV_Keybind", "0", keybind, 3, cfg_file);
    std::stringstream reset_fov_stream;
    reset_fov_stream << std::hex << keybind;
    reset_fov_stream >> reset_fov_key;
    // Get Default FoV
    sp::util::file::append_text(log_file, "    Getting \"Default FoV\" pref...");
    init_fov = (float)GetPrivateProfileInt("FOV", "DefaultFOV", 90, cfg_file);
    // Get Default Hands FoV
    sp::util::file::append_text(log_file, "    Getting \"Default hands FoV\" pref...");
    init_hands_fov = (float)(GetPrivateProfileInt("FOV", "DefaultHandsFOV", 70, cfg_file));
    if (init_hands_fov < 5.0f) {
        sp::util::file::append_text(log_file, "Invalid hands FoV. Setting minimum...");
        init_hands_fov = 0.1f;
    }
}


void init_modifiers()
{
    fp_fov_modifier_instruction = 0;
    sp::util::file::append_text(log_file, "Initializing FoV modifiers...");

    // Find Address of main first person FoV modifier (normally 1.0f max)
    // AoB Scan
    sp::util::file::append_text(log_file, "    Scanning for FoV modifier instruction...");
    PFSEARCH fov_mod_search_res; // Pattern search struct
    while (fp_fov_modifier_instruction == 0) {
        FindPattern("F3 0F 59 40 38 48 8D 44 24 60 48 0F 46 C1 F3 44 0F 59 00 F3 41 0F 58 C0", &fov_mod_search_res, dxmd_base, dxmd_size);
        fp_fov_modifier_instruction = (DWORD64)(BYTE*)fov_mod_search_res.lpvResult;
    }

    // Inject code to access FoV modifier:
    sp::util::file::append_text(log_file, "    Injecting FoV changer code...");
    inject_fov_changer((BYTE*)fp_fov_modifier_instruction);
    
    // Find address of instruction that modifies first person hands
    // AoB Scan
    PFSEARCH hands_fov_mod_search_res;
    sp::util::file::append_text(log_file, "    Scanning for hands FoV modifier instruction...");
    FindPattern("F3 0F 59 0D ? ? ? ? FF 90 F0 01 00 00", &hands_fov_mod_search_res, dxmd_base, dxmd_size);
    fp_hands_fov_modifier_instruction = (DWORD64)(BYTE*)hands_fov_mod_search_res.lpvResult;

    sp::util::file::append_text(log_file, "    Injecting hands FoV changer code...");
    inject_hand_fov_changer((BYTE*)fp_hands_fov_modifier_instruction);
    
    // Set starting FoV based on player's preferred FoV
    sp::util::file::append_text(log_file, "Setting initial FoV...");
    *(float*)ptr_fov_modifier = (default_fov_modifier * init_fov) / default_hands_fov;
    if (*(float*)ptr_fov_modifier > 2.345f) {
        *(float*)ptr_fov_modifier = 2.345f;
    } else if (*(float*)ptr_fov_modifier < 0.1f) {
        *(float*)ptr_fov_modifier = 0.1f;
    }
    // Correct init_fov if given value wasn't valid
    init_fov = (*(float*)ptr_fov_modifier * default_hands_fov) / default_fov_modifier;
    
}


DWORD WINAPI fov_changer_thread(LPVOID param)
{
    running = true;

    init_modifiers();

    sp::util::file::append_text(log_file, "Starting FoV modifier loop...");
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
        } else if (fov_down_key != 0 && GetAsyncKeyState(fov_down_key) & 1) {
            // Decrease FoV (minimum = 0.1f)
            if (*(float*)ptr_fov_modifier >= 0.2f) {
                *(float*)ptr_fov_modifier = *(float*)ptr_fov_modifier - 0.1f;
                if (enable_beeps == 1) {
                    Beep(523, 100);
                }
            }
        } else if (hands_fov_up_key != 0 && GetAsyncKeyState(hands_fov_up_key) & 1) {
            // Increase Hands FoV
            current_hands_fov = current_hands_fov + 5;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        } else if (hands_fov_down_key != 0 && GetAsyncKeyState(hands_fov_down_key) & 1) {
            // Decrease Hands FoV
            if (current_hands_fov >= 5.0f) {
                current_hands_fov = current_hands_fov - 5;
                if (enable_beeps == 1) {
                    Beep(523, 100);
                }
            }
        } else if (restore_preferred_fov_key != 0 && GetAsyncKeyState(restore_preferred_fov_key) & 1) {
            // Set both FoVs back to player's preferred defaults
            *(float*)ptr_fov_modifier = (default_fov_modifier * init_fov) / default_hands_fov;
            current_hands_fov = init_hands_fov;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        } else if (reset_fov_key != 0 && GetAsyncKeyState(reset_fov_key) & 1) {
            // Set both FoVs back to default game-supported settings
            current_hands_fov = default_hands_fov;
            *(float*)ptr_fov_modifier = default_fov_modifier;
            if (enable_beeps == 1) {
                Beep(523, 100);
            }
        }

        // Adjust hands FoV
        *(float*)ptr_fp_hands_fov_base_modifier = current_hands_fov / (*(float*)ptr_fov_modifier);
        Sleep(5);
    }
    return 0;
}
