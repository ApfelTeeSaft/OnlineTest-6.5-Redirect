#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#include <string>
#include <iostream>

std::wstring OpenFolderDialog()
{
    wchar_t path[MAX_PATH] = { 0 };
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select Fortnite Installation Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl)
    {
        SHGetPathFromIDList(pidl, path);
        CoTaskMemFree(pidl);
        return std::wstring(path);
    }
    return L"";
}

bool FileExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributes(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool InjectDLL(DWORD processID, const std::wstring& dllPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess)
        return false;

    void* allocMem = VirtualAllocEx(hProcess, nullptr, dllPath.size() * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem)
    {
        CloseHandle(hProcess);
        return false;
    }

    WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), dllPath.size() * sizeof(wchar_t), nullptr);

    HMODULE hKernel32 = GetModuleHandle(L"Kernel32.dll");
    FARPROC loadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryW, allocMem, 0, nullptr);
    if (!hThread)
    {
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return true;
}

int main()
{
    std::wstring fortniteFolder = OpenFolderDialog();
    if (fortniteFolder.empty())
    {
        MessageBox(NULL, L"No folder selected!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::wstring fortniteExe = fortniteFolder + L"\\FortniteGame\\Binaries\\Win32\\FortniteClient-Win32-Shipping.exe";
    if (!FileExists(fortniteExe))
    {
        MessageBox(NULL, L"Invalid Fortnite folder!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::wstring cmdArgs = L" -epicapp=Fortnite -epicenv=Prod -epicportal -epiclocale=en-us -skippatchcheck -NOSSLPINNING";
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(fortniteExe.c_str(), &cmdArgs[0], NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
    {
        MessageBox(NULL, L"Failed to start Fortnite!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    std::wstring dllPath = L"redirect.dll";
    if (!InjectDLL(pi.dwProcessId, dllPath))
    {
        MessageBox(NULL, L"Failed to inject DLL!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    CloseHandle(pi.hProcess);
    MessageBox(NULL, L"Fortnite started and DLL injected!", L"Success", MB_OK | MB_ICONINFORMATION);
    return 0;
}