#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <TlHelp32.h>

// This function is defined in main.cpp, we just declare it here to use it.
void PrintMessage(const std::string& message, const std::string& type);

inline void CheckInjectedModules() {
    const std::vector<std::wstring> blacklistedModules = {
        L"wmp.dll",             // Windows Media Player (often used for injection)
        L"sbie.dll",            // Sandboxie
        L"vgk.sys.dll",         // Vanguard related - should not be in a user-mode process
        L"áci.dll",             // Common name for Extreme Injector payload
        L"mumblelink.dll"      // Mumble overlay
    };

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    if (Module32First(hSnapshot, &me32)) {
        do {
            for (const auto& moduleName : blacklistedModules) {
                if (_wcsicmp(me32.szModule, moduleName.c_str()) == 0) {
                    // For all other modules, just block them
                    PrintMessage("Blacklisted module detected. Closing application.", "error");
                    Sleep(2000);
                    exit(0);
                }
            }
        } while (Module32Next(hSnapshot, &me32));
    }

    CloseHandle(hSnapshot);
}

void debugLoop();
void debugcheckwithoutloop(); 

VOID Authorize(std::string);
VOID AntiDebugging();
VOID BSOD();
VOID AntiHook();
VOID hookbsod();
VOID hookmsgboxa();
VOID unhookbsod();
VOID unhookmsgboxa();
VOID Anti64();
VOID AntiX64();
VOID AntiPH();
VOID AntiCE();
VOID AntiIDA();
VOID AntiDH();
VOID AntiCMD();
VOID NOSUSPEND();
VOID RefreshModules();
VOID AntiInjection();
VOID CheckText();
VOID MemoryCheck();
VOID AntiDump();
VOID CheckSize(DWORD min, DWORD max);
VOID JunkCode();
VOID banning();
VOID Check();
VOID SearchWindowClassNames();
VOID AntiDissam();
VOID AntiCallbackThread();
VOID AntiCallbackProcess();
VOID AntiBuffer();
VOID ExtraAntiDebugging();
VOID AntiObject();
VOID RefreshModules();
VOID AntiInjection();
VOID CheckBan();
VOID AntiSuspend();



bool IsSuspiciousMemoryAccessDetected(const void* targetAddress, const size_t bufferSize);
std::string CheckSub(std::string AppName, std::string AppID, std::string AppSecret, std::string AppVersion, std::string AppUrl, std::string License);
VOID CheckSesh(std::string AppName, std::string AppID, std::string AppSecret, std::string AppVersion, std::string AppUrl, std::string License);
bool KeyAuthOnRoids(std::string AppName, std::string AppID, std::string AppSecret, std::string AppVersion, std::string AppUrl, std::string License);
std::vector<std::uint8_t> InstallBytes(std::string AppName, std::string AppID, std::string AppSecret, std::string AppVersion, std::string AppUrl, std::string dwnloadkey, std::string License);
std::vector<std::uint8_t> InstallEncryptedBytes(std::string AppName, std::string AppID, std::string AppSecret, std::string AppVersion, std::string AppUrl, std::string dwnloadkey, std::string License, std::vector<unsigned char> encryptionKey);
VOID EncryptData(std::vector<std::uint8_t> bytes, std::vector<unsigned char> encryptionKey);
VOID CheckIntegOfEncryption();
VOID StartEncryption();

inline void hellajunk()
{
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
    JunkCode();
}

#define JUNK_CODE_FUNCTION_1() { \
    int counter = 5; \
    while (counter < 30) { \
        counter += 5; \
        if (counter % 3 == 0) { \
            counter *= 2; \
        } else { \
            counter -= 3; \
        } \
    } \
    \
    if (counter > 15) { \
        for (int i = 0; i < 6; ++i) { \
            counter += i * 7; \
            if (counter % 4 == 0) { \
                counter -= i; \
            } else { \
                counter += 3 * i; \
            } \
        } \
    } else { \
        counter -= 8; \
        if (counter < 10) { \
            counter += 5; \
        } else { \
            counter -= 2; \
        } \
    } \
    \
    do { \
        counter += 4; \
        if (counter % 6 == 0) { \
            counter -= 5; \
        } else { \
            counter += 6; \
        } \
    } while (counter % 5 != 0); \
    \
    while (counter > 0) { \
        counter -= 4; \
        if (counter % 4 == 0) { \
            counter += 7; \
        } else { \
            counter -= 3; \
        } \
    } \
}

