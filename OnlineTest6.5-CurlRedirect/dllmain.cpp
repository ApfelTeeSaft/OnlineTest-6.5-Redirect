#include <windows.h>
#include <iostream>
#include "Minhook/include/MinHook.h"
#include "globals.h"
#include "Url.h"

typedef int(__cdecl* t_curl_easy_setopt)(int curl_handle, int option, ...);
t_curl_easy_setopt o_curl_easy_setopt = nullptr;

int __cdecl hk_curl_easy_setopt(int curl_handle, int option, ...)
{
    if (!curl_handle) return 43;

    va_list args;
    va_start(args, option);

    int result = 0;

    if (option == 81)
    {
        result = o_curl_easy_setopt(curl_handle, option, 0);
    }
    else if (option == 10002)
    {
        char* url = va_arg(args, char*);
        if (url)
        {
            std::string url_str = url;
            Uri uri = Uri::Parse(url_str);

            if (uri.Host.ends_with("ol.epicgames.com") ||
                uri.Host.ends_with("epicgames.dev") ||
                uri.Host.ends_with("ol.epicgames.net") ||
                uri.Host.ends_with(".akamaized.net") ||
                uri.Host.ends_with("on.epicgames.com") ||
                uri.Host.ends_with("game-social.epicgames.com") ||
                uri.Host.contains("superawesome.com") ||
                uri.Host.contains("ak.epicgames.com"))
            {
                std::string redirected_url = Uri::CreateUri(URL_PROTOCOL, URL_HOST, URL_PORT, uri.Path, uri.QueryString);

                std::cout << "Redirecting: " << url << " -> " << redirected_url << std::endl;

                result = o_curl_easy_setopt(curl_handle, option, redirected_url.c_str());
            }
            else
            {
                result = o_curl_easy_setopt(curl_handle, option, url);
            }
        }
        else
        {
            result = o_curl_easy_setopt(curl_handle, option, url);
        }
    }
    else
    {
        void* arg = va_arg(args, void*);
        result = o_curl_easy_setopt(curl_handle, option, arg);
    }

    va_end(args);
    return result;
}

DWORD WINAPI HookThread(LPVOID)
{
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    if (MH_Initialize() != MH_OK)
    {
        std::cout << "Failed to initialize MinHook" << std::endl;
        return 1;
    }

    std::cout << "OT6 Redirect by ApfelTeeSaft" << std::endl;

    uintptr_t Base = (uintptr_t)GetModuleHandleA(0);
    void* p_curl_easy_setopt = (void*)(Base + 0x2090420);

    std::cout << "Hooking curl_easy_setopt at: 0x" << std::hex << p_curl_easy_setopt << std::endl;

    if (MH_CreateHook(p_curl_easy_setopt, &hk_curl_easy_setopt, (LPVOID*)&o_curl_easy_setopt) != MH_OK)
    {
        std::cout << "Failed to create hook" << std::endl;
        return 1;
    }

    if (MH_EnableHook(p_curl_easy_setopt) != MH_OK)
    {
        std::cout << "Failed to enable hook" << std::endl;
        return 1;
    }

    std::cout << "Hook installed successfully!" << std::endl;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
    }
    return TRUE;
}