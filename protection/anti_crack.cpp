#include "anti_crack.h"
#include "../security/lazy_importer.hpp"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <TlHelp32.h>
#include <ntstatus.h>
#include <Psapi.h>
#include <winternl.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <ShlObj.h>
#pragma comment(lib, "shlwapi.lib")

// Define NT status code if not already defined
#ifndef STATUS_ASSERTION_FAILURE
#define STATUS_ASSERTION_FAILURE ((NTSTATUS)0xC0000420L)
#endif

typedef LONG NTSTATUS;

typedef NTSTATUS(NTAPI* pdef_RtlAdjustPrivilege)(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN CurrentThread,
    PBOOLEAN Enabled
);

typedef NTSTATUS(NTAPI* pdef_NtRaiseHardError)(
    NTSTATUS ErrorStatus,
    ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask,
    PULONG_PTR Parameters,
    ULONG ValidResponseOptions,
    PULONG Response
);

namespace AntiCrack {

    int getProcID(const std::string& p_name) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (!Process32FirstW(snapshot, &processEntry)) {
            CloseHandle(snapshot);
            return 0;
        }

        do {
            wchar_t procName[MAX_PATH];
            size_t numConverted;
            mbstowcs_s(&numConverted, procName, p_name.c_str(), MAX_PATH);
            if (_wcsicmp(processEntry.szExeFile, procName) == 0) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &processEntry));

        CloseHandle(snapshot);
        return 0;
    }

    bool isProcRunning(const std::string& process) {
        return getProcID(process) != 0;
    }

    namespace BSOD {
        void bsod() {
            BOOLEAN bEnabled;
            ULONG uResp;
            
            HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
            if (!hNtdll) return;

            auto RtlAdjustPrivilege = reinterpret_cast<pdef_RtlAdjustPrivilege>(GetProcAddress(hNtdll, "RtlAdjustPrivilege"));
            auto NtRaiseHardError = reinterpret_cast<pdef_NtRaiseHardError>(GetProcAddress(hNtdll, "NtRaiseHardError"));

            if (RtlAdjustPrivilege && NtRaiseHardError) {
                RtlAdjustPrivilege(19, TRUE, FALSE, &bEnabled);
                NtRaiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, nullptr, 6, &uResp);
            }
        }
    }

    void HwidBan() {
        char buffer[MAX_PATH];
        GetTempPathA(MAX_PATH, buffer);
        std::string tempDir = buffer;
        std::string fileName = "wctKO02.tmp";
        std::string filePath = tempDir + "\\" + fileName;

        std::ofstream outputFile(filePath);
        if (outputFile.is_open()) {
            outputFile << "HWID Ban Flag" << std::endl;
            outputFile.close();
        }
    }

    bool CheckHwidBan() {
        char buffer[MAX_PATH];
        GetTempPathA(MAX_PATH, buffer);
        std::string tempDir = buffer;
        std::string fileName = "wctKO02.tmp";
        std::string filePath = tempDir + "\\" + fileName;

        DWORD dwAttrib = GetFileAttributesA(filePath.c_str());
        return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    static void CheckDebugString() {
        SetLastError(0);
        OutputDebugStringA("Anti Debug");
        if (GetLastError() != 0) {
            HwidBan();
            LI_FN(exit)(0);
        }
    }

    static void CheckIsDebuggerPresent() {
        if (IsDebuggerPresent()) {
            HwidBan();
            LI_FN(exit)(0);
        }
    }

    static void CheckRemoteDebugger() {
        BOOL found = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &found);
        if (found) {
            HwidBan();
            LI_FN(exit)(0);
        }
    }

    static void CheckDebugWindows() {
        const wchar_t* debugWindows[] = {
            L"Qt5QWindowIcon",          // x64dbg
            L"ProcessHacker",           // Process Hacker
            L"MainWindowClassName",     // Process Hacker
            L"Qt5153QTQWindowIcon",     // IDA
            L"Window",                  // CheatEngine
            L"WindowsForms10.Window.8.app.0.378734a" // FileGrabber
        };

        for (const auto& windowClass : debugWindows) {
            if (FindWindowW(windowClass, NULL)) {
                HwidBan();
                LI_FN(exit)(0);
            }
        }
    }

    static void CheckHardwareBreakpoints() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            if (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0 || ctx.Dr6 != 0 || ctx.Dr7 != 0) {
                HwidBan();
                LI_FN(exit)(0);
            }
        }
    }

    static void HideFromDebugger() {
        typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll) {
            pNtSetInformationThread NtSetInformationThread = 
                (pNtSetInformationThread)GetProcAddress(hNtdll, "NtSetInformationThread");
            if (NtSetInformationThread) {
                NtSetInformationThread(GetCurrentThread(), 0x11, 0, 0);
            }
        }
    }

    static bool IsProcessBlacklisted(const wchar_t* processName) {
        const wchar_t* blacklist[] = {
            L"ida.exe", L"ida64.exe",
            L"idag.exe", L"idag64.exe",
            L"idaw.exe", L"idaw64.exe",
            L"idaq.exe", L"idaq64.exe",
            L"idau.exe", L"idau64.exe",
            L"scylla.exe",
            L"protection_id.exe",
            L"x32dbg.exe", L"x64dbg.exe",
            L"ollydbg.exe",
            L"pestudio.exe",
            L"reshacker.exe",
            L"process hacker.exe",
            L"procexp.exe",
            L"immunitydebugger.exe",
            L"dumpcap.exe",
            L"wireshark.exe",
            L"ghidra.exe",
            L"dnspy.exe",
            L"die.exe" // Detect It Easy
        };

        for (const auto& blocked : blacklist) {
            if (_wcsicmp(processName, blocked) == 0) {
                return true;
            }
        }
        return false;
    }

    static void ScanForAnalysisTools() {
        DWORD processes[1024], needed;
        if (!EnumProcesses(processes, sizeof(processes), &needed)) {
            return;
        }

        DWORD numProcesses = needed / sizeof(DWORD);
        for (DWORD i = 0; i < numProcesses; i++) {
            if (processes[i] != 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
                if (hProcess) {
                    wchar_t processName[MAX_PATH];
                    if (GetModuleBaseNameW(hProcess, NULL, processName, sizeof(processName)/sizeof(wchar_t))) {
                        if (IsProcessBlacklisted(processName)) {
                            CloseHandle(hProcess);
                            BSOD::bsod();
                            return;
                        }
                    }
                    CloseHandle(hProcess);
                }
            }
        }
    }

    static bool CheckForEmulation() {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        
        // Check for suspicious CPU characteristics
        if (systemInfo.dwNumberOfProcessors < 2) return true;
        if (systemInfo.dwPageSize != 4096) return true;
        
        // Time delta check
        DWORD startTick = GetTickCount();
        Sleep(100);
        DWORD endTick = GetTickCount();
        if ((endTick - startTick) < 50 || (endTick - startTick) > 150) return true;
        
        return false;
    }

    static bool DetectBreakpoints() {
        unsigned char* ptr = (unsigned char*)DetectBreakpoints;
        // Check for common debugger breakpoints
        if (ptr[0] == 0xCC) return true; // INT3 breakpoint
        if (ptr[0] == 0xCD && ptr[1] == 0x03) return true; // INT 3
        return false;
    }

    static void CheckMemoryModifications() {
        static const DWORD protections = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        
        MEMORY_BASIC_INFORMATION mbi;
        unsigned char* addr = 0;
        
        while (VirtualQuery(addr, &mbi, sizeof(mbi))) {
            if ((mbi.Protect & protections) && !(mbi.Protect & PAGE_GUARD)) {
                // Check for common patches
                for (size_t i = 0; i < mbi.RegionSize - 1; i++) {
                    if (addr[i] == 0xEB || // JMP
                        addr[i] == 0x90 || // NOP
                        (addr[i] == 0xE9 && i < mbi.RegionSize - 4)) { // JMP NEAR
                        BSOD::bsod();
                        return;
                    }
                }
            }
            addr += mbi.RegionSize;
            if (addr >= (unsigned char*)0x7FFF0000) break;
        }
    }

    static bool CheckCommonPaths() {
        const wchar_t* suspiciousPaths[] = {
            L"C:\\Program Files\\IDA*",
            L"C:\\Program Files (x86)\\IDA*",
            L"C:\\Program Files\\x64dbg",
            L"C:\\Program Files (x86)\\x64dbg",
            L"C:\\Program Files\\Wireshark",
            L"C:\\Program Files\\Ghidra",
            L"C:\\Program Files\\dnSpy",
            L"C:\\Program Files (x86)\\dnSpy",
            L"C:\\Program Files\\Cheat Engine*",
            L"C:\\Program Files (x86)\\Cheat Engine*",
            L"C:\\Program Files\\Process Hacker*",
            L"C:\\Program Files (x86)\\Process Hacker*",
            L"*\\Desktop\\IDA*",
            L"*\\Desktop\\x64dbg*",
            L"*\\Downloads\\IDA*",
            L"*\\Downloads\\x64dbg*",
            L"*\\Hex-Rays*",
            L"*\\Binary Ninja*",
            L"*\\OllyDbg*",
            L"*\\radare2*",
            L"*\\Ghidra*"
        };

        wchar_t userProfile[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, userProfile))) {
            for (const auto& basePath : suspiciousPaths) {
                WIN32_FIND_DATAW findData;
                wchar_t searchPath[MAX_PATH];

                // Replace * in desktop/downloads paths
                wchar_t expandedPath[MAX_PATH];
                wcscpy_s(expandedPath, basePath);
                if (wcsstr(basePath, L"*\\Desktop")) {
                    PathCombineW(searchPath, userProfile, L"Desktop");
                    wchar_t* p = wcsstr(expandedPath, L"*\\Desktop");
                    if (p) wcscpy_s(p, MAX_PATH - (p - expandedPath), searchPath + 2);
                }
                else if (wcsstr(basePath, L"*\\Downloads")) {
                    PathCombineW(searchPath, userProfile, L"Downloads");
                    wchar_t* p = wcsstr(expandedPath, L"*\\Downloads");
                    if (p) wcscpy_s(p, MAX_PATH - (p - expandedPath), searchPath + 2);
                }
                else {
                    wcscpy_s(expandedPath, basePath);
                }

                HANDLE hFind = FindFirstFileW(expandedPath, &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    FindClose(hFind);
                    return true; // Found suspicious software
                }
            }
        }
        return false;
    }

    static bool CheckRegistryKeys() {
        const wchar_t* suspiciousKeys[] = {
            L"SOFTWARE\\Hex-Rays",
            L"SOFTWARE\\x64dbg",
            L"SOFTWARE\\OllyDbg",
            L"SOFTWARE\\CheatEngine",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\IDA*",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\x64dbg*",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Ghidra*",
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process Hacker*"
        };

        for (const auto& key : suspiciousKeys) {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
            if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
        }
        return false;
    }

    namespace vmDetection {
        bool isVM() {
            const std::vector<std::wstring> knownProcessIds = {
                L"vboxservice.exe", L"vboxtray.exe",
                L"vmtoolsd.exe", L"vmwaretray.exe",
                L"vmwareuser.exe", L"VGAuthService.exe",
                L"vmacthlp.exe", L"vmusrvc.exe"
            };

            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE) return false;

            PROCESSENTRY32W pe32;
            pe32.dwSize = sizeof(pe32);

            bool found = false;
            if (Process32FirstW(snapshot, &pe32)) {
                do {
                    for (const auto& processName : knownProcessIds) {
                        if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                } while (Process32NextW(snapshot, &pe32));
            }

            CloseHandle(snapshot);
            return found;
        }

        bool checkVMDrivers() {
            const std::vector<std::wstring> vmDrivers = {
                L"VBoxGuest", L"VBoxMouse", L"VBoxSF", L"VBoxVideo",
                L"vmci", L"vmhgfs", L"vmmouse", L"vmscsi", L"vmusbmouse",
                L"vmx_svga", L"vmxnet"
            };

            DWORD drivers[1024];
            DWORD cbNeeded;
            if (EnumDeviceDrivers(reinterpret_cast<LPVOID*>(drivers), sizeof(drivers), &cbNeeded)) {
                WCHAR driverPath[MAX_PATH];
                for (unsigned i = 0; i < (cbNeeded / sizeof(drivers[0])); i++) {
                    if (GetDeviceDriverBaseNameW(reinterpret_cast<LPVOID>(drivers[i]), driverPath, MAX_PATH)) {
                        for (const auto& vmDriver : vmDrivers) {
                            if (_wcsicmp(driverPath, vmDriver.c_str()) == 0) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }
    }

    bool isSafeSystem() {
        // Check for VM
        if (vmDetection::isVM() || vmDetection::checkVMDrivers()) {
            return false;
        }

        // Check for known processes
        const std::vector<std::wstring> knownProcessIds = {
            L"ollydbg.exe", L"x64dbg.exe", L"x32dbg.exe",
            L"ida.exe", L"ida64.exe", L"idag.exe", L"idag64.exe",
            L"idaw.exe", L"idaw64.exe", L"idaq.exe", L"idaq64.exe",
            L"idau.exe", L"idau64.exe", L"scylla.exe", L"protection_id.exe",
            L"windbg.exe", L"reshacker.exe", L"ImportREC.exe", L"IMMUNITYDEBUGGER.EXE",
            L"devenv.exe", L"ProcessHacker.exe", L"cheatengine-x86_64.exe", L"cheatengine-i386.exe",
            L"cheatengine.exe", L"fiddler.exe"
        };

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return true;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(pe32);

        bool found = false;
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                for (const auto& processName : knownProcessIds) {
                    if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return !found;
    }

    void InitializeAntiDebug() {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Tick, NULL, 0, NULL);
    }

    static bool CheckNewProcesses() {
        static DWORD lastTickCount = 0;
        static std::vector<DWORD> knownProcessIds;
        
        // Check every 500ms
        if (GetTickCount() - lastTickCount < 500) {
            return false;
        }
        lastTickCount = GetTickCount();

        DWORD processes[1024], cbNeeded;
        if (!EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
            return false;
        }

        DWORD numProcesses = cbNeeded / sizeof(DWORD);
        for (DWORD i = 0; i < numProcesses; i++) {
            if (processes[i] != 0) {
                // Check if this is a new process
                if (std::find(knownProcessIds.begin(), knownProcessIds.end(), processes[i]) == knownProcessIds.end()) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
                    if (hProcess) {
                        wchar_t processName[MAX_PATH];
                        if (GetModuleBaseNameW(hProcess, NULL, processName, sizeof(processName)/sizeof(wchar_t))) {
                            if (IsProcessBlacklisted(processName)) {
                                CloseHandle(hProcess);
                                return true; // Suspicious process found
                            }
                        }
                        CloseHandle(hProcess);
                        knownProcessIds.push_back(processes[i]);
                    }
                }
            }
        }
        return false;
    }

    static bool CheckMemorySignatures() {
        MEMORY_BASIC_INFORMATION mbi;
        BYTE* addr = 0;
        
        const BYTE suspiciousPatterns[][10] = {
            {0x90, 0x90, 0x90, 0x90, 0x90},           // Multiple NOPs
            {0xEB, 0x00},                             // JMP +0
            {0xE9, 0x00, 0x00, 0x00, 0x00},          // JMP near
            {0xCC},                                   // INT3
            {0xCD, 0x03},                            // INT 3
            {0x68, 0x00, 0x00, 0x00, 0x00, 0xC3},    // PUSH addr; RET
        };

        while (VirtualQuery(addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
                
                BYTE* buffer = new BYTE[mbi.RegionSize];
                if (ReadProcessMemory(GetCurrentProcess(), addr, buffer, mbi.RegionSize, nullptr)) {
                    for (size_t i = 0; i < mbi.RegionSize - 10; i++) {
                        for (const auto& pattern : suspiciousPatterns) {
                            if (memcmp(&buffer[i], pattern, sizeof(pattern)) == 0) {
                                delete[] buffer;
                                return true; // Found suspicious pattern
                            }
                        }
                    }
                }
                delete[] buffer;
            }
            addr += mbi.RegionSize;
            if ((UINT_PTR)addr >= 0x7FFF0000) break;
        }
        return false;
    }

    static bool CheckLoadedModules() {
        HMODULE hMods[1024];
        DWORD cbNeeded;
        
        if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                wchar_t szModName[MAX_PATH];
                if (GetModuleFileNameExW(GetCurrentProcess(), hMods[i], szModName, sizeof(szModName)/sizeof(wchar_t))) {
                    // Check for suspicious DLL names
                    const wchar_t* suspiciousDlls[] = {
                        L"inject", L"hook", L"hack", L"cheat",
                        L"debug", L"crack", L"trainer", L"mod",
                        L"proxy", L"intercept", L"detour"
                    };

                    for (const auto& suspicious : suspiciousDlls) {
                        if (StrStrIW(szModName, suspicious)) {
                            return true; // Found suspicious DLL
                        }
                    }
                }
            }
        }
        return false;
    }

    static void MonitorFileChanges() {
        static FILETIME lastWriteTime = {0};
        wchar_t selfPath[MAX_PATH];
        
        if (GetModuleFileNameW(NULL, selfPath, MAX_PATH)) {
            HANDLE hFile = CreateFileW(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                FILETIME currentWriteTime;
                if (GetFileTime(hFile, NULL, NULL, &currentWriteTime)) {
                    if (lastWriteTime.dwLowDateTime == 0 && lastWriteTime.dwHighDateTime == 0) {
                        lastWriteTime = currentWriteTime;
                    }
                    else if (CompareFileTime(&lastWriteTime, &currentWriteTime) != 0) {
                        CloseHandle(hFile);
                        BSOD::bsod(); // File was modified
                        return;
                    }
                }
                CloseHandle(hFile);
            }
        }
    }

    namespace {
        // Helper-Funktion für Pfadüberprüfungen
        bool CheckSuspiciousPath() {
            wchar_t systemPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, systemPath))) {
                std::wstring path(systemPath);
                const wchar_t* suspicious[] = {
                    L"\\debugger", L"\\analyzer", L"\\decompiler",
                    L"\\ida", L"\\x64dbg", L"\\windbg"
                };
                
                for (const auto& suffix : suspicious) {
                    std::wstring fullPath = path + suffix;
                    if (PathFileExistsW(fullPath.c_str())) {
                        return true;
                    }
                }
            }
            return false;
        }
    }

    void Tick() {
        while (true) {
            // Original checks
            CheckDebugString();
            CheckIsDebuggerPresent();
            CheckRemoteDebugger();
            CheckDebugWindows();
            CheckHardwareBreakpoints();
            HideFromDebugger();

            // Enhanced protection
            ScanForAnalysisTools();
            if (CheckForEmulation()) {
                BSOD::bsod();
            }
            if (DetectBreakpoints()) {
                BSOD::bsod();
            }
            CheckMemoryModifications();

            // New continuous monitoring
            if (CheckNewProcesses()) {
                BSOD::bsod();
            }
            if (CheckMemorySignatures()) {
                BSOD::bsod();
            }
            if (CheckLoadedModules()) {
                BSOD::bsod();
            }
            MonitorFileChanges();

            // Re-check system safety periodically
            if (!isSafeSystem()) {
                BSOD::bsod();
            }

            Sleep(5); // Ultra-fast scanning every 5ms
        }
    }
} 