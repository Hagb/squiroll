#if __INTELLISENSE__
#undef _HAS_CXX20
#define _HAS_CXX20 0
#endif

#include <string.h>
#include <limits.h>
#include <string>
#include <vector>
#include <squirrel.h>


#include "netcode.h"
#include "util.h"
#include "AllocMan.h"
#include "Autopunch.h"
#include "PatchUtils.h"
#include "log.h"
#include "file_replacement.h"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

#if !MINGW_COMPAT
#pragma comment (lib, "Ws2_32.lib")
#endif

const uintptr_t base_address = (uintptr_t)GetModuleHandleA(NULL);
uintptr_t libact_base_address = 0;
uintptr_t lobby_base_address = 0;

//SQVM Rx4DACE4 initialized at Rx124710
HSQUIRRELVM VM;

void Cleanup() {
    autopunch_cleanup();
}

void Debug() {
    AllocConsole();
    (void)freopen("CONIN$", "r", stdin);
    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
    SetConsoleTitleW(L"th155r debug");
}

struct ScriptAPI {
    uint8_t dummy[0xF8];
    bool load_plugins_from_pak;
};

typedef void* thisfastcall act_script_plugin_load_t(
    void* self,
    thisfastcall_edx(int dummy_edx,)
    const char* plugin_path
);

hostent* WSAAPI my_gethostbyname(const char* name) {
    log_printf("LOBBY HOST: %s\n", name);
    return gethostbyname(name);
}

void patch_se_libact(void* base_address);
void patch_se_lobby(void* base_address);
void patch_se_upnp(void* base_address);
void patch_se_information(void* base_address);
void patch_se_trust(void* base_address);

#define load_plugin_from_pak_addr (0x12DDD0_R)

