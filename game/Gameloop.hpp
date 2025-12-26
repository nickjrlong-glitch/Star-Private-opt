#include "../driver/comm.hpp"
#include "../sdk-offsets/sdk.hpp"
#include "../sdk-offsets/offsets.hpp"
#include "../imgui/imgui.h"
#include "../settings/settings.hpp"
#include "visuals/Esp-Draw.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>
#define VELOCITY 0x180 
#include <cfloat> 
#include <algorithm> 
#include "aimbot/Aimbot.hpp"
// driver namespace
namespace driver {
    template <typename T>
    T read(uint64_t address) {
        return ::read<T>(address);
    }

    template <typename T>
    T write(uint64_t address, T value) {
        ::write<T>(address, value);
        return value;
    }

    inline bool IsValidPointer(uintptr_t ptr) {
        return ptr != 0 && ptr > 0x10000 && ptr < 0x7FFFFFFFFFFF;
    }
}

// Cached player struct
struct CachedPlayer {
    uintptr_t pawn_private;
};

// Static variables for SpeedHack
static std::mutex cache_mutex;
static std::vector<CachedPlayer> cached_players;

// Forward vector calculation
inline Vector3 get_forward_vector(Vector3 rotation) {
    float radyaw = (rotation.y * M_PI / 180.0f);
    float radpitch = (rotation.x * M_PI / 180.0f);
    Vector3 forward;
    forward.x = cosf(radpitch) * cosf(radyaw);
    forward.y = cosf(radpitch) * sinf(radyaw);
    forward.z = sinf(radpitch);
    return forward;
}

// Get closest enemy
inline uintptr_t get_closest_enemy() {
    return cache::closest_pawn;
}

__forceinline auto SkeltonESP(uintptr_t mesh) -> void;
__forceinline auto SkeltonESP_Smooth(uintptr_t mesh) -> void;

void AdjustPlayerSize();


inline static int keystatus = 0;
inline static int triggerbot_keystatus = 0;
inline static int freezeplayer_enemy_keystatus = 0;
inline static int menu_keystatus = 0;

inline static int realkey = 0;

inline bool GetKey(int key)
{
    realkey = key;
    return true;
}

inline void ChangeKey(void* blank)
{
    keystatus = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                aimbot.aimkey = i;
                keystatus = 0;
                return;
            }
        }
    }
}

inline void ChangeKeyTriggerbot(void* blank)
{
    triggerbot_keystatus = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                settings::triggerbot::key = i;
                triggerbot_keystatus = 0;
                return;
            }
        }
    }
}

inline void ChangeKeyFreezeEnemyPlayers(void* blank)
{
    freezeplayer_enemy_keystatus = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                aimbot.freezeplayerkey = i;
                freezeplayer_enemy_keystatus = 0;
                return;
            }
        }
    }
}

Vector3 PredictLocation(Vector3 target, Vector3 targetVelocity, float projectileSpeed, float projectileGravityScale, float distance)
{
    float horizontalTime = distance / projectileSpeed;
    float verticalTime = distance / projectileSpeed;

    target.x += targetVelocity.x * horizontalTime;
    target.y += targetVelocity.y * horizontalTime;
    target.z += targetVelocity.z * verticalTime +
        abs(-980 * projectileGravityScale) * 0.5f * (verticalTime * verticalTime);

    return target;
}

inline bool Prediction = false;

static const char* keyNames[] =
{
    "Keybind",
    "Left Mouse",
    "Right Mouse",
    "Cancel",
    "Middle Mouse",
    "Mouse 5",
    "Mouse 4",
    "",
    "Backspace",
    "Tab",
    "",
    "",
    "Clear",
    "Enter",
    "",
    "",
    "Shift",
    "Control",
    "Alt",
    "Pause",
    "Caps",
    "",
    "",
    "",
    "",
    "",
    "",
    "Escape",
    "",
    "",
    "",
    "",
    "Space",
    "Page Up",
    "Page Down",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "",
    "",
    "",
    "Print",
    "Insert",
    "Delete",
    "",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "",
    "",
    "",
    "",
    "",
    "Numpad 0",
    "Numpad 1",
    "Numpad 2",
    "Numpad 3",
    "Numpad 4",
    "Numpad 5",
    "Numpad 6",
    "Numpad 7",
    "Numpad 8",
    "Numpad 9",
    "Multiply",
    "Add",
    "",
    "Subtract",
    "Decimal",
    "Divide",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
};

inline static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char* const* items = (const char* const*)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

static void HotkeyButton(int aimkey, void* changekey, int status)
{
    const char* preview_value = "Press Key";
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value[0] == '\0')
        aimkeys = "Your Key";
    else
        aimkeys = preview_value;

    if (status == 1)
    {
        aimkeys = "Press Key";
    }
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)changekey, nullptr, 0, nullptr);
        }
    }
}

#ifdef _WIN32
#include <XInput.h>
#pragma comment(lib, "XInput.lib")
#endif
#include "../menu.hpp"

void handle_triggerbot()
{
    if (!settings::triggerbot::enable)
        return;

    if (GetAsyncKeyState(settings::triggerbot::key))
    {
        uintptr_t target_pawn = cache::closest_pawn;

        if (target_pawn && target_pawn != cache::local_pawn)
        {
            if (settings::aimbot::ignore_knocked && is_dead(target_pawn))
            {
                return; 
            }

            if (settings::visuals::ignore_teamates) {
                uintptr_t player_state = read<uintptr_t>(target_pawn + PLAYER_STATE);
                int player_team_id = read<int>(player_state + TEAM_INDEX);
                if (cache::my_team_id == player_team_id) {
                    return; 
                }
            }

            std::string weaponName = GetWeaponName(cache::local_pawn);
            bool shouldTrigger = false;

            if (weaponName.find("Assault") != std::string::npos && settings::triggerbot::assault_rifles)
                shouldTrigger = true;
            else if (weaponName.find("Shotgun") != std::string::npos && settings::triggerbot::shotguns)
                shouldTrigger = true;
            else if ((weaponName.find("SMG") != std::string::npos || weaponName.find("Submachine") != std::string::npos) && settings::triggerbot::smgs)
                shouldTrigger = true;
            else if ((weaponName.find("Sniper") != std::string::npos || weaponName.find("Rifle") != std::string::npos) && settings::triggerbot::snipers)
                shouldTrigger = true;
            else if (weaponName.find("Pistol") != std::string::npos && settings::triggerbot::pistols)
                shouldTrigger = true;

            if (!shouldTrigger)
                return;

            uintptr_t actorRootComponent = read<uintptr_t>(target_pawn + ROOT_COMPONENT);
            Vector3 actorRelativeLocation = read<Vector3>(actorRootComponent + RELATIVE_LOCATION);
            float distance = cache::relative_location.distance(actorRelativeLocation) / 100.0f;

            if (distance < settings::triggerbot::distance)
            {
                Sleep(settings::triggerbot::delay);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }
        }
    }
}

