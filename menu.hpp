#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <dwmapi.h>
#include <d3d9.h>
#include <cstdlib>
#include <XInput.h>
#include <windows.h>
#include <winuser.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include "settings/settings.hpp"
#include "game/aimbot/Aimbot.hpp"
#include "game/Gameloop.hpp"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx9.h"
// KeyAuth removed - no authentication needed
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "color.h"
#include "logo_bytes.hpp"

// d3dx9tex.h removed - using alternative texture loading
#include "blur.hpp"
#include "login_dialog.hpp"

// Helper function to create texture from memory data (replaces D3DXCreateTextureFromFileInMemoryEx)
HRESULT CreateTextureFromMemory(LPDIRECT3DDEVICE9 device, const void* data, UINT dataSize, UINT width, UINT height, LPDIRECT3DTEXTURE9* texture) {
    if (!device || !data || !texture) return E_INVALIDARG;
    
    HRESULT hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);
    if (FAILED(hr)) return hr;
    
    D3DLOCKED_RECT lockedRect;
    hr = (*texture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*texture)->Release();
        *texture = NULL;
        return hr;
    }
    
    // Simple implementation - just copy the data
    memcpy(lockedRect.pBits, data, min(dataSize, width * height * 4));
    
    (*texture)->UnlockRect(0);
    return S_OK;
}

// Helper function to create texture from PNG data using GDI+
// Forward declaration
HRESULT CreateTextureFromPNG_GDI(LPDIRECT3DDEVICE9 device, const void* pngData, UINT pngDataSize, LPDIRECT3DTEXTURE9* texture);
HRESULT CreateTextureFromURL(LPDIRECT3DDEVICE9 device, const wchar_t* url, LPDIRECT3DTEXTURE9* texture);

// PNG decoder using Windows GDI+
#include <gdiplus.h>
#include <urlmon.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "urlmon.lib")
using namespace Gdiplus;

// Function to create texture from PNG using GDI+
HRESULT CreateTextureFromPNG_GDI(LPDIRECT3DDEVICE9 device, const void* pngData, UINT pngDataSize, LPDIRECT3DTEXTURE9* texture) {
    if (!device || !pngData || !texture) return E_INVALIDARG;
    
    *texture = nullptr;
    
    // Initialize GDI+ (only once)
    static bool gdiplusInitialized = false;
    static ULONG_PTR gdiplusToken = 0;
    if (!gdiplusInitialized) {
        GdiplusStartupInput gdiplusStartupInput;
        gdiplusStartupInput.GdiplusVersion = 1;
        gdiplusStartupInput.DebugEventCallback = NULL;
        gdiplusStartupInput.SuppressBackgroundThread = FALSE;
        gdiplusStartupInput.SuppressExternalCodecs = FALSE;
        Status status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        if (status != Ok) {
            return E_FAIL;
        }
        gdiplusInitialized = true;
    }
    
    // Create stream from memory
    IStream* stream = NULL;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, pngDataSize);
    if (!hMem) return E_OUTOFMEMORY;
    
    void* pMem = GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        return E_OUTOFMEMORY;
    }
    memcpy(pMem, pngData, pngDataSize);
    GlobalUnlock(hMem);
    
    HRESULT hr = CreateStreamOnHGlobal(hMem, TRUE, &stream);
    if (FAILED(hr) || !stream) {
        return E_FAIL;
    }
    
    // Load image from stream
    Bitmap* bitmap = new Bitmap(stream);
    if (!bitmap || bitmap->GetLastStatus() != Ok) {
        if (bitmap) delete bitmap;
        stream->Release();
        return E_FAIL;
    }
    
    UINT width = bitmap->GetWidth();
    UINT height = bitmap->GetHeight();
    
    if (width == 0 || height == 0) {
        delete bitmap;
        stream->Release();
        return E_FAIL;
    }
    
    // Create texture with D3DFMT_A8R8G8B8 format
    hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);
    if (FAILED(hr)) {
        delete bitmap;
        stream->Release();
        return hr;
    }
    
    // Lock texture
    D3DLOCKED_RECT lockedRect;
    hr = (*texture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*texture)->Release();
        *texture = NULL;
        delete bitmap;
        stream->Release();
        return hr;
    }
    
    // Use LockBits for faster pixel access (like the other project does)
    BitmapData bmpData;
    Rect rect(0, 0, width, height);
    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Ok) {
        BYTE* src = (BYTE*)bmpData.Scan0;
        BYTE* dst = (BYTE*)lockedRect.pBits;
        
        for (UINT y = 0; y < height; y++) {
            BYTE* srcRow = src + y * bmpData.Stride;
            BYTE* dstRow = dst + y * lockedRect.Pitch;
            
            for (UINT x = 0; x < width; x++) {
                // GDI+ LockBits returns BGRA format; D3DFMT_A8R8G8B8 expects ARGB byte order when locked
                BYTE b = srcRow[x * 4 + 0];
                BYTE g = srcRow[x * 4 + 1];
                BYTE r = srcRow[x * 4 + 2];
                BYTE a = srcRow[x * 4 + 3];
                
                dstRow[x * 4 + 0] = a; // Alpha
                dstRow[x * 4 + 1] = r; // Red
                dstRow[x * 4 + 2] = g; // Green
                dstRow[x * 4 + 3] = b; // Blue
            }
        }
        
        bitmap->UnlockBits(&bmpData);
    } else {
        // Fallback: pixel-by-pixel (slower but more reliable)
        BYTE* dst = (BYTE*)lockedRect.pBits;
        for (UINT y = 0; y < height; y++) {
            BYTE* dstRow = dst + y * lockedRect.Pitch;
            for (UINT x = 0; x < width; x++) {
                Color color;
                if (bitmap->GetPixel(x, y, &color) == Ok) {
                    dstRow[x * 4 + 0] = color.GetA();
                    dstRow[x * 4 + 1] = color.GetR();
                    dstRow[x * 4 + 2] = color.GetG();
                    dstRow[x * 4 + 3] = color.GetB();
                } else {
                    dstRow[x * 4 + 0] = 0;
                    dstRow[x * 4 + 1] = 0;
                    dstRow[x * 4 + 2] = 0;
                    dstRow[x * 4 + 3] = 0;
                }
            }
        }
    }
    
    (*texture)->UnlockRect(0);
    
    delete bitmap;
    stream->Release();
    
    return S_OK;
}

// Function to create texture from URL (downloads image and creates texture)
HRESULT CreateTextureFromURL(LPDIRECT3DDEVICE9 device, const wchar_t* url, LPDIRECT3DTEXTURE9* texture) {
    if (!device || !url || !texture) return E_INVALIDARG;
    
    *texture = nullptr;
    
    // Initialize GDI+ (only once)
    static bool gdiplusInitialized = false;
    static ULONG_PTR gdiplusToken = 0;
    if (!gdiplusInitialized) {
        GdiplusStartupInput gdiplusStartupInput;
        gdiplusStartupInput.GdiplusVersion = 1;
        gdiplusStartupInput.DebugEventCallback = NULL;
        gdiplusStartupInput.SuppressBackgroundThread = FALSE;
        gdiplusStartupInput.SuppressExternalCodecs = FALSE;
        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
            return E_FAIL;
        }
        gdiplusInitialized = true;
    }
    
    // Download image from URL to temporary file
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t tempFile[MAX_PATH];
    wcscpy_s(tempFile, tempPath);
    wcscat_s(tempFile, L"logo_temp.png");
    
    // Delete old temp file if exists
    DeleteFileW(tempFile);
    
    // Download file from URL
    HRESULT hr = URLDownloadToFileW(NULL, url, tempFile, 0, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Load image directly from file using GDI+
    Bitmap* bitmap = new Bitmap(tempFile);
    if (!bitmap || bitmap->GetLastStatus() != Ok) {
        if (bitmap) delete bitmap;
        DeleteFileW(tempFile);
        return E_FAIL;
    }
    
    UINT width = bitmap->GetWidth();
    UINT height = bitmap->GetHeight();
    
    if (width == 0 || height == 0) {
        delete bitmap;
        DeleteFileW(tempFile);
        return E_FAIL;
    }
    
    // Create texture with D3DFMT_A8R8G8B8 format
    hr = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, texture, NULL);
    if (FAILED(hr)) {
        delete bitmap;
        DeleteFileW(tempFile);
        return hr;
    }
    
    // Lock texture
    D3DLOCKED_RECT lockedRect;
    hr = (*texture)->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr)) {
        (*texture)->Release();
        *texture = NULL;
        delete bitmap;
        DeleteFileW(tempFile);
        return hr;
    }
    
    // Use LockBits for faster pixel access
    BitmapData bmpData;
    Rect rect(0, 0, width, height);
    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Ok) {
        BYTE* src = (BYTE*)bmpData.Scan0;
        BYTE* dst = (BYTE*)lockedRect.pBits;
        
        for (UINT y = 0; y < height; y++) {
            BYTE* srcRow = src + y * bmpData.Stride;
            BYTE* dstRow = dst + y * lockedRect.Pitch;
            
            for (UINT x = 0; x < width; x++) {
                // GDI+ LockBits returns BGRA format
                BYTE b = srcRow[x * 4 + 0];
                BYTE g = srcRow[x * 4 + 1];
                BYTE r = srcRow[x * 4 + 2];
                BYTE a = srcRow[x * 4 + 3];
                
                // D3DFMT_A8R8G8B8 format: A, R, G, B (byte order)
                dstRow[x * 4 + 0] = a; // Alpha
                dstRow[x * 4 + 1] = r; // Red
                dstRow[x * 4 + 2] = g; // Green
                dstRow[x * 4 + 3] = b; // Blue
            }
        }
        
        bitmap->UnlockBits(&bmpData);
    } else {
        // Fallback: pixel-by-pixel
        BYTE* dst = (BYTE*)lockedRect.pBits;
        for (UINT y = 0; y < height; y++) {
            BYTE* dstRow = dst + y * lockedRect.Pitch;
            for (UINT x = 0; x < width; x++) {
                Color color;
                if (bitmap->GetPixel(x, y, &color) == Ok) {
                    dstRow[x * 4 + 0] = color.GetA(); // Alpha
                    dstRow[x * 4 + 1] = color.GetR(); // Red
                    dstRow[x * 4 + 2] = color.GetG(); // Green
                    dstRow[x * 4 + 3] = color.GetB(); // Blue
                } else {
                    dstRow[x * 4 + 0] = 0;
                    dstRow[x * 4 + 1] = 0;
                    dstRow[x * 4 + 2] = 0;
                    dstRow[x * 4 + 3] = 0;
                }
            }
        }
    }
    
    (*texture)->UnlockRect(0);
    
    delete bitmap;
    DeleteFileW(tempFile); // Clean up temp file
    
    return S_OK;
}
#include "blur_x.h"
#include "blur_y.h"
#include "keyauth.hpp"
#include "ubuntu_medium.h"
#include "ubunutu_regular.h"
#include "icon_font.h"
#include "ubuntu_bold.h"
#include "cruin.h"
static LPDIRECT3D9              g_pD3 = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevic = NULL;
static D3DPRESENT_PARAMETERS    g_d3dp = {};

float tab_alpha = 0.f, subtab_alpha = 0.f;
float keybind_alpha = 0.0f;
float tab_add, subtab_add;
int active_tab = 0, active_subtab = 0;

int tabs = 0, sub_tabs = 0;

float child_sliding = 40.f;
int key0;
static bool check_0 = true;
bool check_1 = false;
int slider_one, slider_two = 50;
float slider_three = 0.0f;
char input[64] = { "" };
const char* items[]{ "Value", "Random" };
float color_edit[4] = { 64 / 255.f, 77 / 255.f, 236 / 255.f, 190 / 255.f };
int selectedItem = 0;
float alpha_line = 0.0f;
const char* items1[4]{ "Uno", "Dos", "Tres", "Quatro" };
int selectedItem1 = 0;

