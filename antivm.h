#pragma once
#include <Windows.h>
#include <winternl.h>

// Verwende die Windows-definierten Strukturen
using PUNICODE_STRING = _UNICODE_STRING*;

namespace AntiVM {
    inline bool CheckVM() {
        wchar_t deviceName[MAX_PATH];
        if (GetSystemDirectoryW(deviceName, MAX_PATH) == 0) {
            return true; // Fehler = verdächtig
        }

        const wchar_t* vmIdentifiers[] = {
            L"VIRTUAL",
            L"VMWARE",
            L"VBOX",
            L"QEMU",
            L"SANDBOX"
        };

        for (const auto& id : vmIdentifiers) {
            if (StrStrIW(deviceName, id)) {
                return true;
            }
        }

        return false;
    }
} 