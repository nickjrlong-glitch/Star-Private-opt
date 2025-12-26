#pragma once
#include <windows.h>
#include <wininet.h>
#include <vector>
#include <string>

#pragma comment(lib, "wininet.lib")

class ImageLoader {
public:
    static std::vector<unsigned char> LoadImageFromURL(const std::string& url) {
        std::vector<unsigned char> imageData;
        
        HINTERNET hInternet = InternetOpenA("ImageLoader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            return imageData;
        }
        
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return imageData;
        }
        
        char buffer[4096];
        DWORD bytesRead;
        
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            imageData.insert(imageData.end(), buffer, buffer + bytesRead);
        }
        
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        
        return imageData;
    }
    
    static bool SaveImageToFile(const std::vector<unsigned char>& imageData, const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "wb");
        if (!file) {
            return false;
        }
        
        fwrite(imageData.data(), 1, imageData.size(), file);
        fclose(file);
        
        return true;
    }
};