bool esp_preview = false;
float preview_alpha;

const char* multi_items[5] = { "One", "Two", "Three", "Four", "Five" };
bool multi_items_count[5];

#define WIDTH 850
#define HEIGHT 620
IDirect3DTexture9* logo_img = nullptr;
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "xinput.lib")
ImFont* ico_0;
ImFont* ico_1;
ImFont* cruin_0;
ImFont* ubu_1;
ImFont* ubu_0;
ImFont* ubu_2;
ImFont* ubu_preview;
ImFont* ubu_child;
ImFont* weapon_font;


// KeyAuth removed - no authentication needed

void CustomStyleColor() // Îòðèñîâêà öâåòîâ
{
    ImGuiStyle& s = ImGui::GetStyle();

    s.Colors[ImGuiCol_WindowBg] = ImColor(12, 12, 12, 255);
    s.Colors[ImGuiCol_ChildBg] = ImColor(18, 18, 18, 255);
    s.Colors[ImGuiCol_PopupBg] = ImColor(18, 18, 18, 255);
    s.Colors[ImGuiCol_Text] = ImColor(220, 220, 230, 255);
    s.Colors[ImGuiCol_TextDisabled] = ImColor(130, 134, 150, 255);
    s.Colors[ImGuiCol_Border] = ImColor(18, 19, 27, 0);
    s.Colors[ImGuiCol_TextSelectedBg] = ImColor(105, 115, 175, 180);
    s.Colors[ImGuiCol_FrameBg] = ImColor(20, 20, 20, 255);
    s.Colors[ImGuiCol_FrameBgHovered] = ImColor(22, 22, 22, 255);
    s.Colors[ImGuiCol_FrameBgActive] = ImColor(24, 24, 24, 255);
    s.Colors[ImGuiCol_SliderGrab] = ImColor(90, 124, 255, 255);
    s.Colors[ImGuiCol_SliderGrabActive] = ImColor(90, 124, 255, 255);
    s.Colors[ImGuiCol_Button] = ImColor(90, 124, 255, 255);
    s.Colors[ImGuiCol_ButtonHovered] = ImColor(100, 134, 255, 255);
    s.Colors[ImGuiCol_ButtonActive] = ImColor(80, 114, 235, 255);
    s.Colors[ImGuiCol_ScrollbarBg] = ImColor(20, 20, 20, 255);
    s.Colors[ImGuiCol_ScrollbarGrab] = ImColor(60, 60, 70, 255);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(80, 80, 90, 255);
    s.Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(100, 100, 110, 255);
    s.WindowBorderSize = 0;
    s.WindowPadding = ImVec2(0, 0);
    s.ChildRounding = 20;
    s.PopupRounding = 20;
    s.PopupBorderSize = 0;
    s.WindowRounding = 20.f;
    s.FrameBorderSize = 0.0f;
    s.FrameRounding = 12.f;
    s.ScrollbarSize = 8.f;
    s.ScrollbarRounding = 4.f;
    s.FramePadding = ImVec2(6, 3);
    s.ItemInnerSpacing = ImVec2(6, 0); // Reduced spacing between text and checkbox
    s.ItemSpacing = ImVec2(0, 8); // Closer vertical spacing

}
int togle = 0;
static float anim_text = 0.f;

// Font definitions removed - using ImGui default font instead
inline std::string format_expiry(const std::string& expiry_str) {
    try {
        // Convert string to long long
        long long expiry = std::stoll(expiry_str);

        // If expiry is 0, return "Expired"
        if (expiry == 0) return "Expired";

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        // Calculate time difference in seconds
        long long time_diff = expiry - now_seconds;

        // If expired
        if (time_diff <= 0) return "Expired";

        // Calculate days, hours, minutes
        int days = static_cast<int>(time_diff / (24 * 3600));
        time_diff %= (24 * 3600);
        int hours = static_cast<int>(time_diff / 3600);
        time_diff %= 3600;
        int minutes = static_cast<int>(time_diff / 60);

        // Format the output
        std::stringstream ss;
        if (days > 0) ss << days << "d ";
        if (hours > 0) ss << hours << "h ";
        if (minutes > 0) ss << minutes << "m";

        std::string result = ss.str();
        if (result.empty()) return "< 1m";
        return result;
    }
    catch (const std::exception&) {
        return "Error";
    }
}

// Forward declarations

// Font initialization function - simplified since we use ImGui default font
void initialize_menu_fonts() {
    // No need to initialize custom fonts, using ImGui default font
    return;
}

// Radar function
void Radar()
{
	if (settings::radar::radar)
	{
		ImVec2 radarPos;
		ImVec2 radarSize(200, 200);
		
		// Radar-Position aus den Einstellungen laden oder Standard-Position verwenden
		radarPos.x = settings::radar_position_x;
		radarPos.y = settings::radar_position_y;
		
		ImGui::SetNextWindowSize(radarSize);
		ImGui::SetNextWindowPos(radarPos);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(10, 10, 10, 155));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		
		// Radar-Fenster fest positioniert (mit NoMove Flag)
		ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
		{
			ImVec2 winpos = radarPos;
			ImVec2 winsize = radarSize;
			
			if (settings::radar::cross)
			{
				ImGui::GetWindowDrawList()->AddLine(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y), ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y), ImGui::GetColorU32(ImGuiCol_Border), 1.0f);
				ImGui::GetWindowDrawList()->AddLine(ImVec2(winpos.x, winpos.y + winsize.y * 0.5f), ImVec2(winpos.x + winsize.x, winpos.y + winsize.y * 0.5f), ImGui::GetColorU32(ImGuiCol_Border), 1.0f);
			}

			if (settings::radar::local_player)
			{
				ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y * 0.5f), 2.0f, ImColor(255, 255, 255, 255), 64);
			}
			
			// Spieler werden direkt in der Gameloop auf dem Radar gerendert
		}

		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}

// Helper function to generate a random string
std::string random_string(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

void AdjustPlayerSize();

IDirect3D9Ex* p_object = NULL;
IDirect3DDevice9Ex* p_device = NULL;
D3DPRESENT_PARAMETERS p_params = { NULL };
MSG messager = { NULL };
HWND my_wnd = NULL;
HWND game_wnd = NULL;

int getFps()
{
    using namespace std::chrono;
    static int count = 0;
    static auto last = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    static int fps = 0;

    count++;

    if (duration_cast<milliseconds>(now - last).count() > 1000)
    {
        fps = count;
        count = 0;
        last = now;
    }

    return fps;
}

// Helper function for central titles in sub-menus
auto drawSectionTitle = [](const char* title) {
    // Nutze explizit die große, fette Schrift für Überschriften
    if (ImGui::GetIO().Fonts->Fonts.Size > 1) {
        ImGui::PushFont(ubu_preview);
    }
    else {
        ImGui::PushFont(ubu_preview);
    }
    ImGui::SetWindowFontScale(1.2f);
    float textWidth = ImGui::CalcTextSize(title).x * 1.2f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.25f, 0.65f, 1.0f, 1.0f), title); // Helles Blau
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImVec2 lineStart = ImGui::GetCursorPos();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
        ImVec2(ImGui::GetWindowPos().x + lineStart.x, ImGui::GetWindowPos().y + lineStart.y),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y + lineStart.y + 2),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.65f, 1.0f, 1.0f)),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.75f, 1.0f, 1.0f)),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.75f, 1.0f, 1.0f)),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.65f, 1.0f, 1.0f))
    );
    };

// HELFER FÜR DEN COLOR PICKER - FINALER FIX FÜR TOOLTIP-PROBLEM



