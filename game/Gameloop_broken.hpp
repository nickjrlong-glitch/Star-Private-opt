#include "../driver/comm.hpp"
#include "../sdk-offsets/sdk.hpp"
#include "../sdk-offsets/offsets.hpp"
#include "../imgui/imgui.h"
#include "../settings/settings.hpp"
#include "visuals/Esp-Draw.hpp"
#include <algorithm> // Für std::max
#define VELOCITY 0x140 
#include <cfloat> // Für FLT_EPSILON
#include <algorithm> // Für std::min
#include "aimbot/Aimbot.hpp"
#include "visuals/chams.hpp"

// Radar-Einstellungen (global sichtbar)
inline float radar_dist = 50;
inline float radar_position_x = 20;
inline float radar_position_y = 50;
inline float radar_size = 200;
inline float radar_range = 60.f;

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
                triggerbot.triggerbot_key = i;
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

// Prediction Funktion
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

// Prediction Controller
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

// Controller-Integration für Radar-Bewegung
#ifdef _WIN32
#include <XInput.h>
#pragma comment(lib, "XInput.lib")

void UpdateRadarPositionWithController(float sensitivity = 5.0f) {
    XINPUT_STATE state;
    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        // Deadzone-Handling
        const float deadzone = 0.2f;
        float stickX = state.Gamepad.sThumbLX / 32767.0f;
        float stickY = state.Gamepad.sThumbLY / 32767.0f;

        if (fabs(stickX) < deadzone) stickX = 0.0f;
        if (fabs(stickY) < deadzone) stickY = 0.0f;

        // Radar-Position aktualisieren
        settings::radar_position_x += stickX * sensitivity;
        settings::radar_position_y -= stickY * sensitivity; // Y-Achse invertieren

        // Grenzen setzen (verhindert, dass der Radar aus dem Bildschirm läuft)
        ImGuiIO& io = ImGui::GetIO();
        settings::radar_position_x = std::clamp(settings::radar_position_x, 0.0f, io.DisplaySize.x - settings::radar_size);
        settings::radar_position_y = std::clamp(settings::radar_position_y, 0.0f, io.DisplaySize.y - settings::radar_size);
    }
}
#else
// Fallback für Nicht-Windows-Systeme (kann mit anderen Controller-APIs erweitert werden)
void UpdateRadarPositionWithController(float sensitivity = 5.0f) {
    // Platzhalter für andere Controller-APIs
}
#endif

void handle_triggerbot()
{
    if (!triggerbot.triggerbot_enable)
        return;

    if (GetAsyncKeyState(triggerbot.triggerbot_key))
    {
        uintptr_t target_pawn = cache::closest_pawn;

        if (target_pawn && target_pawn != cache::local_pawn)
        {
            if (settings::aimbot::ignore_knocked && is_dead(target_pawn))
            {
                return; // Ignore knocked players
            }
            
            if (settings::visuals::ignore_teamates) {
                uintptr_t player_state = read<uintptr_t>(target_pawn + PLAYER_STATE);
                int player_team_id = read<int>(player_state + TEAM_INDEX);
                if (cache::my_team_id == player_team_id) {
                    return; // Don't shoot teammates
                }
            }
            
            // Check distance
            uintptr_t actorRootComponent = read<uintptr_t>(target_pawn + ROOT_COMPONENT);
            Vector3 actorRelativeLocation = read<Vector3>(actorRootComponent + RELATIVE_LOCATION);
            float distance = cache::localRelativeLocation.distance(actorRelativeLocation) / 100.0f;

            if (distance < triggerbot.triggerbot_distance)
            {
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }
        }
    }
}