void draw_head_cone(uintptr_t mesh) {
    if (!mesh) return;

    Vector3 head3d = get_entity_bone(mesh, 110);
    Vector2 head2d = project_world_to_screen(head3d);
    float width = 160.0f;
    float half = width * 0.5f;
    ImVec2 headCenter(head2d.x, head2d.y);
    float radius = half;
    float apex_offset = half * 1.1f;
    ImVec2 apex(headCenter.x, headCenter.y - apex_offset);

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImColor col_main = settings::colors::icHeadConeColor;


    const int segments = 32;
    std::vector<ImVec2> arc_points;
    for (int i = 0; i <= segments; ++i) {
        float angle = M_PI * (float(i) / float(segments));
        float x = headCenter.x - radius * cosf(angle);
        float y = headCenter.y + radius * sinf(angle);
        arc_points.push_back(ImVec2(x, y));
    }

    for (int i = 0; i < segments; ++i) {
        dl->AddLine(arc_points[i], arc_points[i + 1], col_main, 2.0f);
    }

    for (const auto& pt : arc_points) {
        dl->AddLine(apex, pt, col_main, 1.0f);
    }

    const int layers = 3;
    for (int l = 1; l <= layers; ++l) {
        float t = float(l) / (layers + 1);
        float y = apex.y + (headCenter.y - apex.y) * t + radius * 0.2f * t;
        float band_half = radius * sinf(M_PI * t);
        dl->AddLine(ImVec2(headCenter.x - band_half, y), ImVec2(headCenter.x + band_half, y), col_main, 1.0f);
    }
}