HRESULT directx_init()
{
    if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_object))) exit(3);

    ZeroMemory(&p_params, sizeof(p_params));
    p_params.Windowed = TRUE;
    p_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    p_params.hDeviceWindow = my_wnd;
    p_params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
    p_params.BackBufferFormat = D3DFMT_A8R8G8B8;
    p_params.BackBufferWidth = settings::width;
    p_params.BackBufferHeight = settings::height;
    p_params.EnableAutoDepthStencil = TRUE;
    p_params.AutoDepthStencilFormat = D3DFMT_D16;
    p_params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (FAILED(p_object->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, my_wnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p_params, 0, &p_device)))
    {
        p_object->Release();
        exit(4);
    }

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(my_wnd);
    ImGui_ImplDX9_Init(p_device);

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImFontAtlas* fontAtlas = io.Fonts;

    fontAtlas->Clear(); // Clear any previously loaded fonts to start fresh

    // 1. Load the main font (Segoe UI) with default glyphs
    ImFontConfig segoeUiConfig;
    segoeUiConfig.FontDataOwnedByAtlas = false;
    segoeUiConfig.MergeMode = false; // Not merging yet
    segoeUiConfig.PixelSnapH = true;

    ImFont* mainFont = fontAtlas->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &segoeUiConfig, io.Fonts->GetGlyphRangesDefault());
    if (!mainFont) {
        mainFont = fontAtlas->AddFontDefault(&segoeUiConfig);
    }

    // NEU: Fette, größere Schrift für Überschriften explizit laden
    ImFont* sectionTitleFont = fontAtlas->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeuib.ttf", 20.0f, &segoeUiConfig, io.Fonts->GetGlyphRangesDefault());
    if (!sectionTitleFont) {
        // Fallback auf normalen Font, falls Bold nicht vorhanden
        sectionTitleFont = fontAtlas->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 20.0f, &segoeUiConfig, io.Fonts->GetGlyphRangesDefault());
    }
  
    // 2. Load a secondary font specifically for the checkmark and merge it
    // This ensures the checkmark glyph is always available from a reliable source.
    ImFontConfig mergeConfig;
    mergeConfig.FontDataOwnedByAtlas = false; // Default font data is usually owned by atlas
    mergeConfig.MergeMode = true; // IMPORTANT: Merge into the existing font atlas
    mergeConfig.PixelSnapH = true;

    // Define glyph ranges ONLY for the checkmark symbol (U+2714)
    static const ImWchar checkmark_glyph_range[] = { 0x2714, 0x2714, 0 };

    // Load a default ImGui font instance (which is known to have basic symbols) and merge it with the main font
    fontAtlas->AddFontDefault(&mergeConfig);

    io.Fonts->Build(); // Rebuild font atlas after all fonts are added/merged
    io.IniFilename = NULL;

    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
 
    // Farben wie auf Bild 2 (dunkel, lila Akzent)

    ImGui::StyleColorsDark();
    cruin_0 = io.Fonts->AddFontFromMemoryTTF(&cruin, sizeof cruin, 25, NULL, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryTTF(&ubuntu_1, sizeof ubuntu_1, 15, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ico_0 = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 21, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ico_1 = io.Fonts->AddFontFromMemoryTTF(&icon, sizeof icon, 28, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ubu_0 = io.Fonts->AddFontFromMemoryTTF(&ubuntu_0, sizeof ubuntu_0, 18, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ubu_1 = io.Fonts->AddFontFromMemoryTTF(&ubuntu_0, sizeof ubuntu_0, 28, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ubu_2 = io.Fonts->AddFontFromMemoryTTF(&ubuntu_2, sizeof ubuntu_2, 25, NULL, io.Fonts->GetGlyphRangesCyrillic());
    cruin_0 = io.Fonts->AddFontFromMemoryTTF(&cruin, sizeof cruin, 30, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ubu_child = io.Fonts->AddFontFromMemoryTTF(&ubuntu_1, sizeof ubuntu_1, 22, NULL, io.Fonts->GetGlyphRangesCyrillic());
    ubu_preview = io.Fonts->AddFontFromMemoryTTF(&ubuntu_1, sizeof ubuntu_1, 12, NULL, io.Fonts->GetGlyphRangesCyrillic());
    CustomStyleColor();
    
    // ImGui Device Objects erstellen (wichtig für Texturen)
    ImGui_ImplDX9_CreateDeviceObjects();
    
    // Logo initialisieren - verwende eingebettetes Byte-Array (zuverlässiger als URL)
    // WICHTIG: Nach CreateDeviceObjects laden, damit ImGui bereit ist
    if (logo_img == nullptr && p_device != nullptr) {
        // Versuche es mehrmals mit Delays
        for (int attempt = 0; attempt < 5 && logo_img == nullptr; attempt++) {
            if (attempt > 0) {
                Sleep(200);
            }
            HRESULT hr = CreateTextureFromPNG_GDI(p_device, logo_bytes, logo_bytes_size, &logo_img);
            if (SUCCEEDED(hr) && logo_img != nullptr) {
                break; // Erfolgreich geladen
            }
        }
        
        // Falls PNG-Laden fehlschlägt, erstelle eine einfache Test-Textur (50x50, lila)
        if (logo_img == nullptr) {
            HRESULT hr = p_device->CreateTexture(50, 50, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &logo_img, NULL);
            if (SUCCEEDED(hr) && logo_img != nullptr) {
                D3DLOCKED_RECT lockedRect;
                if (SUCCEEDED(logo_img->LockRect(0, &lockedRect, NULL, 0))) {
                    BYTE* pixels = (BYTE*)lockedRect.pBits;
                    for (int y = 0; y < 50; y++) {
                        for (int x = 0; x < 50; x++) {
                            int idx = y * lockedRect.Pitch + x * 4;
                            pixels[idx + 0] = 255; // Alpha
                            pixels[idx + 1] = 200; // Red
                            pixels[idx + 2] = 100; // Green
                            pixels[idx + 3] = 255; // Blue (lila Farbe)
                        }
                    }
                    logo_img->UnlockRect(0);
                }
            }
        }
    }
    p_object->Release();

    return S_OK;
}

void create_overlay()
{
    // Create a standard topmost, click-through, transparent window (no hijack)
    std::string window_class = random_string(16);
    // Use a fixed, user-friendly window title
    std::wstring window_title_w = L"Optimal public";
    std::wstring window_class_w(window_class.begin(), window_class.end());

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = DefWindowProcW;
    wcex.hInstance = GetModuleHandleW(nullptr);
    wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.lpszClassName = window_class_w.c_str();
    wcex.hbrBackground = nullptr;
    if (!RegisterClassExW(&wcex)) {
        return;
    }

    RECT desktop_rect{};
    GetWindowRect(GetDesktopWindow(), &desktop_rect);
    int width = desktop_rect.right - desktop_rect.left;
    int height = desktop_rect.bottom - desktop_rect.top;

    my_wnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        window_class_w.c_str(),
        window_title_w.c_str(),
        WS_POPUP,
        desktop_rect.left,
        desktop_rect.top,
        width,
        height,
        nullptr,
        nullptr,
        wcex.hInstance,
        nullptr);

    if (!my_wnd) {
        return;
    }

    // Make fully transparent background; drawing via ImGui will render as needed
    SetLayeredWindowAttributes(my_wnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(my_wnd, &margin);
    // Ensure the window title is set (even if styles change later)
    SetWindowTextW(my_wnd, L"Optimal public");
    ShowWindow(my_wnd, SW_SHOWDEFAULT);
    UpdateWindow(my_wnd);
}

void features()
{
    AdjustPlayerSize();
    try {
        if (settings::aimbot::enable && settings::aimbot::aimbot_type == 0)
        {
            // The memoryaim call is now handled within actorLoop(), so this direct call is removed.
            // if ((settings::aimbot::mouseAim && GetAsyncKeyState(settings::aimbot::current_key)) || settings::aimbot::controller_aim_pressed)
            // {
            //     memoryaim(cache::closest_mesh);
            // }

            if (settings::aimbot::crosshair)
            {
                if (settings::aimbot::gay_mode_crosshair)
                {
                    // Gay Mode: Rotating 卍 symbol with rainbow colors
                    static float rotation_angle = 0.0f;
                    rotation_angle += ImGui::GetIO().DeltaTime * settings::aimbot::gay_crosshair_speed * 60.0f; // Rotate speed (degrees per second)
                    if (rotation_angle > 360.0f) rotation_angle -= 360.0f;
                    
                    float center_x = GetSystemMetrics(0) / 2.0f;
                    float center_y = GetSystemMetrics(1) / 2.0f;
                    float base_size = settings::aimbot::gay_crosshair_size;
                    float radius = base_size * 0.6f;
                    
                    // Get rainbow color based on time
                    static float rainbow_time = 0.0f;
                    rainbow_time += ImGui::GetIO().DeltaTime * 2.0f;
                    if (rainbow_time > 1.0f) rainbow_time -= 1.0f;
                    
                    // Draw rotating 卍 symbol
                    #ifndef M_PI
                    #define M_PI 3.14159265358979323846
                    #endif
                    float angle_rad = rotation_angle * (float)M_PI / 180.0f;
                    float arm_length = base_size * 0.5f;
                    float arm_width = base_size * 0.15f;
                    float perp_length = base_size * 0.3f;
                    
                    // Draw outer circle background with glow effect
                    ImGui::GetForegroundDrawList()->AddCircle(ImVec2(center_x, center_y), radius + base_size * 0.2f, ImColor(0, 0, 0, 180), 64, 3.0f);
                    ImGui::GetForegroundDrawList()->AddCircle(ImVec2(center_x, center_y), radius + base_size * 0.15f, ImColor(0, 0, 0, 220), 64, 2.0f);
                    
                    // Draw center circle
                    ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(center_x, center_y), base_size * 0.2f, ImColor(0, 0, 0, 240), 32);
                    
                    // Draw 4 arms with rotation - improved design
                    for (int i = 0; i < 4; i++) {
                        float arm_angle = angle_rad + (i * M_PI / 2.0f);
                        
                        // Main arm (from center outward)
                        float start_x = center_x + cosf(arm_angle) * (base_size * 0.15f);
                        float start_y = center_y + sinf(arm_angle) * (base_size * 0.15f);
                        float end_x = center_x + cosf(arm_angle) * radius;
                        float end_y = center_y + sinf(arm_angle) * radius;
                        
                        // Rotate color for each arm - Create rainbow color based on angle
                        float color_hue = fmodf((i * 90.0f) + (rainbow_time * 360.0f), 360.0f);
                        float color_time = color_hue / 360.0f;
                        float r = sinf(color_time * 2.0f * (float)M_PI) * 0.5f + 0.5f;
                        float g = sinf((color_time + 0.333f) * 2.0f * (float)M_PI) * 0.5f + 0.5f;
                        float b = sinf((color_time + 0.666f) * 2.0f * (float)M_PI) * 0.5f + 0.5f;
                        ImColor arm_color = ImColor((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
                        
                        // Draw main arm with shadow
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(start_x + 1, start_y + 1),
                            ImVec2(end_x + 1, end_y + 1),
                            ImColor(0, 0, 0, 150),
                            arm_width + 1.0f
                        );
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(start_x, start_y),
                            ImVec2(end_x, end_y),
                            arm_color,
                            arm_width
                        );
                        
                        // Draw perpendicular line at end of each arm (curved hook)
                        float perp_angle = arm_angle + M_PI / 2.0f;
                        float hook_start_x = end_x;
                        float hook_start_y = end_y;
                        float hook_mid_x = end_x + cosf(perp_angle) * perp_length * 0.5f;
                        float hook_mid_y = end_y + sinf(perp_angle) * perp_length * 0.5f;
                        float hook_end_x = end_x + cosf(perp_angle) * perp_length;
                        float hook_end_y = end_y + sinf(perp_angle) * perp_length;
                        
                        // Draw hook with shadow
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(hook_start_x + 1, hook_start_y + 1),
                            ImVec2(hook_end_x + 1, hook_end_y + 1),
                            ImColor(0, 0, 0, 150),
                            arm_width + 1.0f
                        );
                        ImGui::GetForegroundDrawList()->AddLine(
                            ImVec2(hook_start_x, hook_start_y),
                            ImVec2(hook_end_x, hook_end_y),
                            arm_color,
                            arm_width
                        );
                    }
                }
                else
                {
                    // Normal crosshair
                // Thicker, darker crosshair as shadow
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(GetSystemMetrics(0) / 2 - 9, GetSystemMetrics(1) / 2), ImVec2(GetSystemMetrics(0) / 2 + 9, GetSystemMetrics(1) / 2), ImColor(0, 0, 0, 180), 3.0f);
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(GetSystemMetrics(0) / 2, GetSystemMetrics(1) / 2 - 9), ImVec2(GetSystemMetrics(0) / 2, GetSystemMetrics(1) / 2 + 9), ImColor(0, 0, 0, 180), 3.0f);
                // Bright, accented crosshair on top
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(GetSystemMetrics(0) / 2 - 8, GetSystemMetrics(1) / 2), ImVec2(GetSystemMetrics(0) / 2 + 8, GetSystemMetrics(1) / 2), settings::colors::icCrosshairColor, 2.0f);
                ImGui::GetForegroundDrawList()->AddLine(ImVec2(GetSystemMetrics(0) / 2, GetSystemMetrics(1) / 2 - 8), ImVec2(GetSystemMetrics(0) / 2, GetSystemMetrics(1) / 2 + 8), settings::colors::icCrosshairColor, 2.0f);
                }
            }

            if (settings::aimbot::show_fov)
            {
                WeaponInfo weapon_info = get_weapon_info();
                if (weapon_info.smoothness > 0.0f) {
                    auto draw_list = ImGui::GetForegroundDrawList();
                    draw_list->AddCircle(
                        ImVec2(settings::screen_center_x, settings::screen_center_y),
                        settings::aimbot::fov,
                        settings::colors::icFovColor, 64, 1.5f);

                    if (settings::aimbot::fill_fov)
                        draw_list->AddCircleFilled(
                            ImVec2(settings::screen_center_x, settings::screen_center_y),
                            settings::aimbot::fov,
                            settings::colors::icFovFillColor, 64);
                }
            }
        }

        // actorLoop() immer aufrufen, damit ESP/Visuals immer funktionieren
        actorLoop();

        // World ESP (immer anzeigen)
        if (settings::world::enable)
        {
        }

        // Radar (immer anzeigen)
        if (settings::radar::radar)
        {
            Radar();
        }
    }
    catch (...) {
        // Fehler werden ignoriert, um Abstürze zu verhindern
    }
}

