#include <windows.h>
#include <winuser.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "AllocMan.h"
#include <squirrel.h>

#include "util.h"

const uintptr_t base_address = (uintptr_t)GetModuleHandleA(NULL);

//SQVM Rx4DACE4 initialized at Rx124710
HSQUIRRELVM* VM;

//Bool Manbow::SetWindowText(LPCSTR newname) Rx00EA50

void mem_write(LPVOID address, const void* data, size_t size)
{
    DWORD oldProtect;
    if (VirtualProtect(address, size, PAGE_READWRITE, &oldProtect))
    {
        memcpy(address, data, size);
        VirtualProtect(address, size, oldProtect, &oldProtect);
    }
}

void hotpatch_jump(void* target, void* replacement)
{
    uint8_t bytes[6] = { 0xE9, 0, 0, 0, 0, 0xCC };
    *(uint32_t*)&bytes[1] = (uintptr_t)replacement - (uintptr_t)target - 5;
    mem_write(target, bytes, sizeof(bytes));
}

void hotpatch_rel32(void* target, void* replacement)
{
    int32_t raw = (uintptr_t)replacement - (uintptr_t)target - 4;
    mem_write(target, &raw, sizeof(raw));
}

void GetSqVM(){
    VM = (HSQUIRRELVM*)0x4DACE4_R; 
}

#define closesocketA_call_addr (0x170383_R)
#define closesocketB_call_addr (0x1703ee_R)
#define bind_call_addr (0x1702f4_R)
#define sendto_call_addr (0x170502_R)
#define recvfrom_call_addr (0x170e5b_R)
#define sq_vm_malloc_call_addr (0x186745_R)
#define sq_vm_realloc_call_addr (0x18675A_R)
#define sq_vm_free_call_addr (0x186737_R)

void Cleanup()
{
}

void Debug(){
    AllocConsole();
    auto a = freopen("CONIN$","r",stdin);
    a = freopen("CONOUT$","w",stdout);
    a = freopen("CONOUT$","w",stderr);
    SetConsoleTitleW(L"th155r debug");
}

// Initialization code shared by th155r and thcrap use
// Executes before the start of the process
void common_init() {
    Debug();

    hotpatch_rel32((void*)sq_vm_malloc_call_addr, (void*)my_malloc);
    hotpatch_rel32((void*)sq_vm_realloc_call_addr, (void*)my_realloc);
    hotpatch_rel32((void*)sq_vm_free_call_addr, (void*)my_free);
}

void yes_tampering()
{
    static constexpr BYTE data[] = {0xC3};
    uintptr_t addrA = 0x12E820_R, addrB = 0x130630_R, addrC = 0x132AF0_R;
    mem_write((LPVOID)addrA, data, sizeof(data));
    mem_write((LPVOID)addrB, data, sizeof(data));
    mem_write((LPVOID)addrC, data, sizeof(data));
}

extern "C"
{
    // FUNCTION REQUIRED FOR THE LIBRARY
    // th155r init function
    __declspec(dllexport) int stdcall netcode_init(int32_t param)
    {
        yes_tampering();
        common_init();
        return 0;
    }
    
    // thcrap init function
    // Thcrap already removes the tamper protection,
    // so that code is unnecessary to include here.
    __declspec(dllexport) void cdecl netcode_mod_init(void* param) {
        common_init();
    }
    
    // thcrap plugin init
    __declspec(dllexport) int stdcall thcrap_plugin_init()
    {
        if (HMODULE thcrap_handle = GetModuleHandleA("thcrap.dll")) {
            if (auto runconfig_game_get = (const char*(*)())GetProcAddress(thcrap_handle, "runconfig_game_get")) {
                const char* game_str = runconfig_game_get();
                if (game_str && !strcmp(game_str, "th155")) {
                    return 0;
                }
            }
        }
        return 1;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (dwReason == DLL_PROCESS_DETACH)
    {
        Cleanup();
    }
    return TRUE;
}