#define JUNK_CODE_FUNCTION_2() { \
    int a = 10; \
    for (int i = 0; i < 5; ++i) { \
        a += i * 3; \
        if (a % 2 == 0) { \
            a -= i; \
        } else { \
            a += 2 * i; \
        } \
    } \
    \
    if (a > 20) { \
        for (int j = 0; j < 3; ++j) { \
            a *= (j + 2); \
            if (a % 3 == 0) { \
                a += j; \
            } else { \
                a -= j; \
            } \
        } \
    } else { \
        a -= 5; \
        if (a < 10) { \
            a += 3; \
        } else { \
            a -= 2; \
        } \
    } \
    \
    while (a % 4 != 0) { \
        a += 2; \
        if (a % 5 == 0) { \
            a -= 3; \
        } else { \
            a += 4; \
        } \
    } \
    \
    while (a % 4 != 0) { \
        a += 2; \
        if (a % 3 == 0) { \
            a -= 2; \
        } else { \
            a += 3; \
        } \
    } \
}
#define JUNK_CODE_FUNCTION_3() { \
    int counter = 5; \
    while (counter < 25) { \
        counter += 4; \
        if (counter % 3 == 0) { \
            counter *= 2; \
        } else { \
            counter -= 3; \
        } \
    } \
    \
    if (counter > 15) { \
        for (int i = 0; i < 5; ++i) { \
            counter += i * 6; \
            if (counter % 4 == 0) { \
                counter -= i; \
            } else { \
                counter += 3 * i; \
            } \
        } \
    } else { \
        counter -= 7; \
        if (counter < 10) { \
            counter += 4; \
        } else { \
            counter -= 2; \
        } \
    } \
    \
    do { \
        counter += 3; \
        if (counter % 6 == 0) { \
            counter -= 4; \
        } else { \
            counter += 5; \
        } \
    } while (counter % 5 != 0); \
    \
    while (counter > 0) { \
        counter -= 3; \
        if (counter % 4 == 0) { \
            counter += 6; \
        } else { \
            counter -= 2; \
        } \
    } \
}
#define JUNK_CODE_FUNCTION_4() { \
    int counter = 5; \
    while (counter < 25) { \
        counter += 10; \
        if (counter % 3 == 0) { \
            counter *= 2; \
        } else { \
            counter -= 3; \
        } \
    } \
    \
    if (counter > 15) { \
        for (int i = 0; i < 5; ++i) { \
            counter += i * 6; \
            if (counter % 4 == 0) { \
                counter -= i; \
            } else { \
                counter += 3 * i; \
            } \
        } \
    } else { \
        counter -= 7; \
        if (counter < 10) { \
            counter += 4; \
        } else { \
            counter -= 2; \
        } \
    } \
    \
    do { \
        counter += 3; \
        if (counter % 6 == 0) { \
            counter -= 4; \
        } else { \
            counter += 5; \
        } \
    } while (counter % 5 != 0); \
    \
    while (counter > 0) { \
        counter -= 50; \
        if (counter % 4 == 0) { \
            counter += 6; \
        } else { \
            counter -= 2; \
        } \
    } \
}
#define JUNK_CODE_FUNCTION_5() { \
    int counter = 5; \
    while (counter < 35) { \
        counter += 6; \
        if (counter % 3 == 0) { \
            counter *= 2; \
        } else { \
            counter -= 3; \
        } \
    } \
    \
    if (counter > 15) { \
        for (int i = 0; i < 7; ++i) { \
            counter += i * 8; \
            if (counter % 4 == 0) { \
                counter -= i; \
            } else { \
                counter += 3 * i; \
            } \
        } \
    } else { \
        counter -= 9; \
        if (counter < 10) { \
            counter += 6; \
        } else { \
            counter -= 2; \
        } \
    } \
    \
    do { \
        counter += 5; \
        if (counter % 6 == 0) { \
            counter -= 6; \
        } else { \
            counter += 7; \
        } \
    } while (counter % 5 != 0); \
    \
    while (counter > 0) { \
        counter -= 5; \
        if (counter % 4 == 0) { \
            counter += 8; \
        } else { \
            counter -= 3; \
        } \
    } \
}