// Custom Slider with animated and cleaner design
bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f") {
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID(label);
    
    // Animation state for smooth value changes (only when not dragging)
    static std::map<ImGuiID, float> anim_map;
    auto it = anim_map.find(id);
    if (it == anim_map.end()) {
        anim_map[id] = *v;
        it = anim_map.find(id);
    }
    
    // Text label with padding from left
    ImVec2 cursor_start = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(cursor_start.x + 8.0f); // 8px padding from left
    ImGui::Text(label);
    
    // Ensure we're on a new line - move cursor to next line
    ImGui::NewLine();
    
    // Get window content region to position slider
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 window_content_min = window->ContentRegionRect.Min;
    ImVec2 window_content_max = window->ContentRegionRect.Max;
    ImVec2 current_cursor_screen = ImGui::GetCursorScreenPos();
    
    // Calculate value text width
    char value_buf[64];
    snprintf(value_buf, IM_ARRAYSIZE(value_buf), format, *v);
    float value_width = ImGui::CalcTextSize(value_buf).x;
    
    // Use smaller fixed width for shorter sliders
    // Calculate a reasonable fixed width (smaller than max)
    char max_value_buf[64];
    snprintf(max_value_buf, IM_ARRAYSIZE(max_value_buf), "%.0f m", v_max);
    float max_value_width = ImGui::CalcTextSize(max_value_buf).x;
    
    // Use a smaller fixed width (about 60% of max width for shorter sliders)
    float fixed_value_width = max_value_width * 0.6f;
    fixed_value_width = ImMax(value_width, fixed_value_width); // But at least current value width
    
    // Fixed margins - same for ALL sliders
    float margin_start = 10.0f;  // 10px from left edge
    float margin_end = 20.0f;   // 20px from right edge
    float value_spacing = 12.0f; // 12px spacing between slider end and value
    float slider_height = 16.0f;
    float track_height = 3.0f;
    float grab_radius = 7.0f;
    float min_slider_width = 50.0f;
    
    // SIMPLE CALCULATION - Calculate from RIGHT to LEFT
    // Step 1: Value position (using fixed width for consistent slider length)
    float value_end_x = window_content_max.x - margin_end;
    float value_start_x = value_end_x - fixed_value_width;
    
    // Step 2: Slider position (ends before value with spacing)
    float slider_end_x = value_start_x - value_spacing;
    float slider_start_x = window_content_min.x + margin_start;
    
    // Step 3: Ensure minimum slider width
    if (slider_end_x < slider_start_x + min_slider_width) {
        slider_end_x = slider_start_x + min_slider_width;
        value_start_x = slider_end_x + value_spacing;
        value_end_x = value_start_x + fixed_value_width;
    }
    
    // Step 4: Ensure value doesn't overflow
    if (value_end_x > window_content_max.x - margin_end) {
        value_end_x = window_content_max.x - margin_end;
        value_start_x = value_end_x - fixed_value_width;
        slider_end_x = value_start_x - value_spacing;
        if (slider_end_x < slider_start_x + min_slider_width) {
            slider_end_x = slider_start_x + min_slider_width;
        }
    }
    
    // Step 5: Final clamp - ensure slider never overlaps value
    float max_slider_end = value_start_x - value_spacing;
    slider_end_x = ImClamp(slider_end_x, slider_start_x + min_slider_width, max_slider_end);
    
    // Actual value position (for drawing, use actual value width)
    // Value should be positioned after slider with spacing
    float actual_value_start_x = slider_end_x + value_spacing;
    float actual_value_end_x = actual_value_start_x + value_width;
    
    // Ensure value is visible and doesn't go outside window
    if (actual_value_end_x > window_content_max.x - margin_end) {
        actual_value_end_x = window_content_max.x - margin_end;
        actual_value_start_x = actual_value_end_x - value_width;
    }
    
    float slider_screen_y = current_cursor_screen.y;
    ImRect slider_rect(slider_start_x, slider_screen_y, slider_end_x, slider_screen_y + slider_height);
    
    // Register item for interaction
    ImGuiID slider_id = window->GetID("##Slider");
    ImGui::ItemSize(slider_rect, ImGui::GetStyle().FramePadding.y);
    if (!ImGui::ItemAdd(slider_rect, slider_id)) {
        ImGui::PopID();
        return false;
    }
    
    // Handle interaction
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(slider_rect, slider_id, &hovered, &held, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnClickRelease);
    bool is_active = held;
    bool value_changed = false;
    
    // Handle click and drag
    if (pressed || (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float mouse_x = mouse_pos.x;
        float normalized = ImClamp((mouse_x - slider_rect.Min.x) / slider_rect.GetWidth(), 0.0f, 1.0f);
        float new_value = v_min + normalized * (v_max - v_min);
        new_value = ImClamp(new_value, v_min, v_max);
        if (new_value != *v) {
            *v = new_value;
            it->second = *v;
            value_changed = true;
        }
    }
    
    // Also handle continuous dragging when held
    if (is_active) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float mouse_x = mouse_pos.x;
        float normalized = ImClamp((mouse_x - slider_rect.Min.x) / slider_rect.GetWidth(), 0.0f, 1.0f);
        float new_value = v_min + normalized * (v_max - v_min);
        new_value = ImClamp(new_value, v_min, v_max);
        if (new_value != *v) {
            *v = new_value;
            it->second = *v;
            value_changed = true;
        }
    }
    
    // Smooth interpolation only when not dragging
    if (!is_active) {
        float anim_speed = 15.0f;
        float delta_time = ImGui::GetIO().DeltaTime;
        it->second = ImLerp(it->second, *v, ImMin(anim_speed * delta_time, 1.0f));
    } else {
        it->second = *v; // No animation during drag
    }
    
    // Calculate normalized value
    float normalized_value = ImClamp((it->second - v_min) / (v_max - v_min), 0.0f, 1.0f);
    float track_y_center = slider_rect.GetCenter().y;
    
    // Track bounds
    float track_left = slider_rect.Min.x;
    float track_right = slider_rect.Max.x;
    
    // Grab handle position
    float grab_x = track_left + grab_radius + normalized_value * (track_right - track_left - 2.0f * grab_radius);
    grab_x = ImClamp(grab_x, track_left + grab_radius, track_right - grab_radius);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw inactive track (dark gray-blue like in image) - fully rounded
    float track_rounding = track_height * 0.5f; // Fully rounded (half of height)
    draw_list->AddRectFilled(
        ImVec2(track_left, track_y_center - track_height * 0.5f),
        ImVec2(track_right, track_y_center + track_height * 0.5f),
        IM_COL32(50, 55, 70, 255), // Darker gray-blue
        track_rounding
    );
    
    // Draw active track (bright light blue like in image) - fully rounded on left, straight on right
    if (grab_x > track_left) {
        // Draw rounded rectangle for active track
        draw_list->AddRectFilled(
            ImVec2(track_left, track_y_center - track_height * 0.5f),
            ImVec2(grab_x, track_y_center + track_height * 0.5f),
            IM_COL32(100, 180, 255, 255), // Bright light blue
            track_rounding,
            ImDrawFlags_RoundCornersLeft // Only round left corners
        );
    }
    
    // Draw grab handle (white circle like in image)
    draw_list->AddCircleFilled(
        ImVec2(grab_x, track_y_center),
        grab_radius,
        IM_COL32(255, 255, 255, 255), // White
        16
    );
    
    // Draw value on the right - always draw it
    // Position value after slider with spacing
    float draw_value_x = slider_end_x + value_spacing;
    
    // Clamp to ensure it's visible
    if (draw_value_x + value_width > window_content_max.x - margin_end) {
        draw_value_x = window_content_max.x - margin_end - value_width;
    }
    if (draw_value_x < window_content_min.x + margin_start) {
        draw_value_x = window_content_min.x + margin_start;
    }
    
    ImGui::SetCursorScreenPos(ImVec2(draw_value_x, slider_screen_y));
    ImGui::TextColored(ImColor(220, 220, 230, 255), value_buf);
    
    // Move cursor to next line
    ImGui::SetCursorScreenPos(ImVec2(window_content_min.x, slider_screen_y + slider_height + 4.0f));
    
    ImGui::PopID();
    return value_changed;
}

// Helper function for custom toggle switch (FrozenFree style - red accent)
bool CustomToggle(const char* label, bool* v) {
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID(label);
    
    // Get dimensions
    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;
    
    // Animation state (per toggle using map)
    static std::map<ImGuiID, float> anim_map;
    auto it = anim_map.find(id);
    if (it == anim_map.end()) {
        anim_map[id] = *v ? 1.0f : 0.0f;
        it = anim_map.find(id);
    }
    
    // Smooth interpolation
    float target_t = *v ? 1.0f : 0.0f;
    float anim_speed = 8.0f;
    float delta_time = ImGui::GetIO().DeltaTime;
    it->second = ImLerp(it->second, target_t, ImMin(anim_speed * delta_time, 1.0f));
    
    // Get window bounds
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 window_content_min = window->ContentRegionRect.Min;
    ImVec2 window_content_max = window->ContentRegionRect.Max;
    
    // Text first (left side) with padding
    ImVec2 cursor_start = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(cursor_start.x + 8.0f);
    ImGui::Text(label);
    
    // Button directly next to text (small spacing)
    ImGui::SameLine(0, 12.0f);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Create invisible button
    ImGui::InvisibleButton(label, ImVec2(width, height));
    if (ImGui::IsItemClicked()) *v = !*v;
    
    // Get actual button position
    ImVec2 p = ImGui::GetItemRectMin();
    float button_y = p.y;
    
    // Ensure button is within bounds (margin from right edge)
    float margin_end = 10.0f;
    if (p.x + width > window_content_max.x - margin_end) {
        p.x = window_content_max.x - margin_end - width;
    }
    
    // Use animated t value
    float t = it->second;
    ImU32 col_bg = ImColor(40, 42, 54, 255);
    ImU32 col_circle = *v ? ImColor(90, 124, 255, 255) : ImColor(255, 255, 255, 255);
    
    // Draw button background at corrected position with more rounding
    float button_right = p.x + width;
    draw_list->AddRectFilled(ImVec2(p.x, button_y), ImVec2(button_right, button_y + height), col_bg, height * 0.6f);
    
    // Calculate and draw circle
    float circle_x = p.x + radius + t * (width - radius * 2.0f);
    circle_x = ImClamp(circle_x, p.x + radius, button_right - radius);
    draw_list->AddCircleFilled(ImVec2(circle_x, button_y + radius), radius - 1.5f, col_circle);
    
    // Add spacing after toggle
    ImGui::Spacing();
    
    ImGui::PopID();
    return *v;
}

