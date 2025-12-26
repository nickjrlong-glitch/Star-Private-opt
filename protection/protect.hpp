#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <thread>
#include "../security/antiattach.h"
#include "../security/antivm.h"

namespace Protection {
    namespace {
        volatile bool isInitialized = false;
        volatile bool isThreadRunning = false;
    }

    inline bool Initialize() {
        if (isInitialized) {
            return true;
        }

        // Anti-debug protection
        if (IsDebuggerPresent()) {
            exit(0);
        }

        // Anti-VM check from antivm.h
        if (IsInsideVM()) {
            exit(0);
        }

        // Anti-attach protection
        AntiAttach();

        isInitialized = true;
        return true;
    }

    inline bool ContinuousCheck() {
        if (isThreadRunning) {
            return true;
        }

        HANDLE hThread = CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            isThreadRunning = true;
            while (true) {
                Sleep(1000); // Sleep first to give program time to start

                if (IsDebuggerPresent()) {
                    ExitProcess(0);
                }

                if (IsInsideVM()) {
                    ExitProcess(0);
                }

                AntiAttach();
            }
            return 0;
        }, nullptr, 0, nullptr);

        if (hThread) {
            CloseHandle(hThread);
            return true;
        }

        return false;
    }
}

inline void protect() {
    // Anti-debug protection
    if (IsDebuggerPresent()) {
        exit(0);
    }

    // Anti-VM check from antivm.h
    if (IsInsideVM()) {
        exit(0);
    }

    // Anti-attach protection
    AntiAttach();
} 