#define JUNK_CODE_FUNCTION_6() { \
    int counter = 5; \
    while (counter < 40) { \
        counter += 7; \
        if (counter % 3 == 0) { \
            counter *= 2; \
            if (counter % 5 == 0) { \
                counter -= 3; \
            } else { \
                counter += 4; \
            } \
        } else { \
            counter -= 3; \
        } \
    } \
    \
    if (counter > 15) { \
        for (int i = 0; i < 8; ++i) { \
            counter += i * 9; \
            if (counter % 4 == 0) { \
                counter -= i; \
                if (counter % 3 == 0) { \
                    counter += 2 * i; \
                } else { \
                    counter -= i; \
                } \
            } else { \
                counter += 3 * i; \
            } \
        } \
    } else { \
        counter -= 10; \
        if (counter < 10) { \
            counter += 7; \
            if (counter % 2 == 0) { \
                counter -= 2; \
            } else { \
                counter += 3; \
            } \
        } else { \
            counter -= 2; \
        } \
    } \
    \
    do { \
        counter += 6; \
        if (counter % 6 == 0) { \
            counter -= 5; \
            if (counter % 4 == 0) { \
                counter += 8; \
            } else { \
                counter -= 3; \
            } \
        } else { \
            counter += 7; \
        } \
    } while (counter % 5 != 0); \
    \
    while (counter > 0) { \
        counter -= 6; \
        if (counter % 4 == 0) { \
            counter += 9; \
        } else { \
            counter -= 3; \
            if (counter % 2 == 0) { \
                counter += 4; \
            } else { \
                counter -= 2; \
            } \
        } \
    } \
}

#define JUNK_CODE_SIMPLE_COMPLEXITY() { \
    int a = 10; \
    for (int i = 0; i < 5; ++i) { \
        a += i * 3; \
    } \
    for (int i = 0; i < 5; ++i) { \
        a += i * 3; \
    } \
    if (a > 20) { \
        for (int j = 0; j < 3; ++j) { \
            a *= (j + 2); \
        } \
    } \
    if (a > 20) { \
        for (int j = 0; j < 3; ++j) { \
            a *= (j + 2); \
        } \
    } else { \
        a -= 5; \
    } \
    while (a % 4 != 0) { \
        a += 2; \
    } \
    while (a % 4 != 0) {\
        a += 2; \
            JUNK_CODE_FUNCTION_1(); \
    } \
}

#define JUNK_CODE_MEDIUM_COMPLEXITY() { \
    int a = 10; \