// Custom Combo with arrow indicator (↓ when open, ↑ when closed)
bool CustomCombo(const char* label, int* current_item, const char* const items[], int items_count) {
    ImGui::PushID(label);
    bool value_changed = false;
    
    // Get window bounds to ensure combo stays within box
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 window_content_min = window->ContentRegionRect.Min;
    ImVec2 window_content_max = window->ContentRegionRect.Max;
    
    // Calculate available width with margin from right edge (same as toggles)
    float margin_end = 10.0f;
    float available_width = window_content_max.x - window_content_min.x - margin_end;
    
    // Set the width of the combo to leave margin from right edge
    ImGui::SetNextItemWidth(available_width);
    
    // Get the preview value
    const char* preview_value = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : (items_count > 0 ? items[0] : "");
    
    // Use BeginCombo with correct signature: (label, preview_value, val, multi, flags)
    // val = items_count as float, multi = false
    bool is_open = ImGui::BeginCombo(label, preview_value, (float)items_count, false, 0);
    
    // Set popup background to match current background
    if (is_open) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(18.f / 255.f, 18.f / 255.f, 18.f / 255.f, 255.f / 255.f));
    }
    
    
    if (is_open) {
        for (int i = 0; i < items_count; i++) {
            const bool is_selected = (i == *current_item);
            if (ImGui::Selectable(items[i], is_selected)) {
                *current_item = i;
                value_changed = true;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
        ImGui::PopStyleColor();
    }
    
    ImGui::PopID();
    return value_changed;
}

static void DrawEspPreviewCard(const ImVec2& top_left, const ImVec2& size)
{
    if (size.x <= 0.0f || size.y <= 0.0f)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (!draw_list)
        return;

    const float card_rounding = 20.0f;
    ImVec2 bottom_right(top_left.x + size.x, top_left.y + size.y);

    draw_list->AddRectFilled(top_left, bottom_right, ImColor(18, 18, 18, 255), card_rounding);
    draw_list->AddRect(top_left, bottom_right, ImColor(55, 55, 65, 220), card_rounding, 0, 1.5f);

    draw_list->AddCircleFilled(ImVec2(top_left.x + 32.0f, top_left.y + 32.0f), 14.0f, IM_COL32(210, 210, 220, 255), 32);

    ImFont* font = ImGui::GetFont();
    if (font)
    {
        draw_list->AddText(font, 15.0f, ImVec2(top_left.x + size.x - 100.0f, top_left.y + 20.0f), IM_COL32(255, 255, 255, 230), "Player");
    }

    ImVec2 box_min(top_left.x + size.x * 0.28f, top_left.y + size.y * 0.18f);
    ImVec2 box_max(top_left.x + size.x * 0.72f, top_left.y + size.y * 0.86f);
    float preview_rounding = (settings::visuals::boxType == boxType::rounded)
        ? ImClamp(settings::visuals::box_rounding, 0.0f, 25.0f)
        : 6.0f;

    if (settings::visuals::Filled_box || settings::visuals::Gradient_filled_box)
    {
        ImColor fill = settings::visuals::Gradient_filled_box
            ? ImColor(92, 58, 150, 140)
            : settings::colors::icFilledBoxColor;
        draw_list->AddRectFilled(box_min, box_max, fill, preview_rounding);
    }

    // Use blue color like "Software" text (90, 140, 255)
    ImColor esp_blue_color = ImColor(90, 140, 255, 255);
    
    if (settings::visuals::box)
    {
        if (settings::visuals::Outlined_box)
        {
            draw_list->AddRect(ImVec2(box_min.x - 2.0f, box_min.y - 2.0f),
                               ImVec2(box_max.x + 2.0f, box_max.y + 2.0f),
                               ImColor(0, 0, 0, 200),
                               preview_rounding + 2.0f,
                               0,
                               2.0f);
        }
        draw_list->AddRect(box_min, box_max, esp_blue_color, preview_rounding, 0, 2.0f);
    }

    ImVec2 head_center((box_min.x + box_max.x) * 0.5f, box_min.y + 24.0f);
    ImVec2 neck(head_center.x, head_center.y + 18.0f);
    ImVec2 chest(head_center.x, neck.y + 24.0f);
    ImVec2 pelvis(head_center.x, box_max.y - 36.0f);

    if (settings::visuals::skeleton)
    {
        ImColor skeleton_color = esp_blue_color;
        ImVec2 left_shoulder(head_center.x - 22.0f, chest.y - 6.0f);
        ImVec2 right_shoulder(head_center.x + 22.0f, chest.y - 6.0f);
        ImVec2 left_elbow(head_center.x - 34.0f, chest.y + 16.0f);
        ImVec2 right_elbow(head_center.x + 34.0f, chest.y + 16.0f);
        ImVec2 left_hand(head_center.x - 20.0f, chest.y + 46.0f);
        ImVec2 right_hand(head_center.x + 20.0f, chest.y + 46.0f);
        ImVec2 left_knee(head_center.x - 14.0f, pelvis.y + 26.0f);
        ImVec2 right_knee(head_center.x + 14.0f, pelvis.y + 26.0f);
        ImVec2 left_foot(head_center.x - 12.0f, box_max.y);
        ImVec2 right_foot(head_center.x + 12.0f, box_max.y);

        draw_list->AddLine(neck, chest, skeleton_color, 2.0f);
        draw_list->AddLine(chest, pelvis, skeleton_color, 2.0f);

        draw_list->AddLine(neck, left_shoulder, skeleton_color, 2.0f);
        draw_list->AddLine(left_shoulder, left_elbow, skeleton_color, 2.0f);
        draw_list->AddLine(left_elbow, left_hand, skeleton_color, 2.0f);

        draw_list->AddLine(neck, right_shoulder, skeleton_color, 2.0f);
        draw_list->AddLine(right_shoulder, right_elbow, skeleton_color, 2.0f);
        draw_list->AddLine(right_elbow, right_hand, skeleton_color, 2.0f);

        draw_list->AddLine(pelvis, left_knee, skeleton_color, 2.0f);
        draw_list->AddLine(left_knee, left_foot, skeleton_color, 2.0f);
        draw_list->AddLine(pelvis, right_knee, skeleton_color, 2.0f);
        draw_list->AddLine(right_knee, right_foot, skeleton_color, 2.0f);
    }

    if (settings::visuals::target_line)
    {
        draw_list->AddLine(head_center, ImVec2(head_center.x - 35.0f, head_center.y - 35.0f), esp_blue_color, 2.0f);
    }

    if (settings::visuals::line)
    {
        draw_list->AddLine(ImVec2((box_min.x + box_max.x) * 0.5f, box_max.y),
                           ImVec2((box_min.x + box_max.x) * 0.5f, bottom_right.y - 12.0f),
                           esp_blue_color,
                           2.0f);
    }

    if (font && settings::visuals::distance)
    {
        draw_list->AddText(font, 13.0f, ImVec2(box_min.x, box_max.y + 8.0f), IM_COL32(255, 255, 255, 210), "26 m");
    }

    if (font && settings::visuals::platform)
    {
        draw_list->AddText(font, 13.0f, ImVec2(box_min.x + 4.0f, box_min.y - 22.0f), IM_COL32(150, 150, 160, 255), "Win");
    }
}

void render_menu()
{
    // Initialize fonts to prevent crashes
    initialize_menu_fonts();
    
    switch (settings::aimbot::current_aimkey)
    {
    case 0: settings::aimbot::current_key = VK_LBUTTON; break;
    case 1: settings::aimbot::current_key = VK_RBUTTON; break;
    case 2: settings::aimbot::current_key = VK_XBUTTON1; break;
    case 3: settings::aimbot::current_key = VK_XBUTTON2; break;
    case 4: settings::aimbot::current_key = VK_SHIFT; break;
    case 5: settings::aimbot::current_key = VK_CONTROL; break;
    case 6: settings::aimbot::current_key = VK_MENU; break;
    case 7: settings::aimbot::current_key = VK_CAPITAL; break;
    default: settings::aimbot::current_key = VK_LBUTTON; break;
    }
    
    if (GetAsyncKeyState(VK_INSERT) & 1) settings::show_menu = !settings::show_menu;

    // Format the expiry time - KeyAuth removed
    std::string timeLeft = "Lifetime"; // No expiry needed

        // KeyAuth removed - always lifetime

    int sub_page = 0;
    static int page = 0;
    static float tab_alpha = 0.f; /* */ static float tab_add; /* */ static int active_tab = 0;

    int togle = 0;
    static float anim_text = 0.f;
    // Format the watermark text
    char watermarkText[256];
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    // Get current time
    SYSTEMTIME st;
    GetLocalTime(&st);

    // Watermark - controlled by settings
    if (settings::watermark_enabled)
    {
        // Format the watermark text - Modern Style
        sprintf_s(watermarkText, "https://discord.gg/cpm326Nuf2 | %d FPS | %02d:%02d:%02d",
            getFps(),
            st.wHour,
            st.wMinute,
            st.wSecond
        );

        // Calculate text size accurately for the target font size
        float targetFontSize = 15.0f;
        ImVec2 wmTextSize = ImGui::GetFont()->CalcTextSizeA(targetFontSize, FLT_MAX, -1.0f, watermarkText, NULL);

        ImVec2 padding = ImVec2(20.0f, 14.0f);
        ImVec2 rectSize = ImVec2(wmTextSize.x + 2 * padding.x, wmTextSize.y + 2 * padding.y);
        ImVec2 rectPos = ImVec2(12, 12);
        float rounding = 10.0f;

        // Draw outer glow/shadow effect (multiple layers for depth)
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(rectPos.x + 3, rectPos.y + 3),
            ImVec2(rectPos.x + rectSize.x + 3, rectPos.y + rectSize.y + 3),
            IM_COL32(0, 0, 0, 100),
            rounding
        );
        
        // Draw main background (dark with slight transparency)
        ImGui::GetForegroundDrawList()->AddRectFilled(
            rectPos,
            ImVec2(rectPos.x + rectSize.x, rectPos.y + rectSize.y),
            ImColor(15, 15, 18, 245),
            rounding
        );

        // Draw blue accent border (top and left edges) - matching menu blue
        ImColor accent_blue = ImColor(90, 140, 255, 255);
        
        // Top border
        ImGui::GetForegroundDrawList()->AddLine(
            ImVec2(rectPos.x + rounding, rectPos.y),
            ImVec2(rectPos.x + rectSize.x - rounding, rectPos.y),
            accent_blue,
            2.5f
        );
        
        // Left border
        ImGui::GetForegroundDrawList()->AddLine(
            ImVec2(rectPos.x, rectPos.y + rounding),
            ImVec2(rectPos.x, rectPos.y + rectSize.y - rounding),
            accent_blue,
            2.5f
        );

        // Draw subtle outer border
        ImGui::GetForegroundDrawList()->AddRect(
            rectPos,
            ImVec2(rectPos.x + rectSize.x, rectPos.y + rectSize.y),
            ImColor(30, 30, 35, 255),
            rounding,
            0,
            1.0f
        );

        // Calculate text position
        ImVec2 textDrawPos = ImVec2(rectPos.x + padding.x, rectPos.y + padding.y);

        // Draw text shadow for depth
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetFont(),
            targetFontSize,
            ImVec2(textDrawPos.x + 1.5f, textDrawPos.y + 1.5f),
            ImColor(0, 0, 0, 200),
            watermarkText
        );

        // Draw main text - white with slight blue tint for "ClutchSoftware"
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetFont(),
            targetFontSize,
            textDrawPos,
            ImColor(240, 240, 250, 255),
            watermarkText
        );
        
        // Highlight "ClutchSoftware" part with blue color
        const char* clutchSoftwareStart = strstr(watermarkText, "Optimal public");
        if (clutchSoftwareStart) {
            int clutchSoftwareOffset = (int)(clutchSoftwareStart - watermarkText);
            float beforeWidth = ImGui::GetFont()->CalcTextSizeA(targetFontSize, FLT_MAX, -1.0f, watermarkText, watermarkText + clutchSoftwareOffset).x;
            ImVec2 clutchSoftwarePos = ImVec2(textDrawPos.x + beforeWidth, textDrawPos.y);
            
            // Draw blue highlight for "ClutchSoftware"
            ImGui::GetForegroundDrawList()->AddText(
                ImGui::GetFont(),
                targetFontSize,
                clutchSoftwarePos,
                accent_blue,
                "Optimal public"
            );
        }
    }

    if (settings::show_menu)
    {
        ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
       CustomStyleColor();
   
        ImGui::Begin("Optimal public", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            ImGui::PushFont(ubu_preview);
            const auto& p = ImGui::GetWindowPos();
            ImGuiStyle* s = &ImGui::GetStyle();

            // Main window background - dark gray
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(0 + p.x, 0 + p.y), 
                ImVec2(WIDTH + p.x, HEIGHT + p.y), 
                ImColor(12, 12, 12, 255), 
                s->WindowRounding
            );

            // Left Navigation Bar Width (defined once)
            float left_navbar_width = 80.0f;
            
            // Top Navigation Bar Background - Full width to cover all gaps
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(0 + p.x, 0 + p.y), 
                ImVec2(WIDTH + p.x, 60 + p.y), 
                ImColor(24, 24, 24, 255), 
                s->WindowRounding, 
                ImDrawFlags_RoundCornersTop
            );
            
            // Logo Area (centered in top left area)
            float logo_size = 50.0f;
            float logo_x = p.x + (left_navbar_width - logo_size) / 2.0f;
            float logo_y = p.y + (60 - logo_size) / 2.0f;
            
            // Render logo image if available (same way as visual_img)
            // Logo wird nur in directx_init geladen, nicht hier (verhindert Lag)
            if (logo_img != nullptr) {
                // Draw logo texture (centered, same way as visual_img) - genau wie visual_img
                ImGui::GetWindowDrawList()->AddImage(
                    logo_img,
                    ImVec2(logo_x, logo_y),
                    ImVec2(logo_x + logo_size, logo_y + logo_size),
                    ImVec2(0, 0),
                    ImVec2(1, 1),
                    IM_COL32(255, 255, 255, 255)
                );
            }
            

            // ClutchSoftware text (centered in top navbar)
            ImVec2 clutchSize = ImGui::GetFont()->CalcTextSizeA(18.0f, FLT_MAX, -1.0f, "Optimal", NULL);
            ImVec2 softwareSize = ImGui::GetFont()->CalcTextSizeA(18.0f, FLT_MAX, -1.0f, "Public", NULL);
            float total_text_width = clutchSize.x + softwareSize.x;
            float text_start_x = p.x + (WIDTH - total_text_width) * 0.5f;
            float text_y = p.y + (60 - 18.0f) * 0.5f; // Vertically centered
            
            ImGui::GetWindowDrawList()->AddText(
                ImGui::GetFont(), 
                18.0f, 
                ImVec2(text_start_x, text_y), 
                ImColor(255, 255, 255, 255), 
                "Optimal"
            );
            ImGui::GetWindowDrawList()->AddText(
                ImGui::GetFont(), 
                18.0f, 
                ImVec2(text_start_x + clutchSize.x, text_y), 
                ImColor(255, 255, 255, 255),
                "Public"
            );

            // Left Navigation Bar Background - Full height to cover all gaps
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(0 + p.x, 0 + p.y), 
                ImVec2(left_navbar_width + p.x, HEIGHT + p.y), 
                ImColor(24, 24, 24, 255), 
                s->WindowRounding, 
                ImDrawFlags_RoundCornersLeft
            );
            
            // Left Navigation Tabs with Icons
            static int bottom_tabs = 0;
            static int previous_tab = 0;
            static float tab_slide_offset = 0.0f;
            const char* bottom_tab_names[] = { "Aimbot", "Visuals", "Config", "Settings" };
            float tab_height = (HEIGHT - 60) / 4.0f;
            float icon_size = 20.0f;
            
            // Slide animation for tab switching
            if (bottom_tabs != previous_tab) {
                previous_tab = bottom_tabs;
            }
            
            // Animate slide offset - smoother animation (vertical now)
            float target_offset = bottom_tabs * tab_height;
            float current_offset = tab_slide_offset;
            float diff = target_offset - current_offset;
            float animation_speed = 12.0f; // Increased for smoother animation
            tab_slide_offset += diff * ImGui::GetIO().DeltaTime * animation_speed;
            
            // Smooth interpolation for better animation
            if (fabsf(diff) < 0.5f) {
                tab_slide_offset = target_offset; // Snap to target when close
            }
            
            for (int i = 0; i < 4; i++) {
                float tab_y = p.y + 60 + (i * tab_height);
                bool is_selected = (bottom_tabs == i);
                
                // Slide animation effect - highlight bar slides vertically (only draw once for selected tab)
                if (i == 0) {
                    float highlight_y = p.y + 60 + tab_slide_offset;
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(p.x + 2, highlight_y),
                        ImVec2(p.x + 4, highlight_y + tab_height),
                        ImColor(90, 140, 255, 255),
                        0.0f
                    );
                }
                
                ImColor tab_text_color = is_selected ? ImColor(90, 140, 255, 255) : ImColor(220, 220, 230, 255);
                ImColor tab_icon_color = is_selected ? ImColor(90, 140, 255, 255) : ImColor(220, 220, 230, 255);

                // Tab clickable area
                ImVec2 tab_min = ImVec2(p.x, tab_y);
                ImVec2 tab_max = ImVec2(p.x + left_navbar_width, tab_y + tab_height);
                if (ImGui::IsMouseHoveringRect(tab_min, tab_max) && ImGui::IsMouseClicked(0)) {
                    bottom_tabs = i;
                    active_tab = i;
                }
                
                // Draw icon based on tab type (centered horizontally and vertically in tab)
                float icon_x = p.x + (left_navbar_width - icon_size) / 2.0f;
                float text_height = 12.0f;
                float spacing = 8.0f;
                float total_content_height = icon_size + spacing + text_height;
                float icon_y = tab_y + (tab_height - total_content_height) / 2.0f;
                
                if (i == 0) {
                    // Aimbot icon (target/crosshair)
                    ImGui::GetWindowDrawList()->AddCircle(ImVec2(icon_x + icon_size/2, icon_y + icon_size/2), icon_size/3, tab_icon_color, 0, 2.0f);
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(icon_x + icon_size/2, icon_y), ImVec2(icon_x + icon_size/2, icon_y + icon_size), tab_icon_color, 2.0f);
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(icon_x, icon_y + icon_size/2), ImVec2(icon_x + icon_size, icon_y + icon_size/2), tab_icon_color, 2.0f);
                } else if (i == 1) {
                    // Visuals icon (eye)
                    ImGui::GetWindowDrawList()->AddCircle(ImVec2(icon_x + icon_size/2, icon_y + icon_size/2), icon_size/2.5f, tab_icon_color, 0, 2.0f);
                    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(icon_x + icon_size/2, icon_y + icon_size/2), icon_size/5, tab_icon_color);
                } else if (i == 2) {
                    // Config icon (folder)
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(icon_x + 2, icon_y + 5), ImVec2(icon_x + icon_size - 2, icon_y + icon_size - 2), tab_icon_color);
                    ImGui::GetWindowDrawList()->AddTriangleFilled(ImVec2(icon_x + 2, icon_y + 5), ImVec2(icon_x + 8, icon_y + 5), ImVec2(icon_x + 5, icon_y), ImColor(24, 24, 24, 255));
                } else if (i == 3) {
                    // Settings icon (gear)
                    float center_x = icon_x + icon_size/2;
                    float center_y = icon_y + icon_size/2;
                    float radius = icon_size/3;
                    for (int j = 0; j < 8; j++) {
                        float angle = (j * 3.14159f * 2.0f) / 8.0f;
                        ImVec2 start = ImVec2(center_x + cosf(angle) * radius, center_y + sinf(angle) * radius);
                        ImVec2 end = ImVec2(center_x + cosf(angle) * (radius + 3), center_y + sinf(angle) * (radius + 3));
                        ImGui::GetWindowDrawList()->AddLine(start, end, tab_icon_color, 2.0f);
                    }
                }
                
                // Tab text below icon (centered, consistent spacing)
                ImVec2 text_size = ImGui::GetFont()->CalcTextSizeA(12.0f, FLT_MAX, -1.0f, bottom_tab_names[i], NULL);
                ImGui::GetWindowDrawList()->AddText(
                    ImGui::GetFont(), 
                    12.0f, 
                    ImVec2(p.x + (left_navbar_width - text_size.x) / 2.0f, icon_y + icon_size + spacing), 
                    tab_text_color, 
                    bottom_tab_names[i]
                );
            }
            

            tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (bottom_tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);
            if (tab_alpha == 0.f && tab_add == 0.f) active_tab = bottom_tabs;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * s->Alpha);
            ImGui::SetCursorPos(ImVec2(left_navbar_width + 20, 70));

            ImGui::BeginChild("Content", ImVec2(WIDTH - left_navbar_width - 40, HEIGHT - 80), true, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
                {
                    // Store bottom_tabs for use in tab content
                    tabs = bottom_tabs;

                    if (active_tab == 0) {
                        // Aimbot Tab - FrozenFree Style
                        ImGui::PushFont(ubu_0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));
                        
                        // Left Column - Features/Toggles
                        ImGui::BeginGroup();
                        {
                            ImGui::BeginChild("Aimbot Features", ImVec2(350, 470), true, ImGuiWindowFlags_NoScrollbar);
                            {
                                // Aimbot Title (centered)
                                float title_width = ImGui::CalcTextSize("Aimbot").x;
                                float avail_width = ImGui::GetContentRegionAvail().x;
                                ImGui::SetCursorPosX((avail_width - title_width) * 0.5f);
                                ImGui::TextColored(ImColor(220, 220, 230, 255), "Aimbot");
                                
                                // Enable Aimbot Toggle
                                CustomToggle("Enable Aimbot", &settings::aimbot::enable);
                                settings::aimbot::mouseAim = settings::aimbot::enable;
                                
                                // Ensure aimbot targets head by default (index 0 = head)
                                if (settings::aimbot::current_hitbox < 0 || settings::aimbot::current_hitbox >= 4) {
                                    settings::aimbot::current_hitbox = 0; // Head (index 0)
                                }

                                // Visible Check Toggle
                                CustomToggle("Visible Check", &settings::aimbot::visible_check);

                                // Crosshair Toggle
                                CustomToggle("Crosshair", &settings::aimbot::crosshair);

                                // Draw FOV Toggle
                                CustomToggle("Draw FOV", &settings::aimbot::show_fov);

                                // Fill FOV Toggle
                                CustomToggle("Fill FOV", &settings::aimbot::fill_fov);

                                // FOV Arrows Toggle
                                CustomToggle("FOV Arrows", &settings::aimbot::FOVArrows);

                                // Ignore Knocked Toggle
                                CustomToggle("Ignore Knocked", &settings::aimbot::ignore_knocked);
                                
                                ImGui::Spacing();
                                ImGui::Spacing();
                                ImGui::Spacing();
                                
                                // Aimbot Hitbox Dropdown
                                float hitbox_title_width = ImGui::CalcTextSize("Aimbot Hitbox").x;
                                float hitbox_avail_width = ImGui::GetContentRegionAvail().x;
                                ImGui::SetCursorPosX((hitbox_avail_width - hitbox_title_width) * 0.5f);
                                ImGui::Text("Aimbot Hitbox");
                                
                                ImGui::Spacing();
                                
                                const char* hitbox_types[] = { "Head", "Neck", "Chest", "Ass" };
                                // current_hitbox is stored as index (0-3), not bone ID
                                int hitbox_index = settings::aimbot::current_hitbox;
                                if (hitbox_index < 0 || hitbox_index >= 4) {
                                    hitbox_index = 0; // Default to Head
                                    settings::aimbot::current_hitbox = 0;
                                }
                                CustomCombo("", &hitbox_index, hitbox_types, IM_ARRAYSIZE(hitbox_types));
                                if (hitbox_index >= 0 && hitbox_index < 4) {
                                    settings::aimbot::current_hitbox = hitbox_index; // Store as index, not bone ID
                                }
                            }
                            ImGui::EndChild();
                        }
                        ImGui::EndGroup();

                        ImGui::SameLine(0, 20);

                        // Right Column - Settings/Sliders
                        ImGui::BeginGroup();
                        {
                            ImGui::BeginChild("Aimbot Settings", ImVec2(350, 470), true, ImGuiWindowFlags_NoScrollbar);
                            {
                                // Aimbot Settings Title (centered)
                                float title_width = ImGui::CalcTextSize("Aimbot Settings").x;
                                float avail_width = ImGui::GetContentRegionAvail().x;
                                ImGui::SetCursorPosX((avail_width - title_width) * 0.5f);
                                ImGui::TextColored(ImColor(220, 220, 230, 255), "Aimbot Settings");
                                
                                // Add spacing to move sliders down
                                ImGui::Spacing();
                                ImGui::Spacing();
                                ImGui::Spacing();
                                
                                // Aimbot Smoothness Slider
                                CustomSliderFloat("Aimbot Smoothness", &settings::aimbot::smoothness, 1.0f, 50.0f, "%.1f");
                                
                                // FOV Circle Radius Slider
                                CustomSliderFloat("FOV Circle Radius", &settings::aimbot::fov, 30.0f, 500.0f, "%.0f");
                                
                                // Aimbot Range/Distance Slider
                                CustomSliderFloat("Aimbot Range", &settings::aimbot::aimbot_renderDistance, 10.0f, 1000.0f, "%.0f m");
                            }
                            ImGui::EndChild();
                        }
                        ImGui::EndGroup();
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopFont();
                    }
                    else if (active_tab == 1) {
                        ImGui::PushFont(ubu_0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 12));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
                        
                        ImGui::BeginChild("VisualsLegitbot", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
                        {
                            float total_width = ImGui::GetContentRegionAvail().x;
                            ImGui::Columns(2, "LegitColumns", false);
                            ImGui::SetColumnWidth(0, total_width * 0.6f);
                            
                            ImGui::BeginChild("LegitControls", ImVec2(0, 0), false);
                            {
                                // ESP Title (centered)
                                float title_width = ImGui::CalcTextSize("ESP").x;
                                float avail_width = ImGui::GetContentRegionAvail().x;
                                ImGui::SetCursorPosX((avail_width - title_width) * 0.5f);
                                ImGui::TextColored(ImColor(220, 220, 230, 255), "ESP");
                                ImGui::Spacing();

                                CustomToggle("Enable ESP", &settings::visuals::enable);
                                CustomToggle("Box", &settings::visuals::box);
                                
                                CustomToggle("Skeleton", &settings::visuals::skeleton);
                                CustomToggle("Snap Line", &settings::visuals::line);
                                CustomToggle("Name", &settings::visuals::name);
                                CustomToggle("Distance", &settings::visuals::distance);
                                CustomToggle("Platform", &settings::visuals::platform);
                                CustomToggle("Rank", &settings::visuals::rank);
                                CustomToggle("Spectator List", &settings::visuals::spectator_list);
                            }
                            ImGui::EndChild();
                            
                            ImGui::NextColumn();
                            
                            // Add spacing to move ESP Preview down
                                ImGui::Spacing();
                            ImGui::Spacing();
                                ImGui::Spacing();
                                
                            ImGui::BeginChild("LegitPreview", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
                            {
                                ImVec2 preview_avail = ImGui::GetContentRegionAvail();
                                ImVec2 preview_size(preview_avail.x, ImClamp(preview_avail.y, 240.0f, 380.0f));
                                if (preview_size.y < 260.0f)
                                    preview_size.y = 260.0f;
                                ImVec2 preview_pos = ImGui::GetCursorScreenPos();
                                DrawEspPreviewCard(preview_pos, preview_size);
                                ImGui::Dummy(preview_size);
                            }
                            ImGui::EndChild();
                            
                            ImGui::Columns(1);
                        }
                        ImGui::EndChild();
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopFont();
                    }
                    else if (active_tab == 2) {
                        // Config Tab - Like in image
                        ImGui::PushFont(ubu_0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 10));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));
                        
                        static int selectedConfig = -1;

                        // Get config directory path
                        char exe_path[MAX_PATH];
                        std::string configDir;
                        if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
                            std::string exe_dir = std::filesystem::path(exe_path).parent_path().string();
                            configDir = exe_dir + "\\configs";
                            std::filesystem::create_directories(configDir);
                        }

                        static std::vector<std::string> configFiles;
                        static std::vector<std::string> configFilesDisplay;
                        std::vector<const char*> configFilesCStr;

                        configFiles.clear();
                        configFilesDisplay.clear();
                        for (const auto& entry : std::filesystem::directory_iterator(configDir)) {
                            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                std::string filename = entry.path().filename().string();
                                std::string filenameWithoutExt = filename;
                                if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json") {
                                    filenameWithoutExt = filename.substr(0, filename.size() - 5);
                                }
                                configFiles.push_back(filenameWithoutExt);
                                configFilesDisplay.push_back(filenameWithoutExt + ".cfg");
                            }
                        }
                        for (const auto& file : configFilesDisplay) {
                            configFilesCStr.push_back(file.c_str());
                        }

                        ImGui::BeginChild("Config Manager", ImVec2(0, 0), true);
                        {
                            // Center "Config" title
                            float avail_width = ImGui::GetContentRegionAvail().x;
                            float title_width = ImGui::CalcTextSize("Config").x;
                            ImGui::SetCursorPosX((avail_width - title_width) * 0.5f);
                            ImGui::TextColored(ImColor(90, 140, 255, 255), "Config");
                            
                            ImGui::Spacing();
                            ImGui::Spacing();
                            
                            // Center "Config List" label
                            float list_label_width = ImGui::CalcTextSize("Config List").x;
                            ImGui::SetCursorPosX((avail_width - list_label_width) * 0.5f);
                            ImGui::Text("Config List");
                            
                            ImGui::Spacing();
                            
                            // Config List Box - centered
                            float list_width = avail_width - 40.0f;
                            float list_start_x = (avail_width - list_width) * 0.5f;
                            ImGui::SetCursorPosX(list_start_x);
                            ImGui::SetNextItemWidth(list_width);
                            ImGui::ListBox("##ConfigFiles", &selectedConfig, configFilesCStr.data(), configFilesCStr.size(), 10);

                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();
                            ImGui::Spacing();

                            // Buttons - Centered
                            float button_width = 150.0f;
                            float button_height = 40.0f;
                            float button_spacing = 20.0f;

                            // Center buttons
                            float buttons_total_width = (button_width * 2) + button_spacing;
                            float buttons_start = (avail_width - buttons_total_width) * 0.5f;

                            // Load Button
                            ImGui::SetCursorPosX(buttons_start);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(90.f/255.f, 140.f/255.f, 255.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(105.f/255.f, 155.f/255.f, 255.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(75.f/255.f, 125.f/255.f, 240.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            if (ImGui::Button("Load", ImVec2(button_width, button_height)) && selectedConfig >= 0 && selectedConfig < configFiles.size()) {
                            std::string selectedConfigFile = configFiles[selectedConfig];
                            settings::load_config(selectedConfigFile);
                        }
                            ImGui::PopStyleColor(4);

                            ImGui::SameLine(0, button_spacing);

                            // Save Button
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(90.f/255.f, 140.f/255.f, 255.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(105.f/255.f, 155.f/255.f, 255.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(75.f/255.f, 125.f/255.f, 240.f/255.f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            if (ImGui::Button("Save", ImVec2(button_width, button_height)) && selectedConfig >= 0 && selectedConfig < configFiles.size()) {
                            std::string selectedConfigFile = configFiles[selectedConfig];
                            settings::save_config(selectedConfigFile);
                        }
                            ImGui::PopStyleColor(4);


                        }
                        ImGui::EndChild();
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopFont();
                    }
                    else if (active_tab == 3) {
                        // Settings Tab - Two Columns
                        ImGui::PushFont(ubu_0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 10));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));
                        
                        // Left Column - Settings Toggles
                        ImGui::BeginGroup();
                        {
                            ImGui::BeginChild("Settings Features", ImVec2(350, 540), true, ImGuiWindowFlags_NoScrollbar);
                            {
                                // Enable Spinbot Toggle
                                CustomToggle("Enable Spinbot", &settings::exploits::Spinbot);
                                
                                ImGui::Spacing();
                                
                                // Spinbot Speed Slider (only visible when Spinbot is enabled)
                                if (settings::exploits::Spinbot) {
                                    CustomSliderFloat("Spinbot Speed", &settings::exploits::SpinbotSpeed, 1.0f, 50.0f, "%.1f");
                                }
                                
                                ImGui::Spacing();
                                ImGui::Spacing();
                                
                                // Stream-proof Toggle
                                CustomToggle("Stream-proof", &settings::visuals::streamproof);
                                
                                ImGui::Spacing();
                                
                                // Spectator List Toggle
                                CustomToggle("Spectator List", &settings::visuals::spectator_list);
                                
                                ImGui::Spacing();
                                
                                // Radar Toggle
                                CustomToggle("Radar", &settings::radar::radar);
                                
                                ImGui::Spacing();
                                ImGui::Spacing();
                                
                                // EXIT Button
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(82.f/255.f, 136.f/255.f, 255.f/255.f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(96.f/255.f, 150.f/255.f, 255.f/255.f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(68.f/255.f, 118.f/255.f, 235.f/255.f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                                if (ImGui::Button("EXIT", ImVec2(200, 40))) {
                                    exit(0);
                                }
                                ImGui::PopStyleColor(4);
                            }
                            ImGui::EndChild();
                        }
                        ImGui::EndGroup();
                        
                        ImGui::PopStyleVar(2);
                        ImGui::PopFont();
                    }
                }
                ImGui::EndChild();

                ImGui::PopStyleVar();

            // Keep spectator list and ESP preview code
                if (settings::visuals::spectator_list) {
                    std::vector<std::string> spectatorNames;

                    for (int i = 0; i < cache::player_count; i++) {
                        uintptr_t player_state = read<uintptr_t>(cache::player_array + (i * sizeof(uintptr_t)));
                        if (!player_state || player_state == cache::player_state) continue;

                        uintptr_t view_target = read<uintptr_t>(player_state + VIEW_TARGET);
                        if (view_target == cache::local_pawn) {
                            spectatorNames.push_back(GetPlayerName(player_state));
                            continue;
                        }

                        uintptr_t pawn_private = read<uintptr_t>(player_state + PAWN_PRIVATE);
                        if (pawn_private && is_dead(pawn_private)) {
                            uintptr_t spectator_target = read<uintptr_t>(player_state + 0x3A8);
                            if (spectator_target == cache::local_pawn) {
                                spectatorNames.push_back(GetPlayerName(player_state));
                                continue;
                            }
                            spectator_target = read<uintptr_t>(player_state + 0x3B0);
                            if (spectator_target == cache::local_pawn) {
                                spectatorNames.push_back(GetPlayerName(player_state));
                                continue;
                            }
                        }
                    }

                    ImGui::SetNextWindowPos(ImVec2(1200, 50), ImGuiCond_Once);
                    ImGui::SetNextWindowSize(ImVec2(160, 140));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(18.f / 255.f, 18.f / 255.f, 18.f / 255.f, 1.f));
                    ImGui::Begin("Spectators", &settings::visuals::spectator_list,
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                    ImGui::PopStyleColor();

                    ImFont* ubu_child = ImGui::GetFont();
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImVec2 p = ImGui::GetWindowPos();
                    float font_size = 15.f;
                    ImU32 color = ImColor(255, 255, 255);

        draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 160, p.y + 35), ImColor(18, 18, 18, 255), 8, ImDrawCornerFlags_Top);

                    char headerText[64];
                    sprintf_s(headerText, "Spectators: %d", static_cast<int>(spectatorNames.size()));
                    draw_list->AddText(ubu_child, font_size, ImVec2(p.x + 10, p.y + 9), color, headerText);

                    ImGui::Dummy(ImVec2(0, 140));

                    float y_offset = 45.f;
                    float line_height = font_size + 2.f;

                    if (spectatorNames.empty()) {
                        draw_list->AddText(ubu_child, font_size, ImVec2(p.x + 10, p.y + y_offset), color, "No Spectators");
                    }
                    else {
                        for (const auto& name : spectatorNames) {
                            draw_list->AddText(ubu_child, font_size, ImVec2(p.x + 10, p.y + y_offset), color, name.c_str());
                            y_offset += line_height;
                        }
                    }

                    ImGui::End();
                }

            ImGui::PopFont(); // Pop ubu_preview
            }
            ImGui::End();
        }
   
            keybind_alpha = ImClamp(keybind_alpha + (4.f * ImGui::GetIO().DeltaTime * (settings::visuals::spectator_list ? 1.f : -1.f)), 0.f, 1.f);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, keybind_alpha * ImGui::GetStyle().Alpha);
            ImGui::PopStyleVar();

            preview_alpha = ImClamp(preview_alpha + (4.f * ImGui::GetIO().DeltaTime * (esp_preview ? 1.f : -1.f)), 0.f, 1.f);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, preview_alpha * ImGui::GetStyle().Alpha);

            if (esp_preview)
            {
        ImGuiWindow* generalWindow = ImGui::FindWindowByName("Optimal public");
                if (generalWindow)
                {
                    ImVec2 menuPos = generalWindow->Pos;
                    ImVec2 menuSize = generalWindow->Size;

                    float spacing = 20.0f; 

                    ImVec2 previewPos(
                        menuPos.x + menuSize.x + spacing,
                        menuPos.y + (menuSize.y / 2.0f) - (420 / 2.0f) 
                    );

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(18.f / 255.f, 18.f / 255.f, 18.f / 255.f, 255.f / 255.f));
                    ImGui::SetNextWindowPos(previewPos, ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(280, 420));

                    ImGui::Begin("Esp Preview", nullptr,
                        ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_NoMove);

                    ImGui::PopStyleColor();

                    const auto& p = ImGui::GetWindowPos();
                    const auto& draw_list = ImGui::GetWindowDrawList();
            ImFont* ubu_child_font = ImGui::GetFont();

            draw_list->AddText(ubu_child_font, 15.f, ImVec2(p.x + 10, p.y + 10), ImColor(200, 200, 210), "ESP Preview");
                draw_list->AddCircleFilled(ImVec2(p.x + 257, p.y + 18), 12.0f, ImColor(140, 112, 255, 255), 32);
                if (settings::visuals::platform) {
                draw_list->AddText(ubu_child_font, 15.f, ImVec2(p.x + 83, p.y + 28), ImColor(255, 255, 255), "Win");
                }
               if (settings::visuals::name) {
                draw_list->AddText(ubu_child_font, 15.f, ImVec2(p.x + 83, p.y + 40), ImColor(255, 255, 255), "Player");
            }

               if (settings::visuals::box) {
               draw_list->AddRect(ImVec2(p.x + 40, p.y + 65), ImVec2(p.x + 210, p.y + 375), ImColor(255, 255, 255), 2, 2);
               }
                if (settings::visuals::skeleton) {
                struct BoneConnection { int from, to; };

                float rect_x1 = p.x + 40;
                float rect_y1 = p.y + 65;
                float rect_x2 = p.x + 210;
                float rect_y2 = p.y + 375; 

                float width = rect_x2 - rect_x1;
                float height = rect_y2 - rect_y1;
                float center_x = rect_x1 + width / 2.f;

                ImU32 color = ImColor(0, 255, 0);
                float thickness = 1.5f;

                float scale = height / 310.f;

                std::map<int, ImVec2> bonePositions;

                bonePositions[110] = ImVec2(center_x, rect_y1 + 5 * scale);  
                bonePositions[67] = ImVec2(center_x, rect_y1 + 25 * scale);

                bonePositions[7] = ImVec2(center_x, rect_y1 + 70 * scale); 
                bonePositions[2] = ImVec2(center_x, rect_y1 + 130 * scale);

                bonePositions[9] = ImVec2(center_x + 25 * scale, rect_y1 + 70 * scale); 
                bonePositions[35] = ImVec2(center_x + 35 * scale, rect_y1 + 110 * scale);
                bonePositions[11] = ImVec2(center_x + 35 * scale, rect_y1 + 140 * scale);

                bonePositions[38] = ImVec2(center_x - 25 * scale, rect_y1 + 70 * scale);
                bonePositions[39] = ImVec2(center_x - 35 * scale, rect_y1 + 110 * scale);
                bonePositions[40] = ImVec2(center_x - 35 * scale, rect_y1 + 140 * scale);

                float foot_y = p.y + 380 - 5;
                bonePositions[71] = ImVec2(center_x + 15 * scale, rect_y1 + 150 * scale); 
                bonePositions[72] = ImVec2(center_x + 15 * scale, rect_y1 + 200 * scale); 
                bonePositions[73] = ImVec2(center_x + 15 * scale, rect_y1 + 250 * scale); 
                bonePositions[75] = ImVec2(center_x + 15 * scale, foot_y);                

                bonePositions[78] = ImVec2(center_x - 15 * scale, rect_y1 + 150 * scale); 
                bonePositions[79] = ImVec2(center_x - 15 * scale, rect_y1 + 200 * scale); 
                bonePositions[80] = ImVec2(center_x - 15 * scale, rect_y1 + 250 * scale); 
                bonePositions[82] = ImVec2(center_x - 15 * scale, foot_y);                  

                BoneConnection boneConnections[] = {
                    {67, 7},  {7, 2},   {7, 9},  {9, 35},  {35, 11},
                    {7, 38},  {38, 39}, {39, 40}, {2, 71}, {71, 72},
                    {72, 73}, {73, 75}, {2, 78}, {78, 79}, {79, 80}, {80, 82}
                };

                for (auto& bone : boneConnections) {
                    if (bonePositions.count(bone.from) && bonePositions.count(bone.to)) {
                        draw_list->AddLine(bonePositions[bone.from], bonePositions[bone.to], color, thickness);
                    }
                }
              }
          
                if (settings::visuals::distance) {
                draw_list->AddText(ubu_child_font, 14.f, ImVec2(p.x + 113, p.y + 390), ImColor(255, 255, 255), "200m");
                }
                if (settings::visuals::rank) {
                draw_list->AddText(ubu_child_font, 14.f, ImVec2(p.x + 113, p.y + 410), ImColor(68, 35, 94), "Unreal");
                }
                ImGui::End();
                }
            }
            ImGui::PopStyleVar();
  }
 

