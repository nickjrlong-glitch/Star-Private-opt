#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <random>
#include <chrono>
#include <ctime> // For time conversion
#include <comdef.h>
#include <Wbemidl.h>
#include <algorithm>
#include <cctype>
#include <Iphlpapi.h> // For MAC address
#include <fstream>

#include "skCrypter.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib") // For MAC address
#pragma comment(lib, "Rpcrt4.lib") // For UUID functions

namespace KeyAuth {

    // Helper function to query a property from WMI
    std::string get_wmi_property(IWbemServices* pSvc, const std::wstring& wmi_class, const std::wstring& property_name) {
        std::string result_str = "";
        IEnumWbemClassObject* pEnumerator = NULL;
        std::wstring query = L"SELECT " + property_name + L" FROM " + wmi_class;
        HRESULT hres = pSvc->ExecQuery(
            bstr_t(L"WQL"),
            bstr_t(query.c_str()),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hres)) {
            return ""; // Query failed
        }

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        if (pEnumerator) {
            hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (SUCCEEDED(hres) && uReturn != 0) {
                VARIANT vtProp;
                hres = pclsObj->Get(property_name.c_str(), 0, &vtProp, 0, 0);
                if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
                    _bstr_t bstr(vtProp.bstrVal, false);
                    result_str = (const char*)bstr;
                }
                VariantClear(&vtProp);
                pclsObj->Release();
            }
            pEnumerator->Release();
        }

        result_str.erase(std::remove_if(result_str.begin(), result_str.end(), [](unsigned char c) { return !std::isalnum(c); }), result_str.end());
        return result_str;
    }

    // Function to get a persistent unique identifier from the registry
    std::string get_or_create_persistent_uid() {
        HKEY hKey;
        LONG lRes = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ | KEY_WRITE, &hKey);

        if (lRes != ERROR_SUCCESS) {
            // Fallback if we can't even open the parent key
            return "regkeyfail1";
        }

        char szBuffer[256];
        DWORD dwBufferSize = sizeof(szBuffer);
        lRes = RegGetValueA(hKey, NULL, "FSTermID", RRF_RT_REG_SZ, NULL, (PVOID)&szBuffer, &dwBufferSize);

        if (lRes == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(szBuffer);
        }
        else {
            // Key not found, so we create it
            UUID uuid;
            UuidCreate(&uuid);
            unsigned char* str_uuid;
            UuidToStringA(&uuid, &str_uuid);
            std::string new_uid((char*)str_uuid);
            RpcStringFreeA(&str_uuid);

            RegSetValueExA(hKey, "FSTermID", 0, REG_SZ, (const BYTE*)new_uid.c_str(), new_uid.length() + 1);
            RegCloseKey(hKey);
            return new_uid;
        }
    }

    std::string get_hwid() {
        HW_PROFILE_INFOA hwProfileInfo;
        if (GetCurrentHwProfileA(&hwProfileInfo) != 0)
            return hwProfileInfo.szHwProfileGuid;
        return "no_hwid_found";
    }

    // Improved JSON value extractor
    std::string extract_value(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        // Find the colon after the key
        pos = json.find(':', pos + search.length());
        if (pos == std::string::npos) return "";

        // Skip whitespace
        pos++;
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
            pos++;

        if (pos >= json.length()) return "";

        if (json[pos] == '"') {
            // String value
            pos++;  // Skip opening quote
            std::string result;
            bool escaped = false;

            while (pos < json.length()) {
                char c = json[pos];
                if (!escaped) {
                    if (c == '\\') {
                        escaped = true;
                    }
                    else if (c == '"') {
                        break;  // End of string
                    }
                    else {
                        result += c;
                    }
                }
                else {
                    result += c;
                    escaped = false;
                }
                pos++;
            }
            return result;
        }
        else if (json[pos] == '{' || json[pos] == '[') {
            // Object or array - find matching closing bracket
            char open = json[pos];
            char close = (open == '{') ? '}' : ']';
            int depth = 1;
            std::string result;
            result += open;
            pos++;

            while (pos < json.length() && depth > 0) {
                char c = json[pos];
                if (c == open) depth++;
                else if (c == close) depth--;
                result += c;
                pos++;
            }
            return result;
        }
        else {
            // Number, boolean, or null
            std::string result;
            while (pos < json.length() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']' && json[pos] != ' ' && json[pos] != '\n' && json[pos] != '\r') {
                result += json[pos];
                pos++;
            }
            return result;
        }
    }

    // Helper to convert Unix timestamp to "X days left"
    std::string format_expiry(const std::string& expiry_timestamp) {
        try {
            OutputDebugStringA(("\nFormatting expiry timestamp: " + expiry_timestamp + "\n").c_str());

            if (expiry_timestamp.empty() || expiry_timestamp == "0") {
                return "Expired";
            }

            // Try to convert the timestamp
            long long expiry_ts = 0;
            try {
                // First, try to parse as Unix timestamp
                expiry_ts = std::stoll(expiry_timestamp);
                OutputDebugStringA("Parsed as Unix timestamp\n");
            }
            catch (...) {
                try {
                    // If that fails, try to parse as date string (YYYY-MM-DD)
                    if (expiry_timestamp.length() >= 10) {
                        std::tm tm = {};
                        std::istringstream ss(expiry_timestamp.substr(0, 10));
                        ss >> std::get_time(&tm, "%Y-%m-%d");
                        if (!ss.fail()) {
                            expiry_ts = std::mktime(&tm);
                            OutputDebugStringA("Parsed as date string\n");
                        }
                    }
                }
                catch (...) {
                    OutputDebugStringA("Failed to parse timestamp\n");
                    return "Invalid";
                }
            }

            if (expiry_ts == 0) {
                OutputDebugStringA("Zero timestamp\n");
                return "Invalid";
            }

            // Get current time
            std::time_t now = std::time(nullptr);
            OutputDebugStringA(("Current time: " + std::to_string(now) + "\n").c_str());
            OutputDebugStringA(("Expiry time: " + std::to_string(expiry_ts) + "\n").c_str());

            // Calculate difference in seconds
            double seconds_left = difftime(static_cast<time_t>(expiry_ts), now);
            OutputDebugStringA(("Seconds left: " + std::to_string(seconds_left) + "\n").c_str());

            if (seconds_left < 0) {
                return "Expired";
            }

            // Convert to days/hours/minutes
            int days_left = static_cast<int>(seconds_left / (60 * 60 * 24));
            OutputDebugStringA(("Days left: " + std::to_string(days_left) + "\n").c_str());

            if (days_left == 0) {
                // Less than a day left, show hours
                int hours_left = static_cast<int>(seconds_left / (60 * 60));
                if (hours_left == 0) {
                    // Less than an hour left, show minutes
                    int minutes_left = static_cast<int>(seconds_left / 60);
                    return std::to_string(minutes_left) + "m";
                }
                return std::to_string(hours_left) + "h";
            }

            return std::to_string(days_left) + "d";
        }
        catch (const std::exception& e) {
            OutputDebugStringA(("Exception in format_expiry: " + std::string(e.what()) + "\n").c_str());
            return "Error";
        }
        catch (...) {
            OutputDebugStringA("Unknown exception in format_expiry\n");
            return "Error";
        }
    }

    // Returns the motherboard serial number
    std::string get_motherboard_serial() {
        HRESULT hres;
        IWbemLocator* pLoc = NULL;
        IWbemServices* pSvc = NULL;
        std::string serial = "N/A";
        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) return serial;
        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (FAILED(hres) && hres != RPC_E_TOO_LATE) { CoUninitialize(); return serial; }
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) { CoUninitialize(); return serial; }
        hres = pLoc->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) { pLoc->Release(); CoUninitialize(); return serial; }
        serial = get_wmi_property(pSvc, L"Win32_BaseBoard", L"SerialNumber");
        pSvc->Release(); pLoc->Release(); CoUninitialize();
        return serial;
    }
    // Returns the disk serial number (first physical disk)
    std::string get_disk_serial() {
        HRESULT hres;
        IWbemLocator* pLoc = NULL;
        IWbemServices* pSvc = NULL;
        std::string serial = "N/A";
        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) return serial;
        hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (FAILED(hres) && hres != RPC_E_TOO_LATE) { CoUninitialize(); return serial; }
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) { CoUninitialize(); return serial; }
        hres = pLoc->ConnectServer(bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) { pLoc->Release(); CoUninitialize(); return serial; }
        serial = get_wmi_property(pSvc, L"Win32_DiskDrive", L"SerialNumber");
        pSvc->Release(); pLoc->Release(); CoUninitialize();
        return serial;
    }
    // Returns the MAC address of the first adapter
    std::string get_mac_address() {
        IP_ADAPTER_INFO AdapterInfo[16];
        DWORD buflen = sizeof(AdapterInfo);
        if (GetAdaptersInfo(AdapterInfo, &buflen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
            char mac[32] = { 0 };
            sprintf_s(mac, "%02X-%02X-%02X-%02X-%02X-%02X", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2], pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
            return std::string(mac);
        }
        return "N/A";
    }
    // Returns the persistent UUID from registry
    std::string get_uuid() {
        return get_or_create_persistent_uid();
    }

    class api {
    public:
        std::string name, ownerid, secret, version, url;
        std::string latest_version;
        std::string sessionid, enckey;
        bool initialized;
        std::vector<std::string> subscriptions;
        std::string key; // Store the key

        struct user_data_structure {
            std::string username;
            std::string expiry;
            std::string subscription;
        } user_data;

        api(const std::string& name, const std::string& ownerid, const std::string& secret, const std::string& version, const std::string& url)
            : name(name), ownerid(ownerid), secret(secret), version(version), url(url) {
        }

        std::string req(const std::string& data) {
            HINTERNET internet = InternetOpenA("KeyAuth", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!internet) {
                response.message = "Failed to open internet";
                return "";
            }

            HINTERNET connect = InternetConnectA(internet, "keyauth.win", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
            if (!connect) {
                response.message = "Failed to connect";
                InternetCloseHandle(internet);
                return "";
            }

            LPCSTR accept_types[] = { "text/html", "text/plain", NULL };
            HINTERNET request = HttpOpenRequestA(connect, "POST", "/api/1.2/", NULL, NULL, accept_types, INTERNET_FLAG_SECURE, 1);
            if (!request) {
                response.message = "Failed to open request";
                InternetCloseHandle(connect);
                InternetCloseHandle(internet);
                return "";
            }

            std::string headers = "Content-Type: application/x-www-form-urlencoded";
            if (!HttpSendRequestA(request, headers.c_str(), headers.length(), (LPVOID)data.c_str(), data.length())) {
                response.message = "Failed to send request";
                InternetCloseHandle(request);
                InternetCloseHandle(connect);
                InternetCloseHandle(internet);
                return "";
            }

            std::string ret;
            char buffer[4096];
            DWORD bytes_read;
            while (InternetReadFile(request, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
                ret.append(buffer, bytes_read);
            }

            InternetCloseHandle(request);
            InternetCloseHandle(connect);
            InternetCloseHandle(internet);
            return ret;
        }

        void init() {
            enckey = secret.substr(0, 16);
            std::string iv = secret.substr(16, 16);
            auto data = "type=init&ver=" + version + "&name=" + name + "&ownerid=" + ownerid;
            auto response_str = req(data);

            response.success = (extract_value(response_str, "success") == "true");
            response.message = extract_value(response_str, "message");

            if (response.success) {
                sessionid = extract_value(response_str, "sessionid");
            }
        }

        void license(const std::string& key) {
            this->key = key; // Store the key for later use
            std::string hwid = get_hwid();
            auto data = "type=license&key=" + key + "&hwid=" + hwid + "&sessionid=" + sessionid + "&name=" + name + "&ownerid=" + ownerid;
            auto response_str = req(data);

            response.success = (extract_value(response_str, "success") == "true");
            response.message = extract_value(response_str, "message");

            if (response.success) {
                // Find the subscriptions array
                size_t subStart = response_str.find("\"subscriptions\":[{");
                if (subStart != std::string::npos) {
                    // Find the first subscription object
                    size_t firstSub = response_str.find("{", subStart);
                    if (firstSub != std::string::npos) {
                        // Find the expiry field within this subscription
                        size_t expiryStart = response_str.find("\"expiry\":\"", firstSub);
                        if (expiryStart != std::string::npos) {
                            expiryStart += 9; // Skip past "expiry":"
                            size_t expiryEnd = response_str.find("\"", expiryStart);
                            if (expiryEnd != std::string::npos) {
                                user_data.expiry = response_str.substr(expiryStart, expiryEnd - expiryStart);
                            }
                        }
                    }
                }

                // If no expiry found, try to get it again
                if (user_data.expiry.empty() || user_data.expiry == "0") {
                    check_expiry();
                }
            }
        }

        void check_expiry() {
            if (!key.empty()) {
                std::string hwid = get_hwid();
                auto data = "type=license&key=" + key + "&hwid=" + hwid + "&sessionid=" + sessionid + "&name=" + name + "&ownerid=" + ownerid;
                auto response_str = req(data);

                if (response_str.find("\"success\":true") != std::string::npos) {
                    size_t subStart = response_str.find("\"subscriptions\":[{");
                    if (subStart != std::string::npos) {
                        size_t firstSub = response_str.find("{", subStart);
                        if (firstSub != std::string::npos) {
                            size_t expiryStart = response_str.find("\"expiry\":\"", firstSub);
                            if (expiryStart != std::string::npos) {
                                expiryStart += 9;
                                size_t expiryEnd = response_str.find("\"", expiryStart);
                                if (expiryEnd != std::string::npos) {
                                    user_data.expiry = response_str.substr(expiryStart, expiryEnd - expiryStart);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            user_data.expiry = "0"; // Set to expired if we couldn't get the expiry time
        }

        struct {
            bool success;
            std::string message;
        } response;
    };
}