JUNK_CODE_FUNCTION_2(); \
JUNK_CODE_FUNCTION_6(); \
    if (a > 5) { \
        a += 5; \
        if (a < 20) { \
            for (int i = 0; i < 3; ++i) { \
                a += i * 3; \
                if (a % 2 == 0) { \
                    a *= 2; \
                    if (a > 20) { \
                        while (a % 3 != 0) { \
                            a += 3; \
                            for (int j = 0; j < 2; ++j) { \
                                a -= j * 2; \
                                if (a < 15) { \
                                    a += 5; \
                                    if (a % 4 == 0) { \
                                        a *= 2; \
                                        for (int k = 0; k < 3; ++k) { \
                                            a -= k * 3; \
                                            if (a > 10) { \
                                                a /= 2; \
                                                for (int l = 0; l < 4; ++l) { \
                                                    a += l * 4; \
                                                    while (a % 2 != 0) { \
                                                        a -= 1; \
                                                        for (int m = 0; m < 2; ++m) { \
                                                            a *= m + 1; \
                                                            if (a % 3 == 0) { \
                                                                a += 6; \
                                                            } else { \
                                                                a -= 2; \
                                                            } \
                                                            while (a % 4 != 0) { \
                                                                a += 2; \
                                                            } \
                                                        } \
                                                    } \
                                                    if (a < 15) { \
                                                        a += 7; \
                                                        if (a % 3 == 1) { \
                                                            a += 2; \
                                                        } \
                                                    } \
                                                } \
                                            } else { \
                                                a += 4; \
                                                if (a % 5 == 0) { \
                                                    a -= 5; \
                                                } \
                                            } \
                                        } \
                                    } else { \
                                        a -= 3; \
                                    } \
                                } \
                            } \
                        } \
                    } else { \
                        a -= 2; \
                        if (a < 5) { \
                            a += 10; \
                            for (int n = 0; n < 2; ++n) { \
                                a *= n + 1; \
                                if (a % 3 == 0) { \
                                    a += 6; \
                                    for (int o = 0; o < 3; ++o) { \
                                        a -= o * 2; \
                                        while (a % 3 != 0) { \
                                            a += 2; \
                                        } \
                                        if (a < 30) { \
                                            a *= 2; \
                                            for (int p = 0; p < 3; ++p) { \
                                                a -= p * 3; \
                                                if (a > 10) { \
                                                    a /= 2; \
                                                    for (int q = 0; q < 4; ++q) { \
                                                        a += q * 4; \
                                                        while (a % 5 != 0) { \
                                                            a -= 1; \
                                                            JUNK_CODE_FUNCTION_3(); \
                                                            JUNK_CODE_FUNCTION_1(); \
                                                            for (int r = 0; r < 2; ++r) { \
                                                                a += r * 3; \
                                                                if (a % 2 == 1) { \
                                                                    a -= 1; \
                                                                    JUNK_CODE_FUNCTION_6(); \
                                                                    JUNK_CODE_FUNCTION_1(); \
                                                                } \
                                                            } \
                                                        } \
                                                    } \
                                                } else { \
                                                    a += 4; \
                                                                    JUNK_CODE_FUNCTION_2(); \
                                                                    JUNK_CODE_FUNCTION_1(); \
                                                } \
                                            } \
                                        } \
                                    } \
                                } else { \
                                    a -= 2; \
                                } \
                            } \
                        } else { \
                            a -= 5; \
                        } \
                    } \
                } else { \
                    a += 5; \
                    if (a > 25) { \
                        a /= 2; \
                        for (int s = 0; s < 2; ++s) { \
                            a += s * 3; \
                            while (a % 2 != 0) { \
                                a -= 1; \
                            } \
                            if (a > 15) { \
                                a -= 4; \
                                if (a % 5 == 0) { \
                                    a += 5; \
                                    for (int t = 0; t < 2; ++t) { \
                                        a -= t * 2; \
                                        while (a % 3 != 0) { \
                                            a += 2; \
                                        } \
                                        if (a < 30) { \
                                            a *= 2; \
                                            for (int u = 0; u < 3; ++u) { \
                                                a -= u * 3; \
                                                if (a > 10) { \
                                                    a /= 2; \
                                                    for (int v = 0; v < 4; ++v) { \
                                                        a += v * 4; \
                                                        while (a % 5 != 0) { \
                                                            a -= 1; \
                                                            JUNK_CODE_FUNCTION_1(); \
                                                            JUNK_CODE_FUNCTION_1(); \
                                                            JUNK_CODE_FUNCTION_1(); \
                                                        } \
                                                    } \
                                                } else { \
                                                    a += 4; \
                                                } \
                                            } \
                                        } \
                                    } \
                                } \
                            } else { \
                                a += 2; \
                            } \
                        } \
                    } else { \
                        a *= 2; \
                                                            JUNK_CODE_FUNCTION_3(); \
                                                            JUNK_CODE_FUNCTION_1(); \
                                                            JUNK_CODE_FUNCTION_5(); \
                    } \
                } \
            } \
        } \
    } \
}