HWND get_process_wnd(uint32_t pid)
{
    std::pair<HWND, uint32_t> params = { 0, pid };
    BOOL bresult = EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL
        {
            auto pparams = (std::pair<HWND, uint32_t>*)(lparam);
            uint32_t processid = 0;
            if (GetWindowThreadProcessId(hwnd, reinterpret_cast<LPDWORD>(&processid)) && processid == pparams->second)
            {
                SetLastError((uint32_t)-1);
                pparams->first = hwnd;
                return FALSE;
            }
            return TRUE;
        }, (LPARAM)&params);

    if (!bresult && GetLastError() == -1 && params.first) return params.first;

    return 0;
}

WPARAM render_loop()
{
    static RECT old_rc;
    static bool streamproof_state = false;

    bool running = true;
    while (running)
    {
        if (intel_driver::process_id == 0 || game_wnd == NULL || !IsWindow(game_wnd))
        {
            uint32_t new_pid = intel_driver::find_process(L"FortniteClient-Win64-Shipping.exe");
            if (new_pid != 0)
            {
                intel_driver::process_id = new_pid;
                game_wnd = get_process_wnd(intel_driver::process_id);
                if (game_wnd)
                {
                    virtualaddy = intel_driver::find_image();
                    cr3 = intel_driver::fetch_cr3();
                }
            }
        }

        if (streamproof_state != settings::visuals::streamproof) {
            SetWindowDisplayAffinity(my_wnd, settings::visuals::streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
            streamproof_state = settings::visuals::streamproof;
        }

        settings::aimbot::controller_aim_pressed = false; 
        if (settings::aimbot::controller_support) {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
                XINPUT_STATE controllerState;
                if (XInputGetState(i, &controllerState) == ERROR_SUCCESS) {
                    if (controllerState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
                        settings::aimbot::controller_aim_pressed = true;
                        break; 
                    }
                }
            }
        }

        try {
            while (PeekMessage(&messager, my_wnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&messager);
                DispatchMessage(&messager);
                if (messager.message == WM_QUIT)
                {
                    running = false;
                }
            }

            if (!running) break;

            if (game_wnd == NULL) {
                Sleep(1000);
                continue;
            }

            HWND active_wnd = GetForegroundWindow();
            if (active_wnd == game_wnd)
            {
                HWND target_wnd = GetWindow(active_wnd, GW_HWNDPREV);
                SetWindowPos(my_wnd, target_wnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            else
            {
                game_wnd = get_process_wnd(intel_driver::process_id);
            }

            RECT rc = {0};
            POINT xy = {0};
            if (game_wnd && IsWindow(game_wnd))
            {
                GetWindowRect(game_wnd, &rc);
                xy.x = rc.left;
                xy.y = rc.top;
            }
            ImGuiIO& io = ImGui::GetIO();
            io.DeltaTime = 1.0f / 60.0f;
            POINT p;
            GetCursorPos(&p);
            io.MousePos.x = p.x - xy.x;
            io.MousePos.y = p.y - xy.y;

            if (GetAsyncKeyState(0x1))
            {
                io.MouseDown[0] = true;
                io.MouseClicked[0] = true;
                io.MouseClickedPos[0].x = io.MousePos.x;
            io.MouseClickedPos[0].y = io.MousePos.y;
            }
            else
            {
                io.MouseDown[0] = false;
            }

            if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom)
            {
                old_rc = rc;
                settings::width = rc.right - rc.left;
                settings::height = rc.bottom - rc.top;
                p_params.BackBufferWidth = settings::width;
                p_params.BackBufferHeight = settings::height;
                SetWindowPos(my_wnd, (HWND)0, xy.x, xy.y, settings::width, settings::height, SWP_NOZORDER | SWP_SHOWWINDOW);
                ImGui_ImplDX9_InvalidateDeviceObjects();
                // Logo-Textur freigeben, damit sie neu geladen wird
                if (logo_img != nullptr) {
                    logo_img->Release();
                    logo_img = nullptr;
                }
                p_device->Reset(&p_params);
                ImGui_ImplDX9_CreateDeviceObjects();
                // Logo neu laden
                if (logo_img == nullptr) {
                    CreateTextureFromPNG_GDI(p_device, logo_bytes, logo_bytes_size, &logo_img);
                }
            }

            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            {
                // Restore transparent/click-through for overlay
                LONG_PTR exStyle = GetWindowLongPtr(my_wnd, GWL_EXSTYLE);
                if (!(exStyle & WS_EX_TRANSPARENT)) {
                    SetWindowLongPtr(my_wnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                }
                if (GetForegroundWindow() == game_wnd)
                {
                    features();
                    render_menu();
                }
            }
            ImGui::EndFrame();
            ImGui::Render();

            p_device->SetRenderState(D3DRS_ZENABLE, FALSE);
            p_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            p_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
            p_device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

            if (p_device->BeginScene() >= 0)
            {
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                p_device->EndScene();
            }

            HRESULT result = p_device->Present(0, 0, 0, 0);
            if (result == D3DERR_DEVICELOST)
            {
                HRESULT coop = p_device->TestCooperativeLevel();
                if (coop == D3DERR_DEVICENOTRESET)
                {
                    ImGui_ImplDX9_InvalidateDeviceObjects();
                    // Logo-Textur freigeben, damit sie neu geladen wird
                    if (logo_img != nullptr) {
                        logo_img->Release();
                        logo_img = nullptr;
                    }
                    if (SUCCEEDED(p_device->Reset(&p_params)))
                    {
                        ImGui_ImplDX9_CreateDeviceObjects();
                        // Logo neu laden
                        if (logo_img == nullptr) {
                            CreateTextureFromPNG_GDI(p_device, logo_bytes, logo_bytes_size, &logo_img);
                        }
                    }
                }
                else
                {
                    Sleep(50);
                }
            }
        }
        catch (...) {
            std::ofstream log("crashlog.txt", std::ios::app);
            log << "[render_loop unknown exception] Overlay/ESP crashed!" << std::endl;
            log.close();
            MessageBoxA(NULL, "Unknown error in render_loop! See crashlog.txt", "Overlay/ESP crashed", MB_ICONERROR);
            Sleep(100);
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (p_device != 0)
    {
        p_device->EndScene();
        p_device->Release();
    }

    if (p_object != 0) p_object->Release();

    DestroyWindow(my_wnd);

    return messager.wParam;
}