void* thisfastcall patch_exe_script_plugin(
    void* self,
    thisfastcall_edx(int dummy_edx,)
    const char* plugin_path
) {
    void* base_address = ((act_script_plugin_load_t*)load_plugin_from_pak_addr)(
        self,
        thisfastcall_edx(dummy_edx,)
        plugin_path
    );

    if (base_address) {
        log_printf("Applying patches for \"%s\" at %p from EXE\n", plugin_path, base_address);
        if (!strcmp(plugin_path, "data/plugin/se_libact.dll")) {
            patch_se_libact(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_lobby.dll")) {
            patch_se_lobby(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_upnp.dll")) {
            patch_se_upnp(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_information.dll")) {
            patch_se_information(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_trust.dll")) {
            patch_se_trust(base_address);
        }
    }
    else {
        if (!strcmp(plugin_path, "data/plugin\\data/plugin/se_libact.dll.dll")) {
            base_address = (void*)GetModuleHandleW(L"Netcode.dll");
        }
    }

    return base_address;
}

void* thisfastcall patch_act_script_plugin(
    void* self,
    thisfastcall_edx(int dummy_edx,)
    const char* plugin_path
) {
    void* base_address = based_pointer<act_script_plugin_load_t>(libact_base_address, 0x1CC60)(
        self,
        thisfastcall_edx(dummy_edx,)
        plugin_path
    );

    if (base_address) {
        log_printf("Applying patches for \"%s\" at %p from ACT\n", plugin_path, base_address);
        if (!strcmp(plugin_path, "data/plugin/se_lobby.dll")) {
            patch_se_lobby(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_upnp.dll")) {
            patch_se_upnp(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_information.dll")) {
            patch_se_information(base_address);
        }
        else if (!strcmp(plugin_path, "data/plugin/se_trust.dll")) {
            patch_se_trust(base_address);
        }
    }

    return base_address;
}

void patch_se_libact(void* base_address) {
    libact_base_address = (uintptr_t)base_address;
    
#if ALLOCATION_PATCH_TYPE == PATCH_SQUIRREL_ALLOCS
    //hotpatch_rel32(based_pointer(base_address, 0xC4BE5), my_malloc);
    //hotpatch_rel32(based_pointer(base_address, 0xC4C89), my_realloc);
    //hotpatch_rel32(based_pointer(base_address, 0xC4C75), my_free);
#elif ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(based_pointer(base_address, 0x134632), my_malloc);
    hotpatch_jump(based_pointer(base_address, 0x12C67B), my_calloc);
    hotpatch_jump(based_pointer(base_address, 0x13BD53), my_realloc);
    hotpatch_jump(based_pointer(base_address, 0x12C6D8), my_free);
    hotpatch_jump(based_pointer(base_address, 0x138F44), my_recalloc);
    hotpatch_jump(based_pointer(base_address, 0x141C26), my_msize);
#endif

    hotpatch_rel32(based_pointer(base_address, 0x15F7C), patch_act_script_plugin);
}

/*
typedef int thisfastcall send_text_t(
    void* self,
    thisfastcall_edx(int dummy_edx, )
    const char* str
);

int thisfastcall log_sent_text(
    void* self,
    thisfastcall_edx(int dummy_edx,)
    const char* str
) {
    log_printf("Sending:%s", str);
    return based_pointer<send_text_t>(lobby_base_address, 0x20820)(
        self,
        thisfastcall_edx(dummy_edx,)
        str
    );
}
*/

void patch_se_lobby(void* base_address) {
    lobby_base_address = (uintptr_t)base_address;

#if ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(based_pointer(base_address, 0x41007), my_malloc);
    hotpatch_jump(based_pointer(base_address, 0x402F8), my_calloc);
    hotpatch_jump(based_pointer(base_address, 0x41055), my_realloc);
    hotpatch_jump(based_pointer(base_address, 0x40355), my_free);
    hotpatch_jump(based_pointer(base_address, 0x4DCF1), my_recalloc);
    hotpatch_jump(based_pointer(base_address, 0x53F76), my_msize);
#endif
    //hotpatch_import(based_pointer(base_address, 0x1292AC), my_gethostbyname);

    //mem_write(based_pointer(base_address, 0x206F8), NOP_BYTES<2>);
    //mem_write(based_pointer(base_address, 0x20872), NOP_BYTES<2>);

    mem_write(based_pointer(base_address, 0x203AA), NOP_BYTES<55>);

    //hotpatch_rel32(based_pointer(base_address, 0x4C6F), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x4D33), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x52C7), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x62C7), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x6902), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x6E50), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x6F4D), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x7084), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x7436), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x7643), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x7C67), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x7D2D), log_sent_text);
    //hotpatch_rel32(based_pointer(base_address, 0x84E6), log_sent_text);
}

void patch_se_upnp(void* base_address) {
#if ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(based_pointer(base_address, 0x1C4BB), my_malloc);
    hotpatch_jump(based_pointer(base_address, 0x21229), my_calloc);
    hotpatch_jump(based_pointer(base_address, 0x1C509), my_realloc);
    hotpatch_jump(based_pointer(base_address, 0x215AD), my_free);
    hotpatch_jump(based_pointer(base_address, 0x2D3BF), my_recalloc);
    hotpatch_jump(based_pointer(base_address, 0x32413), my_msize);
#endif
}

void patch_se_information(void* base_address) {
#if ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(based_pointer(base_address, 0x1B853), my_malloc);
    hotpatch_jump(based_pointer(base_address, 0x1BBE9), my_calloc);
    hotpatch_jump(based_pointer(base_address, 0x1B8A1), my_realloc);
    hotpatch_jump(based_pointer(base_address, 0x1BFAE), my_free);
    hotpatch_jump(based_pointer(base_address, 0x25FF0), my_recalloc);
    hotpatch_jump(based_pointer(base_address, 0x2B406), my_msize);
#endif
}

void patch_se_trust(void* base_address) {
#if ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(based_pointer(base_address, 0x5BA7), my_malloc);
    hotpatch_jump(based_pointer(base_address, 0x5C92), my_calloc);
    hotpatch_jump(based_pointer(base_address, 0x936E), my_realloc);
    hotpatch_jump(based_pointer(base_address, 0x5B6D), my_free);
    hotpatch_jump(based_pointer(base_address, 0x78AB), my_recalloc);
    hotpatch_jump(based_pointer(base_address, 0x933B), my_msize);
#endif

    static constexpr const uint8_t data[] = { 0x31, 0xC0 };
    mem_write(based_pointer(base_address, 0x15CC), data);
}

#define sq_vm_malloc_call_addr (0x186745_R)
#define sq_vm_realloc_call_addr (0x18675A_R)
#define sq_vm_free_call_addr (0x186737_R)

#define malloc_base_addr (0x312D61_R)
#define calloc_base_addr (0x3122EA_R)
#define realloc_base_addr (0x312DAF_R)
#define free_base_addr (0x312347_R)
#define recalloc_base_addr (0x3182DF_R)
#define msize_base_addr (0x31ED30_R)

#define WSASend_import_addr (0x3884D0_R)
#define WSASendTo_import_addr (0x3884D4_R)
#define WSARecvFrom_import_addr (0x3884D8_R)
#define bind_import_addr (0x3884E0_R)
#define closesocket_import_addr (0x388514_R)

#define patch_act_script_plugin_hook_addr (0x127ADC_R)

// Basic thcrap style replacement mode
#define file_replacement_hook_addrA (0x23FAA_R)
#define file_replacement_read_addrA (0x2DFA1_R)
#define file_replacement_read_addrB (0x2DFED_R)
#define CloseHandle_import_addr (0x3881DC_R)

// No encryption replacement mode
#define file_replacement_hook_addrB (0x23F98_R)

void patch_autopunch() {
    //hotpatch_import(WSARecvFrom_import_addr, my_WSARecvFrom);
    //hotpatch_import(WSASendTo_import_addr, my_WSASendTo);
    //hotpatch_import(WSASend_import_addr, my_WSASend);
    //hotpatch_import(bind_import_addr, my_bind);
    //hotpatch_import(closesocket_import_addr, my_closesocket);

    //autopunch_init();
}

void patch_allocman() {
#if ALLOCATION_PATCH_TYPE == PATCH_SQUIRREL_ALLOCS
    hotpatch_rel32(sq_vm_malloc_call_addr, my_malloc);
    hotpatch_rel32(sq_vm_realloc_call_addr, my_realloc);
    hotpatch_rel32(sq_vm_free_call_addr, my_free);
#elif ALLOCATION_PATCH_TYPE == PATCH_ALL_ALLOCS
    hotpatch_jump(malloc_base_addr, my_malloc);
    hotpatch_jump(calloc_base_addr, my_calloc);
    hotpatch_jump(realloc_base_addr, my_realloc);
    hotpatch_jump(free_base_addr, my_free);
    hotpatch_jump(recalloc_base_addr, my_recalloc);
    hotpatch_jump(msize_base_addr, my_msize);
#endif
}

#if FILE_REPLACEMENT_TYPE != FILE_REPLACEMENT_NONE
void patch_file_loading() {

#if FILE_REPLACEMENT_TYPE == FILE_REPLACEMENT_BASIC_THCRAP
    hotpatch_call(file_replacement_hook_addrA, file_replacement_hook);

    static constexpr uint8_t patch[] = { 0xE9, 0x8C, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
    mem_write(file_replacement_hook_addrA + 5, patch);

    hotpatch_icall(file_replacement_read_addrA, file_replacement_read);
    hotpatch_icall(file_replacement_read_addrB, file_replacement_read);

    hotpatch_import(CloseHandle_import_addr, close_handle_hook);

#elif FILE_REPLACEMENT_TYPE == FILE_REPLACEMENT_NO_CRYPT

    hotpatch_call(file_replacement_hook_addrB, file_replacement_hook);

    static constexpr uint8_t patch[] = {
        0x85, 0xD2,                         // TEST EDX, EDX
        0x0F, 0x85, 0xBF, 0x00, 0x00, 0x00, // JNZ Rx24064
        0x0F, 0x1F, 0x44, 0x00, 0x00        // NOP
    };
    mem_write(file_replacement_hook_addrB + 5, patch);

#endif

}
#endif
// Initialization code shared by th155r and thcrap use
// Executes before the start of the process
void common_init() {
//#ifndef NDEBUG
    Debug();
//#endif
    patch_allocman();

    patch_netplay();
    //hotpatch_rel32(init_call_addr,my_init);

    //hotpatch_rel32(sq_vm_init_call_addrB, my_sq_vm_init); //not sure why its called twice but pretty sure the first call is enough

    //patch_autopunch();

    hotpatch_rel32(patch_act_script_plugin_hook_addr, patch_exe_script_plugin);

    //mem_write(0x1DEAD_R, INFINITE_LOOP_BYTES); // Replaces a TEST ECX, ECX

#if FILE_REPLACEMENT_TYPE == FILE_REPLACEMENT_NO_CRYPT
    patch_file_loading();
#endif
}

void yes_tampering() {
    hotpatch_ret(0x12E820_R, 0);
    hotpatch_ret(0x130630_R, 0);
    hotpatch_ret(0x132AF0_R, 0);
}

extern "C" {
    // FUNCTION REQUIRED FOR THE LIBRARY
    // th155r init function
    dll_export int stdcall netcode_init(int32_t param) {
        yes_tampering();
        common_init();
#if FILE_REPLACEMENT_TYPE == FILE_REPLACEMENT_BASIC_THCRAP
        patch_file_loading();
#endif
        return 0;
    }
    
    // thcrap init function
    // Thcrap already removes the tamper protection,
    // so that code is unnecessary to include here.
    dll_export void cdecl netcode_mod_init(void* param) {
        common_init();
    }
    
    // thcrap plugin init
    dll_export int stdcall thcrap_plugin_init() {
        if (HMODULE thcrap_handle = GetModuleHandleW(L"thcrap.dll")) {
            if (auto runconfig_game_get = (const char*(*)())GetProcAddress(thcrap_handle, "runconfig_game_get")) {
                const char* game_str = runconfig_game_get();
                if (game_str && !strcmp(game_str, "th155")) {
                    if (printf_t* log_func = (printf_t*)GetProcAddress(thcrap_handle, "log_printf")) {
                        log_printf = log_func;
                    }
                    //if (patchhook_register_t* patchhook_register_func = (patchhook_register_t*)GetProcAddress(thcrap_handle, "patchhook_register")) {
                        //patchhook_register = patchhook_register_func;
                    //}
                    return 0;
                }
            }
        }
        return 1;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (dwReason == DLL_PROCESS_DETACH) {
        Cleanup();
    }
    return TRUE;
}