#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

namespace visualstudio_integration
{
    // Visual Studio Installer process names to check for
    const std::vector<std::wstring> vs_installer_processes = {
        L"vs_installer.exe",      // Visual Studio Installer
        L"vs_bootstrapper.exe",   // VS Bootstrapper
        L"vs_installerservice.exe", // VS Installer Service
        L"vs_installerui.exe",    // VS Installer UI
        L"vs_installer.exe",      // Main installer
        L"vs_installer.exe"       // Alternative name
    };

    // Check if Visual Studio Installer is running
    bool is_visual_studio_installer_running()
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &pe32))
        {
            do
            {
                for (const auto& vs_process : vs_installer_processes)
                {
                    if (_wcsicmp(pe32.szExeFile, vs_process.c_str()) == 0)
                    {
                        CloseHandle(snapshot);
                        return true;
                    }
                }
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return false;
    }

    // Get Visual Studio Installer process ID
    DWORD get_visual_studio_installer_pid()
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &pe32))
        {
            do
            {
                if (_wcsicmp(pe32.szExeFile, L"vs_installer.exe") == 0)
                {
                    CloseHandle(snapshot);
                    return pe32.th32ProcessID;
                }
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return 0;
    }

    // Inject into Visual Studio Installer process
    bool inject_into_visual_studio_installer()
    {
        DWORD vs_pid = get_visual_studio_installer_pid();
        if (vs_pid == 0) return false;

        // Open Visual Studio Installer process
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, vs_pid);
        if (!hProcess) return false;

        // Get current module path
        char currentModule[MAX_PATH];
        GetModuleFileNameA(NULL, currentModule, MAX_PATH);

        // Allocate memory in Visual Studio Installer process
        LPVOID pRemoteBuffer = VirtualAllocEx(hProcess, NULL, strlen(currentModule) + 1, 
                                             MEM_COMMIT, PAGE_READWRITE);
        if (!pRemoteBuffer)
        {
            CloseHandle(hProcess);
            return false;
        }

        // Write module path to Visual Studio Installer process
        if (!WriteProcessMemory(hProcess, pRemoteBuffer, currentModule, 
                              strlen(currentModule) + 1, NULL))
        {
            VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // Get LoadLibraryA address
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        LPVOID pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");

        // Create remote thread to load our module
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
                                          (LPTHREAD_START_ROUTINE)pLoadLibraryA, 
                                          pRemoteBuffer, 0, NULL);
        if (!hThread)
        {
            VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // Wait for thread to complete
        WaitForSingleObject(hThread, INFINITE);

        // Cleanup
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);

        return true;
    }

    // Check if we should run as Visual Studio Installer extension
    bool should_run_as_vs_installer_extension()
    {
        return is_visual_studio_installer_running();
    }

    // Initialize Visual Studio Installer integration
    bool init()
    {
        if (!is_visual_studio_installer_running())
        {
            MessageBoxA(NULL, "Visual Studio Installer is required to run this component.\nPlease open Visual Studio Installer and try again.", 
                       "Component Manager", MB_OK | MB_ICONINFORMATION);
            return false;
        }

        return true;
    }

    // Legacy function names for compatibility
    bool is_visual_studio_running()
    {
        return is_visual_studio_installer_running();
    }

    DWORD get_visual_studio_pid()
    {
        return get_visual_studio_installer_pid();
    }

    bool inject_into_visual_studio()
    {
        return inject_into_visual_studio_installer();
    }
} 