void draw_head_cone(uintptr_t mesh) {
    if (!mesh) return;

    Vector3 head3d = get_entity_bone(mesh, 110);
    Vector3 top_of_cone3d = Vector3(head3d.x, head3d.y, head3d.z + 15.f); // Even lower height for a flatter look
    Vector2 top_of_cone2d = project_world_to_screen(top_of_cone3d);
    
    int num_segments = 20; // More segments for a smoother circle
    float radius = 55.f; // Even wider radius
    float thickness = 1.0f;

    ImColor color = settings::colors::icHeadConeColor;
    auto draw_list = ImGui::GetForegroundDrawList();

    Vector2 base_points2d[20];
    for (int i = 0; i < num_segments; i++) {
        float angle = 2.0f * M_PI * i / num_segments;
        Vector3 base_point3d = Vector3(head3d.x + radius * cos(angle), head3d.y + radius * sin(angle), head3d.z);
        base_points2d[i] = project_world_to_screen(base_point3d);
    }

    // Draw lines from the tip to the base
    for (int i = 0; i < num_segments; i++) {
        draw_list->AddLine(ImVec2(top_of_cone2d.x, top_of_cone2d.y), ImVec2(base_points2d[i].x, base_points2d[i].y), color, thickness);
    }

    // Draw the base of the cone
    for (int i = 0; i < num_segments; i++) {
        draw_list->AddLine(ImVec2(base_points2d[i].x, base_points2d[i].y), ImVec2(base_points2d[(i + 1) % num_segments].x, base_points2d[(i + 1) % num_segments].y), color, thickness);
    }
}

void draw_skeleton(uintptr_t mesh, ImColor color) {
    Vector3 bonePositions[] = {
        get_entity_bone(mesh, 110), //HeadBone [0]
        get_entity_bone(mesh, 2),   // Hip [1]
        get_entity_bone(mesh, 66),  // Neck [2]
        get_entity_bone(mesh, 9),   // UpperArmLeft [3]
        get_entity_bone(mesh, 38),  // UpperArmRight [4]
        get_entity_bone(mesh, 10),  // LeftHand [5]
        get_entity_bone(mesh, 39),  // RightHand [6]
        get_entity_bone(mesh, 11),  // LeftHand1 [7]
        get_entity_bone(mesh, 40),  // RightHand1 [8]
        get_entity_bone(mesh, 78),  // RightThigh [9]
        get_entity_bone(mesh, 71),  // LeftThigh [10]
        get_entity_bone(mesh, 79),  // RightCalf [11]
        get_entity_bone(mesh, 72),  // LeftCalf [12]
        get_entity_bone(mesh, 75),  // LeftFoot [13]
        get_entity_bone(mesh, 82)   // RightFoot [14]
    };

    Vector2 bonePositionsOut[15];
    for (int i = 0; i < 15; ++i) {
        bonePositionsOut[i] = project_world_to_screen(bonePositions[i]);
    }

    auto draw_list = ImGui::GetBackgroundDrawList();

    draw_list->AddLine(ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), ImVec2(bonePositionsOut[2].x, bonePositionsOut[2].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[3].x, bonePositionsOut[3].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[4].x, bonePositionsOut[4].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[5].x, bonePositionsOut[5].y), ImVec2(bonePositionsOut[7].x, bonePositionsOut[7].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[6].x, bonePositionsOut[6].y), ImVec2(bonePositionsOut[8].x, bonePositionsOut[8].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), ImVec2(bonePositionsOut[1].x, bonePositionsOut[1].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), ImVec2(bonePositionsOut[10].x, bonePositionsOut[10].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), ImVec2(bonePositionsOut[9].x, bonePositionsOut[9].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[13].x, bonePositionsOut[13].y), ImVec2(bonePositionsOut[12].x, bonePositionsOut[12].y), color, settings::visuals::skeleton_thickness);
    draw_list->AddLine(ImVec2(bonePositionsOut[14].x, bonePositionsOut[14].y), ImVec2(bonePositionsOut[11].x, bonePositionsOut[11].y), color, settings::visuals::skeleton_thickness);
}