void actorLoop()
{
    try {

	static int invalid_world_frames = 0;
	if (!virtualaddy || !cr3) {
		if ((++invalid_world_frames % 120) == 0) {
			virtualaddy = intel_driver::find_image();
			cr3 = intel_driver::fetch_cr3();
		}
		return;
	}
 
    static int cache_clear_counter = 0;
    if (++cache_clear_counter >= 1000) {
        cache_clear_counter = 0;
    }
    












    if (settings::aimbot::aimbot_type == 1) {
    }

    uintptr_t encryptedUWorld = read<uintptr_t>(virtualaddy + UWORLD);
    uintptr_t decryptedUWorld = decryptUWorld(encryptedUWorld);
    cache::uworld = decryptedUWorld;
    cache::game_instance = read<uintptr_t>(cache::uworld + GAME_INSTANCE);
    cache::local_players = read<uintptr_t>(read<uintptr_t>(cache::game_instance + LOCAL_PLAYERS));
    cache::player_controller = read<uintptr_t>(cache::local_players + PLAYER_CONTROLLER);
    cache::local_pawn = read<uintptr_t>(cache::player_controller + LOCAL_PAWN);

	if (!cache::uworld || !cache::game_instance || !cache::local_players || !cache::player_controller) {
		if ((++invalid_world_frames % 120) == 0) {
			virtualaddy = intel_driver::find_image();
			cr3 = intel_driver::fetch_cr3();
		}
		return;
	}
	invalid_world_frames = 0;

    static bool spinbot_initialized = false;
    static Vector3 original_rotation;
    static float current_spin_angle = 0.f;

    if (cache::local_pawn)
    {
        cache::root_component = read<uintptr_t>(cache::local_pawn + ROOT_COMPONENT);
        cache::player_state = read<uintptr_t>(cache::local_pawn + PLAYER_STATE);
        cache::current_weapon = read<uintptr_t>(cache::local_pawn + CURRENT_WEAPON);
        cache::relative_location = read<Vector3>(cache::root_component + RELATIVE_LOCATION);
        cache::my_team_id = (cache::player_state) ? read<int>(cache::player_state + TEAM_INDEX) : 0;

        if (settings::exploits::Spinbot)
        {
            auto mesh = read<uint64_t>(cache::local_pawn + MESH);
            if (mesh)
            {
                if (!spinbot_initialized)
                {
                    original_rotation = read<Vector3>(mesh + 0x150);
                    current_spin_angle = original_rotation.z; // Z ist Yaw
                    spinbot_initialized = true;
                }

                current_spin_angle += settings::exploits::SpinbotSpeed;
                if (current_spin_angle >= 360.0f)
                    current_spin_angle -= 360.0f;

                // Nur Yaw (z) ändern
                write<Vector3>(mesh + 0x150, Vector3(original_rotation.x, original_rotation.y, current_spin_angle));
            }

        }
        else
        {
            if (spinbot_initialized)
            {
                auto mesh = read<uint64_t>(cache::local_pawn + MESH);
                if (mesh)
                {
                    write<Vector3>(mesh + 0x150, original_rotation);
                }
                spinbot_initialized = false;
            }
        }
    }
    else
    {
        cache::root_component = 0;
        cache::player_state = 0;
        cache::current_weapon = 0;
        cache::relative_location = Vector3(0, 0, 0);
        cache::my_team_id = -1;
    }

    cache::closest_distance = FLT_MAX;
    cache::closest_mesh = NULL;
    cache::closest_pawn = NULL;

    uintptr_t game_state = read<uintptr_t>(cache::uworld + GAME_STATE);
	if (!game_state) return;
	uintptr_t player_array = read<uintptr_t>(game_state + PLAYER_ARRAY);
	if (!player_array) return;
	int player_count = read<int>(game_state + PLAYER_ARRAY + sizeof(uintptr_t));
	if (player_count < 0 || player_count > 4096) return;

    WeaponInfo weapon_info = cache::local_pawn ? get_weapon_info() : WeaponInfo{ "", 0.0f };

    ImGuiIO& io = ImGui::GetIO();
    const float screenWidth = io.DisplaySize.x;
    const float screenHeight = io.DisplaySize.y;
    auto draw_list = ImGui::GetForegroundDrawList();

    if (settings::debug && cache::local_pawn) {
        std::string weaponName = GetWeaponName(cache::local_pawn);
        char debug_text[256];
        sprintf_s(debug_text, "Weapon: %s | Type: %s | Smooth: %.2f", weaponName.c_str(), weapon_info.category.c_str(), weapon_info.smoothness);
        stroked_text(ImGui::GetFont(), 18.0f, ImVec2(10, 120), ImColor(255, 255, 0, 255), debug_text);
    }

    if (settings::visuals::spectator_list && cache::player_state)
    {

        std::vector<std::string> spectatorNames;

        for (int i = 0; i < player_count; i++) {
            uintptr_t player_state = read<uintptr_t>(player_array + (i * sizeof(uintptr_t)));
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

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(21.f / 255.f, 21.f / 255.f, 26.f / 255.f, 1.f));
        ImGui::Begin("Spectators", &settings::visuals::spectator_list,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::PopStyleColor();

        ImFont* ubu_child = ImGui::GetFont();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();
        float font_size = 15.f;
        ImU32 color = ImColor(255, 255, 255);


        draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 160, p.y + 35), ImColor(23, 23, 29, 255), 8, ImDrawCornerFlags_Top);

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


    if (settings::RenderCount)
    {
        char rendercount[64];
        sprintf_s(rendercount, "Render Count: %d", player_count);
        ImVec2 text_size = ImGui::CalcTextSize(rendercount);
        draw_list->AddText(ImGui::GetFont(), 15.0f, ImVec2((screenWidth - text_size.x) / 2.0f, 80.0f), ImColor(255, 0, 0, 255), rendercount);
    }

    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    if (settings::aimbot::show_fov)
    {
        ImColor fov_color = settings::colors::icFovColor;
        if (settings::aimbot::Rgb_Fov) {
            static float r = 1.0f, g = 0.0f, b = 0.0f;
            static int phase = 0;
            if (phase == 0) { g += 0.01f; if (g >= 1.0f) { g = 1.0f; phase = 1; } }
            else if (phase == 1) { r -= 0.01f; if (r <= 0.0f) { r = 0.0f; phase = 2; } }
            else if (phase == 2) { b += 0.01f; if (b >= 1.0f) { b = 1.0f; phase = 3; } }
            else if (phase == 3) { g -= 0.01f; if (g <= 0.0f) { g = 0.0f; phase = 4; } }
            else if (phase == 4) { r += 0.01f; if (r >= 1.0f) { r = 1.0f; phase = 5; } }
            else if (phase == 5) { b -= 0.01f; if (b <= 0.0f) { b = 0.0f; phase = 0; } }
            fov_color = ImColor(r, g, b);
        }
        draw_list->AddCircle(
            ImVec2(settings::screen_center_x, settings::screen_center_y),
            settings::aimbot::fov,
            fov_color, 64, 1.5f);
        if (settings::aimbot::fill_fov)
            draw_list->AddCircleFilled(
                ImVec2(settings::screen_center_x, settings::screen_center_y),
                settings::aimbot::fov,
                settings::colors::icFovFillColor, 64);
    }

    if (settings::aimbot::FOVArrows) {
        WeaponInfo weapon_info = get_weapon_info();
        if (weapon_info.smoothness > 0.0f) {
            for (int i = 0; i < player_count; i++) {
                uintptr_t player_state = read<uintptr_t>(player_array + (i * sizeof(uintptr_t)));
                if (!player_state) continue;

                if (settings::visuals::ignore_teamates && cache::local_pawn) {
                    int player_team_id = read<int>(player_state + TEAM_INDEX);
                    if (cache::my_team_id == player_team_id) {
                        continue;
                    }
                }

                uintptr_t pawn_private = read<uintptr_t>(player_state + PAWN_PRIVATE);
                if (!pawn_private || pawn_private == cache::local_pawn) continue;

                if (cache::local_pawn && is_dead(pawn_private)) continue;

                uintptr_t mesh = read<uintptr_t>(pawn_private + MESH);
                if (!mesh) continue;

                Vector3 headPos = get_entity_bone(mesh, 110);
                Vector2 screenPos = project_world_to_screen(headPos);

                float distance = cache::relative_location.distance(read<Vector3>(read<uintptr_t>(pawn_private + ROOT_COMPONENT) + RELATIVE_LOCATION)) / 100.0f;
                if (distance > settings::visuals::renderDistance) continue;

                ImVec2 direction(screenPos.x - center.x, screenPos.y - center.y);
                float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
                if (length < 1.0f) continue;

                if (length <= settings::aimbot::fov) continue;

                direction.x /= length;
                direction.y /= length;

                const float gapFromFov = 8.0f;    
                const float arrowSize = 12.0f;         
                const float arrowLength = arrowSize * 1.35f; 
                const float arrowWidth = arrowSize * 0.7f; 

                ImVec2 arrowBase(
                    center.x + direction.x * (settings::aimbot::fov + gapFromFov),
                    center.y + direction.y * (settings::aimbot::fov + gapFromFov)
                );

                draw_list->AddTriangleFilled(
                    ImVec2(arrowBase.x + direction.x * arrowLength, arrowBase.y + direction.y * arrowLength),
                    ImVec2(arrowBase.x - direction.y * arrowWidth, arrowBase.y + direction.x * arrowWidth),
                    ImVec2(arrowBase.x + direction.y * arrowWidth, arrowBase.y - direction.x * arrowWidth),
                    settings::aimbot::FOVArrowsColor
                );

                draw_list->AddTriangle(
                    ImVec2(arrowBase.x + direction.x * arrowLength, arrowBase.y + direction.y * arrowLength),
                    ImVec2(arrowBase.x - direction.y * arrowWidth, arrowBase.y + direction.x * arrowWidth),
                    ImVec2(arrowBase.x + direction.y * arrowWidth, arrowBase.y - direction.x * arrowWidth),
                    ImColor(0, 0, 0, 255),
                    1.0f
                );
            }
        }
    }

    static bool esp_enabled = true;
    if (settings::esp_toggle_enabled) {
        if (GetAsyncKeyState(settings::esp_toggle_key) & 1) {
            esp_enabled = !esp_enabled;
        }
    }
    // ESP must be enabled both by toggle key AND by settings::visuals::enable
    if (!esp_enabled || !settings::visuals::enable) {
        return;
    }

    for (int i = 0; i < player_count; i++)
    {
        uintptr_t player_state = read<uintptr_t>(player_array + (i * sizeof(uintptr_t)));
        if (!player_state) continue;
        
        // Early exit if no visuals enabled - performance optimization
        // Note: settings::visuals::enable is already checked above, so we know ESP is enabled here
        if (!(settings::visuals::box || settings::visuals::Filled_box || settings::visuals::Gradient_filled_box || settings::visuals::skeleton || settings::visuals::head_circle || settings::visuals::head_cone || settings::visuals::line || settings::visuals::platform || settings::visuals::name || settings::visuals::distance || settings::visuals::rank || settings::visuals::weapon_name || settings::visuals::target_line)) {
            continue;
        }

        uintptr_t pawn_private = read<uintptr_t>(player_state + PAWN_PRIVATE);
        if (!pawn_private || pawn_private == cache::local_pawn) continue;

        // Check team before expensive reads - performance optimization
        int player_team_id = read<int>(player_state + TEAM_INDEX);
        if (settings::visuals::ignore_teamates && cache::local_pawn) {
            if (cache::my_team_id > 0 && player_team_id > 0 && cache::my_team_id == player_team_id)
                continue;
        }

        if (cache::local_pawn && is_dead(pawn_private) && settings::aimbot::ignore_knocked) continue;

        // Cache root component and location together - performance optimization
		uintptr_t actorRootComponent = read<uintptr_t>(pawn_private + ROOT_COMPONENT);
		if (!actorRootComponent) continue;
		Vector3 actorRelativeLocation = read<Vector3>(actorRootComponent + RELATIVE_LOCATION);

        // Calculate distance early for distance checks
        float distance = 0.0f;
        if (cache::local_pawn) {
            distance = cache::relative_location.distance(actorRelativeLocation) / 100.0f;
        }
        else {
            distance = 50.0f;
        }
        
        // Early distance check before expensive mesh reads - performance optimization
        // ESP distance check (we know settings::visuals::enable is true here because of check above)
        if (distance > settings::visuals::renderDistance) {
            // If ESP distance too far, check if aimbot should still work
            if (!settings::aimbot::mouseAim || distance > settings::aimbot::aimbot_renderDistance) {
                continue;
            }
        }

        uintptr_t mesh = read<uintptr_t>(pawn_private + MESH);
        if (!mesh) continue;
        if (settings::radar::radar && distance <= 100.0f)
        {

            Vector3 bottom3d = get_entity_bone(mesh, 0);
            int ScreenX, ScreenY = 0;
            ImVec2 radarPos(settings::radar_position_x, settings::radar_position_y);
            project_world_to_radar(bottom3d, ScreenX, ScreenY, radarPos);
            
            if (ScreenX < radarPos.x) ScreenX = static_cast<int>(radarPos.x);
            if (ScreenX > radarPos.x + 200) ScreenX = static_cast<int>(radarPos.x + 200);
            if (ScreenY < radarPos.y) ScreenY = static_cast<int>(radarPos.y);
            if (ScreenY > radarPos.y + 200) ScreenY = static_cast<int>(radarPos.y + 200);
            
            ImGui::GetForegroundDrawList()->AddCircleFilled(
                ImVec2(ScreenX, ScreenY), 
                settings::radar::RedDotSize, 
                ImColor(255, 0, 0, 255), 
                64
            );
        }



        // Distance check already done above, now get bone positions
        Vector3 head3d = get_entity_bone(mesh, 110);
        Vector2 head2d = project_world_to_screen(head3d);
        if (!std::isfinite(head2d.x) || !std::isfinite(head2d.y)) continue;
        Vector3 bottom3d = get_entity_bone(mesh, 0);
        Vector2 bottom2d = project_world_to_screen(bottom3d);
        if (!std::isfinite(bottom2d.x) || !std::isfinite(bottom2d.y)) continue;
        Vector2 boxHead = project_world_to_screen(Vector3(head3d.x, head3d.y, head3d.z + 20.0f));
        if (!std::isfinite(boxHead.x) || !std::isfinite(boxHead.y)) continue;
        float box_height = abs(boxHead.y - bottom2d.y);
        float box_width = box_height * settings::visuals::box_width;
        if (box_height <= 0.0f || box_height > 3000.0f) continue;
        if (box_width <= 0.0f || box_width > 3000.0f) continue;
        bool visible = settings::visuals::visible_check ? is_visible(mesh) : true;
        
        ImColor box_color;
        ImColor skeleton_color;
        

        if (settings::visuals::Rainbow_esp && settings::visuals::box) {
            box_color = GetRainbowColor(2.0f, 255);
        } else {
            box_color = visible ? settings::colors::icBoxColorVisible : settings::colors::icBoxColorInvisible;
        }
        
        if (settings::visuals::Rainbow_esp && settings::visuals::skeleton) {
            skeleton_color = GetRainbowColor(2.3f, 255);
        } else {
            skeleton_color = visible ? settings::colors::icSkeletonColorVisible : settings::colors::icSkeletonColorInvisible;
            settings::colors::icTracerColorInvisible;
        }

        float x = boxHead.x - box_width / 2;
        float y = boxHead.y;
        const float configured_rounding = std::clamp(settings::visuals::box_rounding, 0.0f, 30.0f);


        if (settings::visuals::head_circle)
        {
            ImColor head_circle_color = visible ? settings::colors::icHeadCircleColorVisible : settings::colors::icHeadCircleColorInvisible;
            draw_list->AddCircle(ImVec2(head2d.x, head2d.y), settings::visuals::head_circle_radius, head_circle_color, 64, settings::visuals::head_circle_thickness);
        }

        if (settings::visuals::head_cone)
        {
            draw_head_cone(mesh);
        }

        
        if (settings::visuals::Filled_box)
        {
            ImU32 filled_box_color;
            if (settings::visuals::Rainbow_esp) {
                filled_box_color = GetRainbowColor(1.8f, 150);
            } else {
                filled_box_color = ImGui::ColorConvertFloat4ToU32(settings::colors::icFilledBoxColor);
            }
           
            if (settings::visuals::boxType == boxType::rounded)
            {
                float rounding = configured_rounding;
                draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + box_width, y + box_height), filled_box_color, rounding);
            }
            else
            {
                draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + box_width, y + box_height), filled_box_color);
            }
        }

        if (settings::visuals::Gradient_filled_box)
        {

            if (settings::visuals::visible_check && !visible)
            {

                DrawGradientFilledBox(
                    x, 
                    y, 
                    box_width, 
                    box_height, 
                    ImGui::ColorConvertFloat4ToU32(ImColor(255, 0, 0, 120)),
                    ImGui::ColorConvertFloat4ToU32(ImColor(255, 0, 0, 120)), 
                    settings::visuals::Outlined_box
                );
            }
            else
            {
                DrawGradientFilledBox(
                    x, 
                    y, 
                    box_width, 
                    box_height, 
                    ImGui::ColorConvertFloat4ToU32(settings::colors::icGradientBoxColorTop), 
                    ImGui::ColorConvertFloat4ToU32(settings::colors::icGradientBoxColorBottom), 
                    settings::visuals::Outlined_box
                );
            }
        }


        if (settings::visuals::Filled_box && settings::visuals::Rainbow_esp)
        {

            if (settings::visuals::visible_check && !visible)
            {

                ImU32 red_color = ImGui::ColorConvertFloat4ToU32(ImColor(255, 0, 0, 120));
                if (settings::visuals::boxType == boxType::rounded)
                {
                    float rounding = configured_rounding;
                    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + box_width, y + box_height), red_color, rounding);
                }
                else
                {
                    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + box_width, y + box_height), red_color);
                }
            }
            else
            {
                DrawRainbowFilledBox(
                    x, 
                    y, 
                    box_width, 
                    box_height, 
                    settings::visuals::Outlined_box
                );
            }
        }

        if (settings::visuals::box) {
            if (settings::visuals::boxType == boxType::normal) {
                if (settings::visuals::Outlined_box) {
                    ImColor outline_color = ImColor(0, 0, 0, 255);
                    float outline_thickness = settings::visuals::box_thickness + 2.0f;
                    draw_list->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + box_width + 1, y + box_height + 1), outline_color, 0.0f, 0, outline_thickness);
                }
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + box_width, y + box_height), box_color, 0.0f, 0, settings::visuals::box_thickness);
            }
            else if (settings::visuals::boxType == boxType::corner) {
                draw_corner_box(x, y, box_width, box_height, box_color, settings::visuals::Outlined_box, settings::visuals::box_thickness);
            }
            else if (settings::visuals::boxType == boxType::rounded) {
                float rounding = configured_rounding;
                if (settings::visuals::Outlined_box) {
                    ImColor outline_color = ImColor(0, 0, 0, 255);
                    float outline_thickness = settings::visuals::box_thickness + 2.0f;
                    draw_list->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + box_width + 1, y + box_height + 1), outline_color, rounding + 2.0f, 0, outline_thickness);
                }
                draw_list->AddRect(ImVec2(x, y), ImVec2(x + box_width, y + box_height), box_color, rounding, 0, settings::visuals::box_thickness);
            }
        }

        if (settings::visuals::skeleton)
        {
            if (settings::visuals::skeleton_type == 0) {

                Vector3 head = get_entity_bone(mesh, 110);
                Vector2 wts_head = project_world_to_screen(head);
                Vector3 neck = get_entity_bone(mesh, 67);
                Vector2 wts_neck = project_world_to_screen(neck);
                Vector3 chest = get_entity_bone(mesh, 7);
                Vector2 wts_chest = project_world_to_screen(chest);
                Vector3 pelvis = get_entity_bone(mesh, 2);
                Vector2 wts_pelvis = project_world_to_screen(pelvis);

                Vector3 right_shoulder = get_entity_bone(mesh, 9);
                Vector2 wts_right_shoulder = project_world_to_screen(right_shoulder);
                Vector3 right_elbow = get_entity_bone(mesh, 10);
                Vector2 wts_right_elbow = project_world_to_screen(right_elbow);
                Vector3 right_wrist = get_entity_bone(mesh, 11);
                Vector2 wts_right_wrist = project_world_to_screen(right_wrist);

                Vector3 left_shoulder = get_entity_bone(mesh, 38);
                Vector2 wts_left_shoulder = project_world_to_screen(left_shoulder);
                Vector3 left_elbow = get_entity_bone(mesh, 39);
                Vector2 wts_left_elbow = project_world_to_screen(left_elbow);
                Vector3 left_wrist = get_entity_bone(mesh, 40);
                Vector2 wts_left_wrist = project_world_to_screen(left_wrist);

                Vector3 right_hip = get_entity_bone(mesh, 71);
                Vector2 wts_right_hip = project_world_to_screen(right_hip);
                Vector3 right_knee = get_entity_bone(mesh, 72);
                Vector2 wts_right_knee = project_world_to_screen(right_knee);
                Vector3 right_ankle = get_entity_bone(mesh, 73);
                Vector2 wts_right_ankle = project_world_to_screen(right_ankle);
                Vector3 right_foot_upper = get_entity_bone(mesh, 86);
                Vector2 wts_right_foot_upper = project_world_to_screen(right_foot_upper);
                Vector3 right_foot = get_entity_bone(mesh, 76);
                Vector2 wts_right_foot = project_world_to_screen(right_foot);

                Vector3 left_hip = get_entity_bone(mesh, 78);
                Vector2 wts_left_hip = project_world_to_screen(left_hip);
                Vector3 left_knee = get_entity_bone(mesh, 79);
                Vector2 wts_left_knee = project_world_to_screen(left_knee);
                Vector3 left_ankle = get_entity_bone(mesh, 80);
                Vector2 wts_left_ankle = project_world_to_screen(left_ankle);
                Vector3 left_foot_upper = get_entity_bone(mesh, 87);
                Vector2 wts_left_foot_upper = project_world_to_screen(left_foot_upper);
                Vector3 left_foot = get_entity_bone(mesh, 83);
                Vector2 wts_left_foot = project_world_to_screen(left_foot);


                if (settings::visuals::Outlined_skeleton)
                {
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_head.x, wts_head.y), ImVec2(wts_neck.x, wts_neck.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_neck.x, wts_neck.y), ImVec2(wts_chest.x, wts_chest.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_pelvis.x, wts_pelvis.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_right_shoulder.x, wts_right_shoulder.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_shoulder.x, wts_right_shoulder.y), ImVec2(wts_right_elbow.x, wts_right_elbow.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_elbow.x, wts_right_elbow.y), ImVec2(wts_right_wrist.x, wts_right_wrist.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_left_shoulder.x, wts_left_shoulder.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_shoulder.x, wts_left_shoulder.y), ImVec2(wts_left_elbow.x, wts_left_elbow.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_elbow.x, wts_left_elbow.y), ImVec2(wts_left_wrist.x, wts_left_wrist.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_pelvis.x, wts_pelvis.y), ImVec2(wts_right_hip.x, wts_right_hip.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_hip.x, wts_right_hip.y), ImVec2(wts_right_knee.x, wts_right_knee.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_knee.x, wts_right_knee.y), ImVec2(wts_right_ankle.x, wts_right_ankle.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_ankle.x, wts_right_ankle.y), ImVec2(wts_right_foot_upper.x, wts_right_foot_upper.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_foot_upper.x, wts_right_foot_upper.y), ImVec2(wts_right_foot.x, wts_right_foot.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_pelvis.x, wts_pelvis.y), ImVec2(wts_left_hip.x, wts_left_hip.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_hip.x, wts_left_hip.y), ImVec2(wts_left_knee.x, wts_left_knee.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_knee.x, wts_left_knee.y), ImVec2(wts_left_ankle.x, wts_left_ankle.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_ankle.x, wts_left_ankle.y), ImVec2(wts_left_foot_upper.x, wts_left_foot_upper.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_foot_upper.x, wts_left_foot_upper.y), ImVec2(wts_left_foot.x, wts_left_foot.y), ImColor(0, 0, 0, 255), settings::visuals::skeleton_thickness + 2.0f);
                }
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_head.x, wts_head.y), ImVec2(wts_neck.x, wts_neck.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_neck.x, wts_neck.y), ImVec2(wts_chest.x, wts_chest.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_pelvis.x, wts_pelvis.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_right_shoulder.x, wts_right_shoulder.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_shoulder.x, wts_right_shoulder.y), ImVec2(wts_right_elbow.x, wts_right_elbow.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_elbow.x, wts_right_elbow.y), ImVec2(wts_right_wrist.x, wts_right_wrist.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_chest.x, wts_chest.y), ImVec2(wts_left_shoulder.x, wts_left_shoulder.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_shoulder.x, wts_left_shoulder.y), ImVec2(wts_left_elbow.x, wts_left_elbow.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_elbow.x, wts_left_elbow.y), ImVec2(wts_left_wrist.x, wts_left_wrist.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_pelvis.x, wts_pelvis.y), ImVec2(wts_right_hip.x, wts_right_hip.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_hip.x, wts_right_hip.y), ImVec2(wts_right_knee.x, wts_right_knee.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_knee.x, wts_right_knee.y), ImVec2(wts_right_ankle.x, wts_right_ankle.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_ankle.x, wts_right_ankle.y), ImVec2(wts_right_foot_upper.x, wts_right_foot_upper.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_right_foot_upper.x, wts_right_foot_upper.y), ImVec2(wts_right_foot.x, wts_right_foot.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_pelvis.x, wts_pelvis.y), ImVec2(wts_left_hip.x, wts_left_hip.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_hip.x, wts_left_hip.y), ImVec2(wts_left_knee.x, wts_left_knee.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_knee.x, wts_left_knee.y), ImVec2(wts_left_ankle.x, wts_left_ankle.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_ankle.x, wts_left_ankle.y), ImVec2(wts_left_foot_upper.x, wts_left_foot_upper.y), skeleton_color, settings::visuals::skeleton_thickness);
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(wts_left_foot_upper.x, wts_left_foot_upper.y), ImVec2(wts_left_foot.x, wts_left_foot.y), skeleton_color, settings::visuals::skeleton_thickness);
            }
            else if (settings::visuals::skeleton_type == 1) {
                if (settings::visuals::Outlined_skeleton) {
                    SkeltonESP(mesh);
                }
                else {
                    SkeltonESP(mesh);
                }
            }
            else if (settings::visuals::skeleton_type == 2) {
                if (settings::visuals::Outlined_skeleton) {
                    SkeltonESP_Smooth(mesh);
                }
                else {
                    SkeltonESP_Smooth(mesh);
                }
            }
        }

        if (settings::visuals::line)
        {
            ImU32 tracer_color = visible ? settings::colors::icTracerColorVisible
                : settings::colors::icTracerColorInvisible;

            ImVec2 startPos;
            switch (settings::visuals::snaplineType)
            {
            case snaplinePosition::up:
                startPos = ImVec2(settings::screen_center_x, 0);
                break;
            case snaplinePosition::middle:
                startPos = ImVec2(settings::screen_center_x, settings::screen_center_y);
                break;
            case snaplinePosition::down:
                startPos = ImVec2(settings::screen_center_x, settings::height);
                break;
            default:
                startPos = ImVec2(settings::screen_center_x, 0);
                break;
            }

            ImVec2 endPos;
            switch (settings::visuals::snaplineType)
            {
            case snaplinePosition::down:
                endPos = ImVec2(bottom2d.x, bottom2d.y);
                break;
            case snaplinePosition::middle:
            {
                float boxMiddleY = boxHead.y + (bottom2d.y - boxHead.y) / 2;
                endPos = ImVec2(head2d.x, boxMiddleY);
            }
            break;
            default:
                endPos = ImVec2(head2d.x, boxHead.y);
                break;
            }

            if (settings::visuals::Outlined_snapline)
            {
                ImColor outline_color = ImColor(0, 0, 0, 255);
                float outline_thickness = settings::visuals::snapline_thickness + 2.0f;

                draw_list->AddLine(
                    startPos,
                    endPos,
                    outline_color,
                    outline_thickness
                );
            }

            draw_list->AddLine(
                startPos,
                endPos,
                tracer_color,
                (float)settings::visuals::snapline_thickness
            );
        }

        if (settings::visuals::name)
        {
            std::string playerUsername = GetPlayerName(player_state);
            ImVec2 textSize = ImGui::CalcTextSize(playerUsername.c_str());
            stroked_text(ImGui::GetFont(), 22.0f, ImVec2(boxHead.x - textSize.x / 2, boxHead.y - 10 - textSize.y / 2), settings::colors::icNameColor, playerUsername.c_str());
        }

        if (settings::visuals::platform)
        {
            uintptr_t platformPtr = read<uintptr_t>(player_state + PLATFORM);
            wchar_t platformChar[64] = { 0 };
            intel_driver::read_physical(reinterpret_cast<PVOID>(platformPtr), reinterpret_cast<uint8_t*>(platformChar), sizeof(platformChar));
            std::wstring platformWstr(platformChar);
            std::string platformStr(platformWstr.begin(), platformWstr.end());

            ImColor platformColor = settings::colors::icPlatformColor;
            std::string displayStr = platformStr;
            if (platformStr == "win" || platformStr == "Windows" || platformStr == "pc" || platformStr == "PC") {
                displayStr = "Windows";
            } else if (platformStr == "ps" || platformStr == "PlayStation") {
                displayStr = "PlayStation";
            } else if (platformStr == "xbox" || platformStr == "Xbox") {
                displayStr = "Xbox";
            } else if (platformStr == "switch" || platformStr == "Nintendo Switch") {
                displayStr = "Nintendo Switch";
            } else if (platformStr == "android" || platformStr == "Android") {
                displayStr = "Android";
            }

            ImVec2 textSize = ImGui::CalcTextSize(displayStr.c_str());
            float offsetY = settings::visuals::name ? 25.0f : 10.0f;
            stroked_text(ImGui::GetFont(), 22.0f, ImVec2(boxHead.x - textSize.x / 2, boxHead.y - offsetY - textSize.y / 2), platformColor, displayStr.c_str());
        }


        if (settings::visuals::distance)
        {
            char dist[64];
            sprintf_s(dist, "%.fm", distance);
            ImVec2 textSize = ImGui::CalcTextSize(dist);
            stroked_text(ImGui::GetFont(), 22.0f, ImVec2(bottom2d.x - textSize.x / 2, bottom2d.y + 10 - textSize.y / 2), settings::colors::icDistanceColor, dist);
        }


        if (settings::visuals::weapon_name)
        {
            std::string weaponName = GetWeaponName(pawn_private);
            if (!weaponName.empty()) {
                ImVec2 textSize = ImGui::CalcTextSize(weaponName.c_str());
                float offsetY = settings::visuals::distance ? 35.0f : 10.0f;
                stroked_text(ImGui::GetFont(), 13.0f, ImVec2(bottom2d.x - textSize.x / 2, bottom2d.y + offsetY - textSize.y / 2), ImColor(255, 255, 255), weaponName.c_str());
            }
        }



        if (settings::visuals::rank)
        {
            uintptr_t habaneroComponent = read<uintptr_t>(player_state + HABANERO_COMPONENT);
            uint32_t rank = read<uint32_t>(habaneroComponent + RANKED_PROGRESS + 0x10);
            std::string rankName = getRank(rank);
            ImVec2 textSize = ImGui::CalcTextSize(rankName.c_str());
            float offsetY = (settings::visuals::distance ? 35.0f : 0.0f) + (settings::visuals::weapon_name ? 15.0f : 0.0f) + 10.0f;
            
            ImColor rankColor = ImColor(255, 255, 255);
            
            if (rankName.find("Bronze") != std::string::npos) {
                rankColor = ImColor(205, 127, 50); 
            }
            else if (rankName.find("Silver") != std::string::npos) {
                rankColor = ImColor(192, 192, 192);
            }
            else if (rankName.find("Gold") != std::string::npos) {
                rankColor = ImColor(255, 215, 0); 
            }
            else if (rankName.find("Platinum") != std::string::npos) {
                rankColor = ImColor(0, 191, 255);
            }
            else if (rankName.find("Diamond") != std::string::npos) {
                rankColor = ImColor(0, 0, 139);
            }
            else if (rankName.find("Elite") != std::string::npos) {
                rankColor = ImColor(64, 64, 64);
            }
            else if (rankName.find("Champion") != std::string::npos) {
                rankColor = ImColor(255, 140, 0);
            }
            else if (rankName.find("Unreal") != std::string::npos) {
                rankColor = ImColor(128, 0, 128);
            }
            
            stroked_text(ImGui::GetFont(), 22.0f, ImVec2(bottom2d.x - textSize.x / 2, bottom2d.y + offsetY - textSize.y / 2), rankColor, rankName.c_str());
        }

        auto distToCross = getCrossDistance(head2d.x, head2d.y, settings::width / 2, settings::height / 2);
        if (distToCross <= settings::aimbot::fov && distToCross < cache::closest_distance)
        {
            if (distance <= settings::aimbot::aimbot_renderDistance) {
                cache::closest_distance = distToCross;
                cache::closest_mesh = mesh;
                cache::closest_pawn = pawn_private;
            }
        }
        if (settings::aimbot::aimbot_type == 0) {
            if (((settings::aimbot::mouseAim && GetAsyncKeyState(settings::aimbot::current_key)) || settings::aimbot::controller_aim_pressed)
                && cache::closest_mesh && (!settings::aimbot::ignore_knocked || !is_dead(cache::closest_pawn)))
            {
                memoryaim(cache::closest_mesh);
            }
        }
        else if (settings::aimbot::aimbot_type == 1) {
            SHORT keyState = GetAsyncKeyState(VK_F2);
            if (keyState) {
                (50, 0);
            }
        }
    }



    if (settings::exploits::FOVChanger)
    {
        static bool initialized = false;
        static float originalFOV = 0.0f;
        static float originalBaseFOV = 0.0f;

        uintptr_t PlayerCameraManager = read<uintptr_t>(cache::player_controller + PLAYERCAMERAMANAGER);
        if ((PlayerCameraManager)) {

            if (settings::exploits::FOVChanger) {
                if (!initialized) {
                    originalFOV = read<float>(PlayerCameraManager + DEFAULTFOV + 0x4);
                    originalBaseFOV = read<float>(PlayerCameraManager + BASEFOV);
                    initialized = true;
                }
                write<float>(PlayerCameraManager + DEFAULTFOV + 0x4, settings::exploits::FOVVALUE);
                write<float>(PlayerCameraManager + BASEFOV, settings::exploits::FOVVALUE);
            }
            else if (initialized) {
                write<float>(PlayerCameraManager + DEFAULTFOV + 0x4, originalFOV);
                write<float>(PlayerCameraManager + BASEFOV, originalBaseFOV);
                initialized = false;
            }
        }
    }



    handle_triggerbot();

    if (settings::exploits::Airstuck) {
        if (GetAsyncKeyState(VK_SHIFT)) {
            write<float>(cache::local_pawn + 0x68, 1e-8f);
        }
        else {
            write<float>(cache::local_pawn + 0x68, 1.0f);
        }
    }

    if (settings::exploits::Carfly)
    {
        uintptr_t CurrentVehicle = read<DWORD_PTR>(cache::local_pawn + 0x2C08); 

        if (CurrentVehicle && GetAsyncKeyState(VK_SPACE))
        {
            write<bool>(CurrentVehicle + 0x8A2, false); 
        }
        else {
            write<bool>(CurrentVehicle + 0x8A2, true); 
        }
    }



    if (settings::exploits::SpeedHack) {
        if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
            std::lock_guard<std::mutex> lock(cache_mutex);
            for (const auto& player : cached_players) {
                if (player.pawn_private && driver::IsValidPointer(player.pawn_private)) {
                    driver::write<float>(player.pawn_private + CUSTOM_TIME_DILATION, 1.0f);
                }
            }
            return;
        }
        if (!cache::local_pawn || !cache::root_component ||
            !driver::IsValidPointer(cache::local_pawn) || !driver::IsValidPointer(cache::root_component)) {
            return;
        }
        Vector3 local_position = driver::read<Vector3>(cache::root_component + RELATIVE_LOCATION);
        Vector3 forward_vector = get_forward_vector(cache::rotation);
        float distance = 100.0f;
        Vector3 target_position = local_position + (forward_vector * distance);
        target_position.z += 20.0f;
        uintptr_t target_pawn = get_closest_enemy();
        if (!target_pawn || !driver::IsValidPointer(target_pawn)) {
            return;
        }
        uintptr_t enemy_root_component = driver::read<uintptr_t>(target_pawn + ROOT_COMPONENT);
        if (!enemy_root_component || !driver::IsValidPointer(enemy_root_component)) {
            return;
        }
        Vector3 current_position = driver::read<Vector3>(enemy_root_component + RELATIVE_LOCATION);
        //  driver::write<float>(target_pawn + CUSTOM_TIME_DILATION, 1.0f);
        const int steps = 5;
        Vector3 step_vector = (target_position - current_position);
        uintptr_t enemy_character_movement = driver::read<uintptr_t>(target_pawn + CHARACTER_MOVEMENT);
        for (int i = 0; i < steps; ++i) {
            Vector3 interpolated_position = current_position + (step_vector * static_cast<double>(i + 1));
            driver::write<Vector3>(enemy_root_component + RELATIVE_LOCATION, interpolated_position);

            if (enemy_character_movement && driver::IsValidPointer(enemy_character_movement)) {
                driver::write<Vector3>(enemy_character_movement + LAST_UPDATE_LOCATION, interpolated_position);
                driver::write<Vector3>(enemy_character_movement + VELOCITY_OFFSET, step_vector * 100.0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        driver::write<Vector3>(enemy_root_component + RELATIVE_LOCATION, target_position);
        if (enemy_character_movement && driver::IsValidPointer(enemy_character_movement)) {
            driver::write<Vector3>(enemy_character_movement + LAST_UPDATE_LOCATION, target_position);
            driver::write<Vector3>(enemy_character_movement + VELOCITY_OFFSET, Vector3(0.0, 0.0, 0.0));
        }
    }



    






    if (cache::closest_mesh && settings::visuals::enable && settings::visuals::target_line) {
        Vector3 head3d = get_entity_bone(cache::closest_mesh, 110);
        Vector2 head2d = project_world_to_screen(head3d);
        if (!std::isfinite(head2d.x) || !std::isfinite(head2d.y)) return;
        ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::GetForegroundDrawList()->AddLine(center, ImVec2(head2d.x, head2d.y), settings::colors::icTargetLineColor, 2.5f);
    }
    } catch (...) {
        static int once = 0;
        if (!once) {
            std::ofstream log("crashlog.txt", std::ios::app);
            if (log.is_open()) log << "[frame] actorLoop exception (swallowed)" << std::endl;
            once = 1;
        }
        return;
    }
}

