#include <windows.h>
#include <iostream>
#include "Minhook/include/MinHook.h"
#include "globals.h"
#include "Url.h"
#include "curl.h"

typedef CURLcode(__cdecl* t_curl_easy_setopt)(struct Curl_easy*, CURLoption, ...);
typedef CURLcode(__cdecl* t_curl_setopt)(struct Curl_easy*, CURLoption, va_list);

t_curl_easy_setopt o_curl_easy_setopt = nullptr;
t_curl_setopt o_curl_setopt = nullptr;

inline CURLcode CurlSetOpt_(struct Curl_easy* data, CURLoption option, ...)
{
    va_list arg;
    va_start(arg, option);
    CURLcode result = o_curl_setopt(data, option, arg);
    va_end(arg);
    return result;
}

CURLcode __cdecl hk_curl_easy_setopt(struct Curl_easy* curl, CURLoption option, ...)
{
    std::cout << "[Hooked] curl_easy_setopt called" << std::endl;

    va_list args;
    va_start(args, option);

    CURLcode result = {};
    if (!curl) return CURLE_BAD_FUNCTION_ARGUMENT;

    if (option == CURLOPT_SSL_VERIFYPEER)
    {
        result = CurlSetOpt_(curl, option, 0);
    }
    else if (option == CURLOPT_URL)
    {
        std::string url = va_arg(args, char*);
        Uri uri = Uri::Parse(url);

        std::cout << "URL: " << uri.Host << uri.Path << '\n';

        if (uri.Host.ends_with("ol.epicgames.com") ||
            uri.Host.ends_with("epicgames.dev") ||
            uri.Host.ends_with("ol.epicgames.net") ||
            uri.Host.ends_with(".akamaized.net") ||
            uri.Host.ends_with("on.epicgames.com") ||
            uri.Host.ends_with("game-social.epicgames.com") ||
            uri.Host.contains("superawesome.com") ||
            uri.Host.contains("ak.epicgames.com"))
        {
            url = Uri::CreateUri(URL_PROTOCOL, URL_HOST, URL_PORT, uri.Path, uri.QueryString);
        }

        result = CurlSetOpt_(curl, option, url.c_str());
    }
    else
    {
        result = CurlSetOpt_(curl, option, args);
    }

    va_end(args);
    return result;
}

DWORD WINAPI HookThread(LPVOID)
{
    if (MH_Initialize() != MH_OK)
        return 1;

    void* p_curl_easy_setopt = (void*)0x2090420;

    MH_CreateHook(p_curl_easy_setopt, &hk_curl_easy_setopt, (LPVOID*)&o_curl_easy_setopt);
    MH_EnableHook(p_curl_easy_setopt);

    std::cout << "Hooks installed successfully!" << std::endl;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
    }
    return TRUE;
}