void actorLoop()
{
    auto draw_list = ImGui::GetBackgroundDrawList();
    float screenWidth = ImGui::GetIO().DisplaySize.x;
    float screenHeight = ImGui::GetIO().DisplaySize.y;

    // Cache leeren und neu befüllen
    cache::update();
    cache::closest_distance = FLT_MAX;
    cache::closest_mesh = 0;
    cache::closest_pawn = 0;

    auto cached_actors = cache::actors;
    for (const auto& actor : cached_actors)
    {
        uintptr_t mesh = actor.mesh;
        uintptr_t pawn_private = actor.actor; // Using pawn_private as the main actor reference
        float distance = actor.distance;

        chams::apply();

        if (settings::visuals::enable || (settings::aimbot::mouseAim && distance <= settings::aimbot::aimbot_renderDistance))
        {
            Vector3 head3d = get_entity_bone(mesh, 110);
            Vector2 head2d = project_world_to_screen(head3d);
            Vector3 bottom3d = get_entity_bone(mesh, 0);
            Vector2 bottom2d = project_world_to_screen(bottom3d);
            Vector2 boxHead = project_world_to_screen(Vector3(head3d.x, head3d.y, head3d.z + 20.0f));
            float box_height = abs(boxHead.y - bottom2d.y);
            float box_width = box_height * settings::visuals::box_width;
            bool visible = settings::visuals::visible_check ? is_visible(mesh) : true;
            ImColor color = visible ? settings::colors::icBoxColorVisible : settings::colors::icBoxColorInvisible;

            if (distance <= settings::visuals::renderDistance) {

                if (settings::visuals::box)
                {
                    float x = boxHead.x - box_width / 2;
                    float y = boxHead.y;

                    if (settings::visuals::boxType == boxType::normal) {
                        draw_list->AddRect(ImVec2(x, y), ImVec2(x + box_width, y + box_height), color, 0.0f, 0, settings::visuals::box_thickness);
                    }
                    else if (settings::visuals::boxType == boxType::corner) {
                        float corner_size = (box_width / 4 > 5.0f) ? box_width / 4 : 5.0f;
                        draw_list->AddLine(ImVec2(x, y), ImVec2(x + corner_size, y), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x, y), ImVec2(x, y + corner_size), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x + box_width - corner_size, y), ImVec2(x + box_width, y), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x + box_width, y), ImVec2(x + box_width, y + corner_size), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x, y + box_height - corner_size), ImVec2(x, y + box_height), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x, y + box_height), ImVec2(x + corner_size, y + box_height), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x + box_width - corner_size, y + box_height), ImVec2(x + box_width, y + box_height), color, settings::visuals::box_thickness);
                        draw_list->AddLine(ImVec2(x + box_width, y + box_height - corner_size), ImVec2(x + box_width, y + box_height), color, settings::visuals::box_thickness);
                    }
                }

                if (settings::visuals::skeleton)
                {
                    ImColor skeleton_color = visible ? settings::colors::icSkeletonColorVisible : settings::colors::icSkeletonColorInvisible;
                    draw_skeleton(mesh, skeleton_color);
                }

                if (settings::visuals::distance)
                {
                    char dist[64];
                    sprintf_s(dist, "%.fm", distance);
                    ImVec2 textSize = ImGui::CalcTextSize(dist);
                    stroked_text(ImGui::GetFont(), 13.0f, ImVec2(bottom2d.x - textSize.x / 2, bottom2d.y + 10 - textSize.y / 2), ImColor(255, 255, 255), dist);
                }

                // Other ESP features like name, weapon, etc. would go here
            }

            auto distToCross = getCrossDistance(head2d.x, head2d.y, screenWidth / 2, screenHeight / 2);
            if (distToCross <= settings::aimbot::fov && distToCross < cache::closest_distance)
            {
                if (distance <= settings::aimbot::aimbot_renderDistance) {
                    cache::closest_distance = distToCross;
                    cache::closest_mesh = mesh;
                    cache::closest_pawn = pawn_private;
                }
            }
        }

        chams::reset();
    }

    if (settings::exploits::FOVChanger)
    {
        static bool initialized = false;
        static float originalFOV = 0.0f;
        static float originalBaseFOV = 0.0f;

        uintptr_t PlayerCameraManager = read<uintptr_t>(cache::player_controller + PLAYERCAMERAMANAGER);
        if (is_valid(PlayerCameraManager)) {

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
}