__forceinline auto SkeltonESP(uintptr_t mesh) -> void
{
    ImColor skeleton_color;
    if (settings::visuals::Rainbow_esp && settings::visuals::skeleton) {
        skeleton_color = GetRainbowColor(2.3f, 255);
    } else {
        skeleton_color = is_visible(mesh) ? settings::colors::icSkeletonColorVisible : settings::colors::icSkeletonColorInvisible;
    }

    Vector3 bonePositions[] = {
        get_entity_bone(mesh, 110),
        get_entity_bone(mesh, 2),
        get_entity_bone(mesh, 66),
        get_entity_bone(mesh, 9),  
        get_entity_bone(mesh, 38),
        get_entity_bone(mesh, 10),
        get_entity_bone(mesh, 39),
        get_entity_bone(mesh, 11),
        get_entity_bone(mesh, 40),
        get_entity_bone(mesh, 78),
        get_entity_bone(mesh, 71),
        get_entity_bone(mesh, 79),
        get_entity_bone(mesh, 72),
        get_entity_bone(mesh, 75),        
        get_entity_bone(mesh, 82),
    };
    Vector2 bonePositionsOut[16];
    for (int i = 0; i < 16; ++i) {
        bonePositionsOut[i] = project_world_to_screen(bonePositions[i]);
    }
    if (settings::visuals::Outlined_skeleton) {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        float thick = (float)settings::visuals::skeleton_thickness + 2.0f;
        ImColor outline_col = ImColor(0, 0, 0, 255);
        dl->AddLine(ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[7].x, bonePositionsOut[7].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[8].x, bonePositionsOut[8].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[13].x, bonePositionsOut[13].y), ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), outline_col, thick);
        dl->AddLine(ImVec2(bonePositionsOut[14].x, bonePositionsOut[14].y), ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), outline_col, thick);
    }
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[7].x, bonePositionsOut[7].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[8].x, bonePositionsOut[8].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[13].x, bonePositionsOut[13].y), ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddLine(ImVec2(bonePositionsOut[14].x, bonePositionsOut[14].y), ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), skeleton_color, (float)settings::visuals::skeleton_thickness);
}