#define JUNK_CODE_HARD_COMPLEXITY() \
{ \
        int N = 5; \
        if (N % 2 == 0) { \
            for (int u = 0; u < N; ++u) { \
            int randomVar6 = 51; \
            for (int v = 0; v < 6; ++v) { \
                int w = v + 2; \
                do { \
                    w += 3; \
                    if (w == 11) { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_1(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                        int innerVar4 = w * randomVar6 / 2; \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_3(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_4(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    } \
                } while (w < 15); \
            } \
            \
            if (randomVar6 > 25) { \
                int x = randomVar6 / 4; \
                do { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_4(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    x -= 2; \
                } while (x > 0); \
            } \
            \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_2(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
        } \
        } else { \
            for (int y = 0; y < N; ++y) { \
            int randomVar2 = 68; \
            for (int z = 0; z < 7; ++z) { \
                int a = z + 4; \
                do { \
                    a += 4; \
                    if (a == 16) { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_6(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                        int innerVar2 = a * randomVar2 / 3; \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_6(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_5(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    } \
                } while (a < 20); \
            } \
            \
            if (randomVar2 > 40) { \
                int b = randomVar2 / 5; \
                do { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_3(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_6(); \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    b -= 3; \
                } while (b > 0); \
            } \
            \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_4(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_3(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    JUNK_CODE_FUNCTION_1();\
                    JUNK_CODE_FUNCTION_5();\
                    JUNK_CODE_FUNCTION_6();\
        } \
}\
}

#define JUNK_CODE_SUPERHARD_COMPLEXITY() \
{ \
        int N = 6; \
if (N % 2 == 0) { \
            for (int q = 0; q < N; ++q) { \
            int randomVar4 = 77; \
            for (int p = 0; p < 8; ++p) { \
                int t = p + 6; \
                do { \
                    t += 5; \
                    if (t == 18) { \
                        JUNK_CODE_FUNCTION_1(); \
                        int innerVar1 = t * randomVar4 / 3; \
                        for (int q = 0; q < N; ++q) { \
                            int randomVar4 = 77; \
                            for (int p = 0; p < 6; ++p) { \
                                int t = p + 3; \
                                do { \
                                    t += 2; \
                                    if (t == 11) { \
                                        JUNK_CODE_FUNCTION_1(); \
                                        int innerVar1 = t * randomVar4 / 3; \
                                    } \
                                } while (t < 18); \
                            } \
                            if (randomVar4 > 35) { \
                                int r = randomVar4 / 5; \
                                do { \
                                    JUNK_CODE_FUNCTION_1(); \
                                    r -= 4; \
                                } while (r > 0); \
                            } \
                        } \
                    } \
                } while (t < 30); \
            } \
            \
            if (randomVar4 > 35) { \
                int r = randomVar4 / 5; \
                do { \
                                for (int p = 0; p < 8; ++p) { \
                int t = p + 6; \
                do { \
                    t += 5; \
                    if (t == 18) { \
                        JUNK_CODE_FUNCTION_1(); \
                        int innerVar1 = t * randomVar4 / 3; \
                        for (int q = 0; q < N; ++q) { \
                            int randomVar4 = 77; \
                            for (int p = 0; p < 6; ++p) { \
                                int t = p + 3; \
                                do { \
                                    t += 2; \
                                    if (t == 11) { \
                                        JUNK_CODE_FUNCTION_1(); \
                                        int innerVar1 = t * randomVar4 / 3; \
                                    } \
                                } while (t < 18); \
                            } \
                            if (randomVar4 > 35) { \
                                int r = randomVar4 / 5; \
                                do { \
                                    JUNK_CODE_FUNCTION_1(); \
                                    r -= 4; \
                                } while (r > 0); \
                            } \
                        } \
                    } \
                } while (t < 30); \
            } \
                    r -= 4; \
                } while (r > 0); \
            } \
            \
                for (int p = 0; p < 8; ++p) { \
                int t = p + 6; \
                do { \
                    t += 5; \
                    if (t == 18) { \
                        JUNK_CODE_FUNCTION_1(); \
                        int innerVar1 = t * randomVar4 / 3; \
                        for (int q = 0; q < N; ++q) { \
                            int randomVar4 = 77; \
                            for (int p = 0; p < 6; ++p) { \
                                int t = p + 3; \
                                do { \
                                    t += 2; \
                                    if (t == 11) { \
                                        JUNK_CODE_FUNCTION_1(); \
                                        int innerVar1 = t * randomVar4 / 3; \
                                    } \
                                } while (t < 18); \
                            } \
                            if (randomVar4 > 35) { \
                                int r = randomVar4 / 5; \
                                do { \
                                    JUNK_CODE_FUNCTION_1(); \
                                    r -= 4; \
                                } while (r > 0); \
                            } \
                        } \
                    } \
                } while (t < 30); \
            } \
        } \
        } else { \
            for (int q = 0; q < N; ++q) { \
            int randomVar4 = 89; \
            for (int p = 0; p < 7; ++p) { \
                int t = p + 4; \
                do { \
                    t += 3; \
                    if (t == 14) { \
                        JUNK_CODE_FUNCTION_1(); \
                        int innerVar1 = t * randomVar4 / 2; \
                        JUNK_CODE_FUNCTION_1(); \
                    } \
                } while (t < 25); \
            } \
            \
            if (randomVar4 > 40) { \
                int r = randomVar4 / 6; \
                do { \
                    JUNK_CODE_FUNCTION_1(); \
                    r -= 3; \
                } while (r > 0); \
            } \
            \
                    JUNK_CODE_FUNCTION_1(); \
        } \
        } \
        if (N % 2 == 0) { \
            for (int u = 0; u < N; ++u) { \
            int randomVar6 = 51; \
            for (int v = 0; v < 6; ++v) { \
                int w = v + 2; \
                do { \
                    w += 3; \
                    if (w == 11) { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_1(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                        int innerVar4 = w * randomVar6 / 2; \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_3(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_4(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    } \
                } while (w < 15); \
            } \
            \
            if (randomVar6 > 25) { \
                int x = randomVar6 / 4; \
                do { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_4(); \
                                JUNK_CODE_FUNCTION_4(); \
                                JUNK_CODE_FUNCTION_4(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    x -= 2; \
                } while (x > 0); \
            } \
            \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_2(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
        } \
        } else { \
            for (int y = 0; y < N; ++y) { \
            int randomVar2 = 68; \
            for (int z = 0; z < 7; ++z) { \
                int a = z + 4; \
                do { \
                    a += 4; \
                    if (a == 16) { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_6(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_2(); \
                                JUNK_CODE_FUNCTION_2(); \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                        int innerVar2 = a * randomVar2 / 3; \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_6(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_5(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    } \
                } while (a < 20); \
            } \
            \
            if (randomVar2 > 40) { \
                int b = randomVar2 / 5; \
                do { \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_3(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_5(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_6(); \
                                JUNK_CODE_FUNCTION_2(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
                    b -= 3; \
                } while (b > 0); \
            } \
            \
                    for (int q = 0; q < N; ++q) { \
                        int randomVar4 = 7; \
                        for (int p = 0; p < 2; ++p) { \
                            int t = p + 1; \
                            do { \
                                t += 1; \
                                if (t == 2) { \
                                    JUNK_CODE_FUNCTION_5(); \
                                    JUNK_CODE_FUNCTION_4(); \
                                    int innerVar1 = t * randomVar4 / 2; \
                                } \
                            } while (t < 3); \
                        } \
                        if (randomVar4 > 5) { \
                            int r = randomVar4 / 2; \
                            do { \
                                JUNK_CODE_FUNCTION_1(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                JUNK_CODE_FUNCTION_3(); \
                                r -= 1; \
                            } while (r > 0); \
                        } \
                    } \
        } \
}\
}

inline VOID JunkCode()
{
    JUNK_CODE_SUPERHARD_COMPLEXITY();
    JUNK_CODE_HARD_COMPLEXITY()
}

inline void RandomJunkCodeCalls(int minCalls, int maxCalls) {
    int numCalls = rand() % (maxCalls - minCalls + 1) + minCalls;  

    for (int i = 0; i < numCalls; ++i) {
        JunkCode(); 
    }
}

#define PROTECT(func) \
    do { \
        const void* funcAddress = reinterpret_cast<const void*>(&(func)); \
        if (IsSuspiciousMemoryAccessDetected(funcAddress, 1)) { \
            BSOD(); \
        } \
        else { \
            (func)(); \
        } \
    } while (0)

#include <vector>
#include <string>
#include <windows.h>
#include <TlHelp32.h>

// ... existing code ... 