__forceinline auto SkeltonESP_Smooth(uintptr_t mesh) -> void
{
    ImColor skeleton_color;
    if (settings::visuals::Rainbow_esp && settings::visuals::skeleton) {
        skeleton_color = GetRainbowColor(2.3f, 255);
    } else {
        skeleton_color = is_visible(mesh) ? settings::colors::icSkeletonColorVisible : settings::colors::icSkeletonColorInvisible;
    }
    Vector2 head = project_world_to_screen(get_entity_bone(mesh, 110));
    Vector2 neck = project_world_to_screen(get_entity_bone(mesh, 67));
    Vector2 chest = project_world_to_screen(get_entity_bone(mesh, 7));
    Vector2 pelvis = project_world_to_screen(get_entity_bone(mesh, 2));
    Vector2 right_shoulder = project_world_to_screen(get_entity_bone(mesh, 9));
    Vector2 right_elbow = project_world_to_screen(get_entity_bone(mesh, 10));
    Vector2 right_wrist = project_world_to_screen(get_entity_bone(mesh, 11));
    Vector2 left_shoulder = project_world_to_screen(get_entity_bone(mesh, 38));
    Vector2 left_elbow = project_world_to_screen(get_entity_bone(mesh, 39));
    Vector2 left_wrist = project_world_to_screen(get_entity_bone(mesh, 40));
    Vector2 right_hip = project_world_to_screen(get_entity_bone(mesh, 71));
    Vector2 right_knee = project_world_to_screen(get_entity_bone(mesh, 72));
    Vector2 right_ankle = project_world_to_screen(get_entity_bone(mesh, 73));
    Vector2 left_hip = project_world_to_screen(get_entity_bone(mesh, 78));
    Vector2 left_knee = project_world_to_screen(get_entity_bone(mesh, 79));
    Vector2 left_ankle = project_world_to_screen(get_entity_bone(mesh, 80));

    if (settings::visuals::Outlined_skeleton) {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        float thick = (float)settings::visuals::skeleton_thickness + 2.0f;
        ImColor outline_col = ImColor(0, 0, 0, 255);
        dl->AddBezierCubic(ImVec2(head.x, head.y), ImVec2(neck.x, neck.y), ImVec2(chest.x, chest.y), ImVec2(pelvis.x, pelvis.y), outline_col, thick);
        dl->AddBezierCubic(ImVec2(chest.x, chest.y), ImVec2(right_shoulder.x, right_shoulder.y), ImVec2(right_elbow.x, right_elbow.y), ImVec2(right_wrist.x, right_wrist.y), outline_col, thick);
        dl->AddBezierCubic(ImVec2(chest.x, chest.y), ImVec2(left_shoulder.x, left_shoulder.y), ImVec2(left_elbow.x, left_elbow.y), ImVec2(left_wrist.x, left_wrist.y), outline_col, thick);
        dl->AddBezierCubic(ImVec2(pelvis.x, pelvis.y), ImVec2(right_hip.x, right_hip.y), ImVec2(right_knee.x, right_knee.y), ImVec2(right_ankle.x, right_ankle.y), outline_col, thick);
        dl->AddBezierCubic(ImVec2(pelvis.x, pelvis.y), ImVec2(left_hip.x, left_hip.y), ImVec2(left_knee.x, left_knee.y), ImVec2(left_ankle.x, left_ankle.y), outline_col, thick);
    }
    ImGui::GetForegroundDrawList()->AddBezierCubic(ImVec2(head.x, head.y), ImVec2(neck.x, neck.y), ImVec2(chest.x, chest.y), ImVec2(pelvis.x, pelvis.y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddBezierCubic(ImVec2(chest.x, chest.y), ImVec2(right_shoulder.x, right_shoulder.y), ImVec2(right_elbow.x, right_elbow.y), ImVec2(right_wrist.x, right_wrist.y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddBezierCubic(ImVec2(chest.x, chest.y), ImVec2(left_shoulder.x, left_shoulder.y), ImVec2(left_elbow.x, left_elbow.y), ImVec2(left_wrist.x, left_wrist.y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddBezierCubic(ImVec2(pelvis.x, pelvis.y), ImVec2(right_hip.x, right_hip.y), ImVec2(right_knee.x, right_knee.y), ImVec2(right_ankle.x, right_ankle.y), skeleton_color, (float)settings::visuals::skeleton_thickness);
    ImGui::GetForegroundDrawList()->AddBezierCubic(ImVec2(pelvis.x, pelvis.y), ImVec2(left_hip.x, left_hip.y), ImVec2(left_knee.x, left_knee.y), ImVec2(left_ankle.x, left_ankle.y), skeleton_color, (float)settings::visuals::skeleton_thickness);
}

void AdjustPlayerSize()
{
    if (settings::exploits2::Playersize_big_tiny)
    {
        auto Mesh = read<uint64_t>(cache::local_pawn + MESH);
        if (settings::exploits2::bigortiny == 0) 
        {
            write<Vector3>(Mesh + 0x168, Vector3(settings::exploits2::Playersizebig, settings::exploits2::Playersizebig, settings::exploits2::Playersizebig));
        }
        else 
        {
            write<Vector3>(Mesh + 0x168, Vector3(settings::exploits2::Playersizetiny, settings::exploits2::Playersizetiny, settings::exploits2::Playersizetiny));
        }
    }
}