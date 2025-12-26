#pragma once
#include "../imgui/imgui.h"
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include "../json.hpp"
#include <shlobj.h>

struct bone
{
    int bone1;
    int bone2;
    bone(int b1, int b2) : bone1(b1), bone2(b2) {}
};
inline bone boneConnections[] = {
    bone(67, 7), // neck to chest
    bone(7, 2), // chest to pelvis
    bone(7, 9), // chest to right shoulder
    bone(9, 35), // right shoulder to right elbow
    bone(35, 11), // right elbow to right wrist
    bone(7, 38), // chest to left shoulder
    bone(38, 39), // left shoulder to left elbow
    bone(39, 40), // left elbow to left wrist
    bone(2, 71), // pelvis to right hip
    bone(71, 72), // right hip to right knee
    bone(72, 73), // right knee to right shin
    bone(73, 75), // right shin to right ankle
    bone(2, 78), // pelvis to left hip
    bone(78, 79), // left hip to left knee
    bone(79, 80), // left knee to left shin
    bone(80, 82), // left shin to left ankle
};

enum hitboxType : int
{
    head = 110,
    neck = 67,
    chest = 7,
    ass = 69,
};
inline int hitboxValues[] = {
    hitboxType::head,
    hitboxType::neck,
    hitboxType::chest,
    hitboxType::ass,
};
inline const char* getHitboxTypeName(int type)
{
    switch (type)
    {
    case hitboxType::head: return "Head";
    case hitboxType::neck: return "Neck";
    case hitboxType::chest: return "Chest";
    case hitboxType::ass: return "Ass";
    default: return "Unknown";
    }
}

enum boxType : int
{
    normal = 0,
    corner = 1,
    rounded = 2
};

inline const char* getBoxTypeName(int type)
{
    switch (type)
    {
    case boxType::normal:
        return "Normal";
    case boxType::corner:
        return "Corner";
    case boxType::rounded:
        return "Rounded";
    default:
        return "Unknown";
    }
}

inline std::vector<int> boxValues = { boxType::normal, boxType::corner, boxType::rounded };

enum snaplinePosition : int
{
    up = 0,
    middle = 1,
    down = 2,
};

inline const char* getSnaplinePositionName(int type)
{
    switch (type)
    {
    case snaplinePosition::up:
        return "Up";
    case snaplinePosition::middle:
        return "Middle";
    case snaplinePosition::down:
        return "Down";
    default:
        return "Unknown";
    }
}

inline std::vector<int> snaplinePositionValues = { snaplinePosition::up, snaplinePosition::middle, snaplinePosition::down };

namespace settings
{
    // Forward-declare the functions
    inline void save_config(const std::string& config_name = "");
    inline void load_config(const std::string& config_name = "");
    inline void delete_configs();

    inline int width = GetSystemMetrics(SM_CXSCREEN);
    inline int height = GetSystemMetrics(SM_CYSCREEN);
    inline int screen_center_x = width / 2;
    inline int screen_center_y = height / 2;
    inline bool debug = false;
    inline bool show_menu = true;
    inline int tab = 0;
    inline bool watermark_enabled = true;
    inline bool Chests = false;
    inline bool Llamas = false;
    inline bool Pickups = false;
    inline bool Uncommon = false;
    inline bool Common = false;
    inline bool Epic = false;
    inline bool Rare = false;
    inline bool Legendary = false;
    inline bool Mythic = false;
    inline int maxLootDistance = false;
    inline float radar_dist = 50;
    inline float radar_position_x = 20;
    inline float radar_position_y = 50;
    inline float radar_size = 200;
    inline float radar_range = 60.f;

    namespace radar
    {
        inline bool radar = false;
        inline bool cross = true;
        inline bool local_player = true;
        inline float RedDotSize = 3.0f;
        inline float range = 100.0f;
    }

    namespace aimbot
    {
        inline bool enable = true;
        inline bool mouseAim = true;
        inline bool silent_aim = false;
        inline bool FOVArrows = false;
        inline bool Rgb_Fov = false;
        inline float FOVArrowsSize = 25.0f;
        inline ImColor FOVArrowsColor = ImColor(0, 255, 0);
        inline bool Prediction = false;
        inline bool bulletPrediction = false;
        inline float fov = 150.f;
        inline float smoothness = 10.f;
        inline int aimkey;
        inline int hitbox = 0;
        inline bool visible_check = false;
        inline bool crosshair = false;
        inline bool gay_mode_crosshair = false;
        inline float gay_crosshair_size = 20.0f;
        inline float gay_crosshair_speed = 3.0f;
        inline bool show_fov = true;
        inline bool fill_fov = false;
        inline float aimbot_renderDistance = 100.f;
        inline bool ignore_knocked = false;
        inline static const char* aimkeyy[] = { "Left Mouse", "Right Mouse", "Mouse Button 4", "Mouse Button 5", "Shift", "Ctrl", "ALT", "Caps Lock" };
        inline static int current_aimkey = 1; // default to Right Mouse Button
        inline static int current_key;
        inline static int current_hitbox = 0; // Index for head (0 = head, 1 = neck, 2 = chest, 3 = ass)
        inline bool controller_support = true;
        inline bool controller_aim_pressed = false; // Internal state, not saved to config
        // NEU: Aimbot-Typ (0 = Memory, 1 = Kernel)
        inline int aimbot_type = 0;
        inline static const char* aimbot_types[] = { "Memory Aimbot", "Mouse Aimbot" };
    }
    inline const char* getAimbotTypeName(int type) {
        switch(type) {
            case 0: return "Memory Aimbot";
            case 1: return "Mouse Aimbot";
            default: return "Unknown";
        }
    }
    namespace visuals
    {
        inline bool enable = true;
        // inline bool radar = false; // Wird jetzt durch settings::radar::radar ersetzt
        inline bool ignore_teamates = false;
        inline bool box = true;
        inline bool Filled_box = false;
        inline bool Gradient_filled_box = false;
        inline bool Rainbow_esp = false;
        inline bool Outlined_box = false;
        inline bool Outlined_skeleton = false;
        inline int box_thickness = 1;
        inline float box_width = 0.5f;
        inline float box_rounding = 10.0f;
        inline int skeleton_thickness = 1;
        inline int head_circle_thickness = 1;
        inline bool Rounded = false;
        inline bool skeleton = false;
        inline bool line = false;
        inline bool name = false;
        inline bool platform = false;
        inline bool distance = false;
        inline bool rank = false;
        inline bool weapon_name = false;
        inline bool visible_check = true;
        inline float renderDistance = 300.0f;
        inline int boxType = boxType::normal;
        inline bool streamproof = false;
        inline bool spectator_count = false;
        inline bool spectator_list = false;
        inline bool head_circle = false;
        inline float head_circle_radius = 43.0f;
        inline bool head_cone = false;
        inline int snapline_thickness = 1;
        inline bool Outlined_snapline = false;
        inline int snaplineType = snaplinePosition::up;
        inline bool snapline_visible_check = true;
        inline int skeleton_type = 0; // 0 = Skeleton 1, 1 = Skeleton 2, 2 = Skeleton 3 (Smooth)
        inline bool target_line = false; // Target Line Checkbox
    }
    namespace exploits
    {
        inline bool Spinbot = false;
        inline float SpinbotSpeed = 10.f;
        inline bool FOVChanger = false;
        inline float FOVVALUE = 90.f;
        // Exploits
        inline bool Airstuck = false;
        inline bool Carfly = false;
        inline bool SpeedHack = false;
    }
    // Exploits 2: Custom Player Size
    namespace exploits2
    {
        inline bool Playersize_big_tiny = false;
        inline int bigortiny = 0; // 0 = Big, 1 = Tiny
        inline float Playersizebig = 2.0f;  // Beispielwert
        inline float Playersizetiny = 0.2f; // Beispielwert
    }
    namespace colors
    {
        inline ImColor icFovColor = ImColor(90, 140, 255, 255);
        inline ImColor icFovFillColor = ImColor(40, 40, 40, 100);
        inline ImColor icFilledBoxColor = ImColor(0, 0, 0, 120);
        inline ImColor icGradientBoxColorTop = ImColor(180, 100, 255, 120);    // Lila/Purple wie im Bild
        inline ImColor icGradientBoxColorBottom = ImColor(80, 150, 255, 120);  // Blau wie im Bild
        inline ImColor icBoxColorVisible = ImColor(90, 140, 255, 255);
        inline ImColor icBoxColorInvisible = ImColor(90, 140, 255, 255);
        inline ImColor icSkeletonColorVisible = ImColor(90, 140, 255, 255);
        inline ImColor icSkeletonColorInvisible = ImColor(90, 140, 255, 255);
        inline ImColor icTracerColorVisible = ImColor(90, 140, 255, 255);
        inline ImColor icTracerColorInvisible = ImColor(90, 140, 255, 255);
        inline ImColor icHeadCircleColorVisible = ImColor(0, 255, 0);
        inline ImColor icHeadCircleColorInvisible = ImColor(255, 0, 0);
        inline ImColor icCrosshairColor = ImColor(0, 255, 0);
        inline ImColor icHeadConeColor = ImColor(0, 255, 255);
        inline ImColor icDistanceColor = ImColor(255, 255, 255);
        inline ImColor icNameColor = ImColor(0, 255, 255);
        inline ImColor icTargetLineColor = ImColor(60, 60, 60, 220);
        inline ImColor icPlatformColor = ImColor(255, 255, 255);
    }

    inline bool RenderCount;
    inline float rFovSize = 200.0f;
    inline bool rFovCircle;

    namespace triggerbot
    {
        inline bool enable = false;
        inline bool assault_rifles = true;    // AR
        inline bool shotguns = true;          // Shotguns
        inline bool smgs = true;              // SMGs
        inline bool snipers = true;           // Snipers
        inline bool pistols = true;           // Pistols
        inline int delay = 1;
        inline int distance = 20;
        inline int key = VK_F4;
    }

    namespace world
    {
        inline bool enable = false;
        inline bool chests = false;
        inline bool llamas = false;
        inline bool SupplyDrop = false;
        inline bool common_loot = false;
        inline bool uncommon_loot = false;
        inline bool rare_loot = false;
        inline bool epic_loot = false;
        inline bool legendary_loot = false;
        inline bool mythic_loot = false;
        inline float render_distance = 300.0f;
    }

    inline int esp_toggle_key = VK_F7; // Standard-Key für ESP Toggle
    inline bool esp_toggle_enabled = false; // Checkbox für ESP Toggle
    
    // Settings for FrozenFree menu
    inline bool enable_particles = false;
    inline bool vsync = false;
    inline bool show_fps = false;
}

class menu_t
{
public:
    bool ShowMenu = true;
    int menu_key = VK_INSERT;
    int fontsize = 15;
    ImFont* MenuFont = nullptr;
    bool menu_cursor = false;
    int menu_index = 0;
    int tab = 1;
}; inline menu_t menus;

class watermark_t
{
public:
    bool enable = true;
    float watermark_size = 18.0f;
    int watermark_pos_x = 1;
    int watermark_pos_y = 1;
    bool center = false;
}; inline watermark_t watermark;

class globals_t
{
public:
    int ScreenWidth = 0;
    ImVec2 NiggaWidth = {0.0f, 0.0f};
    int ScreenHeight = 0;
    __int64 va_text = 0;
    HWND window_handle = nullptr;
}; inline globals_t globals;

class triggerbot_t
{
public:
    bool triggerbot_enable = false;
    int triggerbot_distance = 20;
    int delay = 1;
    int triggerbot_key = VK_F4;
}; inline triggerbot_t triggerbot;

class aimbot_t
{
public:
    bool aimbot_enable = false;
    bool memory_aim = false;
    bool silent_aim = false;
    bool crosshair_enable = false;
    float crosshair_size = 10.0f;
    float smoothness = 5.0f;
    int aimkey = VK_RBUTTON;
    int freezeplayerkey = VK_F3;
    int current_hitbox = 0;
    int fovsize = 150;
    int ADS_Fov_Size = 150;
    int smoothsize = 9;
    int Hitbox = 0;
}; inline aimbot_t aimbot;

class world_t
{
public:
    bool enable = false;
    bool chests = false;
    bool pickups = false;
    bool uncommon = false;
    bool common = false;
    bool epic = false;
    bool rare = false;
    bool legendary = false;
    bool mythic = false;
    int loot_distace = 50;
    int cache_update_speed = 100;
}; inline world_t world;

// --- Config Functions ---

using json = nlohmann::json;

inline std::string GetConfigFilePath(const std::string& config_name = "") {
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        std::string exe_dir = std::filesystem::path(exe_path).parent_path().string();
        std::string config_dir = exe_dir + "\\configs";
        std::filesystem::create_directories(config_dir);
        
        if (!config_name.empty()) {
            return config_dir + "\\" + config_name + ".json";
        }
        return config_dir + "\\config.json";
    }
    return "config.json";
}

inline void settings::save_config(const std::string& config_name) {
    std::string file_path = GetConfigFilePath(config_name);
    json config;

    config["aimbot"]["enable"] = settings::aimbot::enable;
    config["aimbot"]["mouseAim"] = settings::aimbot::mouseAim;
    config["aimbot"]["FOVArrows"] = settings::aimbot::FOVArrows;
    config["aimbot"]["FOVArrowsSize"] = settings::aimbot::FOVArrowsSize;
    config["aimbot"]["Prediction"] = settings::aimbot::Prediction;
    config["aimbot"]["silentAim"] = settings::aimbot::silent_aim;
    config["aimbot"]["bulletPrediction"] = settings::aimbot::bulletPrediction;
    config["aimbot"]["fov"] = settings::aimbot::fov;
    config["aimbot"]["smoothness"] = settings::aimbot::smoothness;
    config["aimbot"]["aimkey"] = settings::aimbot::aimkey;
    config["aimbot"]["hitbox"] = settings::aimbot::hitbox;
    config["aimbot"]["current_hitbox"] = settings::aimbot::current_hitbox;
    config["aimbot"]["visible_check"] = settings::aimbot::visible_check;
    config["aimbot"]["crosshair"] = settings::aimbot::crosshair;
    config["aimbot"]["show_fov"] = settings::aimbot::show_fov;
    config["aimbot"]["Rgb_Fov"] = settings::aimbot::Rgb_Fov;
    config["aimbot"]["fill_fov"] = settings::aimbot::fill_fov;
    config["aimbot"]["aimbot_renderDistance"] = settings::aimbot::aimbot_renderDistance;
    config["aimbot"]["ignore_knocked"] = settings::aimbot::ignore_knocked;

    config["triggerbot"]["enable"] = settings::triggerbot::enable;
    config["triggerbot"]["distance"] = settings::triggerbot::distance;
    config["triggerbot"]["key"] = settings::triggerbot::key;

    config["world"]["enable"] = settings::world::enable;
    config["world"]["chests"] = settings::world::chests;
    config["world"]["llamas"] = settings::world::llamas;
    config["world"]["SupplyDrop"] = settings::world::SupplyDrop;
    config["world"]["common_loot"] = settings::world::common_loot;
    config["world"]["uncommon_loot"] = settings::world::uncommon_loot;
    config["world"]["rare_loot"] = settings::world::rare_loot;
    config["world"]["epic_loot"] = settings::world::epic_loot;
    config["world"]["legendary_loot"] = settings::world::legendary_loot;
    config["world"]["mythic_loot"] = settings::world::mythic_loot;
    config["world"]["render_distance"] = settings::world::render_distance;

    config["misc"]["watermark_enabled"] = settings::watermark_enabled;
    config["misc"]["esp_toggle_key"] = settings::esp_toggle_key;
    config["misc"]["esp_toggle_enabled"] = settings::esp_toggle_enabled;

    config["visuals"]["enable"] = settings::visuals::enable;
            // config["visuals"]["radar"] = settings::visuals::radar; // Wird jetzt durch settings::radar::radar ersetzt
    config["visuals"]["ignore_teamates"] = settings::visuals::ignore_teamates;
    config["visuals"]["box"] = settings::visuals::box;
    config["visuals"]["Filled_box"] = settings::visuals::Filled_box;
    config["visuals"]["Gradient_filled_box"] = settings::visuals::Gradient_filled_box;
    config["visuals"]["Rainbow_esp"] = settings::visuals::Rainbow_esp;
    config["visuals"]["Outlined_box"] = settings::visuals::Outlined_box;
    config["visuals"]["Outlined_skeleton"] = settings::visuals::Outlined_skeleton;
    config["visuals"]["box_thickness"] = settings::visuals::box_thickness;
    config["visuals"]["box_width"] = settings::visuals::box_width;
    config["visuals"]["box_rounding"] = settings::visuals::box_rounding;
    config["visuals"]["skeleton_thickness"] = settings::visuals::skeleton_thickness;
    config["visuals"]["head_circle_thickness"] = settings::visuals::head_circle_thickness;
    config["visuals"]["Rounded"] = settings::visuals::Rounded;
    config["visuals"]["skeleton"] = settings::visuals::skeleton;
    config["visuals"]["line"] = settings::visuals::line;
    config["visuals"]["name"] = settings::visuals::name;
    config["visuals"]["platform"] = settings::visuals::platform;
    config["visuals"]["distance"] = settings::visuals::distance;
    config["visuals"]["rank"] = settings::visuals::rank;
    config["visuals"]["weapon_name"] = settings::visuals::weapon_name;
    config["visuals"]["visible_check"] = settings::visuals::visible_check;
    config["visuals"]["renderDistance"] = settings::visuals::renderDistance;
    config["visuals"]["boxType"] = settings::visuals::boxType;
    config["visuals"]["streamproof"] = settings::visuals::streamproof;
    config["visuals"]["spectator_count"] = settings::visuals::spectator_count;
    config["visuals"]["spectator_list"] = settings::visuals::spectator_list;
    config["visuals"]["head_circle"] = settings::visuals::head_circle;
    config["visuals"]["head_circle_radius"] = settings::visuals::head_circle_radius;
    config["visuals"]["head_cone"] = settings::visuals::head_cone;
    config["visuals"]["snapline_thickness"] = settings::visuals::snapline_thickness;
    config["visuals"]["Outlined_snapline"] = settings::visuals::Outlined_snapline;
    config["visuals"]["snaplineType"] = settings::visuals::snaplineType;
    config["visuals"]["skeleton_type"] = settings::visuals::skeleton_type;
    config["visuals"]["target_line"] = settings::visuals::target_line; // NEU

    config["exploits"]["Spinbot"] = settings::exploits::Spinbot;
    config["exploits"]["SpinbotSpeed"] = settings::exploits::SpinbotSpeed;
    config["exploits"]["FOVChanger"] = settings::exploits::FOVChanger;
    config["exploits"]["FOVVALUE"] = settings::exploits::FOVVALUE;
    config["exploits"]["Airstuck"] = settings::exploits::Airstuck;
    config["exploits"]["Carfly"] = settings::exploits::Carfly;
    config["exploits"]["SpeedHack"] = settings::exploits::SpeedHack;

    config["colors"]["icFovColor"] = { settings::colors::icFovColor.Value.x, settings::colors::icFovColor.Value.y, settings::colors::icFovColor.Value.z, settings::colors::icFovColor.Value.w };
    config["colors"]["icFovFillColor"] = { settings::colors::icFovFillColor.Value.x, settings::colors::icFovFillColor.Value.y, settings::colors::icFovFillColor.Value.z, settings::colors::icFovFillColor.Value.w };
    config["colors"]["icFilledBoxColor"] = { settings::colors::icFilledBoxColor.Value.x, settings::colors::icFilledBoxColor.Value.y, settings::colors::icFilledBoxColor.Value.z, settings::colors::icFilledBoxColor.Value.w };
    config["colors"]["icGradientBoxColorTop"] = { settings::colors::icGradientBoxColorTop.Value.x, settings::colors::icGradientBoxColorTop.Value.y, settings::colors::icGradientBoxColorTop.Value.z, settings::colors::icGradientBoxColorTop.Value.w };
    config["colors"]["icGradientBoxColorBottom"] = { settings::colors::icGradientBoxColorBottom.Value.x, settings::colors::icGradientBoxColorBottom.Value.y, settings::colors::icGradientBoxColorBottom.Value.z, settings::colors::icGradientBoxColorBottom.Value.w };
    config["colors"]["icBoxColorVisible"] = { settings::colors::icBoxColorVisible.Value.x, settings::colors::icBoxColorVisible.Value.y, settings::colors::icBoxColorVisible.Value.z, settings::colors::icBoxColorVisible.Value.w };
    config["colors"]["icBoxColorInvisible"] = { settings::colors::icBoxColorInvisible.Value.x, settings::colors::icBoxColorInvisible.Value.y, settings::colors::icBoxColorInvisible.Value.z, settings::colors::icBoxColorInvisible.Value.w };
    config["colors"]["icSkeletonColorVisible"] = { settings::colors::icSkeletonColorVisible.Value.x, settings::colors::icSkeletonColorVisible.Value.y, settings::colors::icSkeletonColorVisible.Value.z, settings::colors::icSkeletonColorVisible.Value.w };
    config["colors"]["icSkeletonColorInvisible"] = { settings::colors::icSkeletonColorInvisible.Value.x, settings::colors::icSkeletonColorInvisible.Value.y, settings::colors::icSkeletonColorInvisible.Value.z, settings::colors::icSkeletonColorInvisible.Value.w };
    config["colors"]["icTracerColorVisible"] = { settings::colors::icTracerColorVisible.Value.x, settings::colors::icTracerColorVisible.Value.y, settings::colors::icTracerColorVisible.Value.z, settings::colors::icTracerColorVisible.Value.w };
    config["colors"]["icTracerColorInvisible"] = { settings::colors::icTracerColorInvisible.Value.x, settings::colors::icTracerColorInvisible.Value.y, settings::colors::icTracerColorInvisible.Value.z, settings::colors::icTracerColorInvisible.Value.w };
    config["colors"]["icHeadCircleColorVisible"] = { settings::colors::icHeadCircleColorVisible.Value.x, settings::colors::icHeadCircleColorVisible.Value.y, settings::colors::icHeadCircleColorVisible.Value.z, settings::colors::icHeadCircleColorVisible.Value.w };
    config["colors"]["icHeadCircleColorInvisible"] = { settings::colors::icHeadCircleColorInvisible.Value.x, settings::colors::icHeadCircleColorInvisible.Value.y, settings::colors::icHeadCircleColorInvisible.Value.z, settings::colors::icHeadCircleColorInvisible.Value.w };
    config["colors"]["icCrosshairColor"] = { settings::colors::icCrosshairColor.Value.x, settings::colors::icCrosshairColor.Value.y, settings::colors::icCrosshairColor.Value.z, settings::colors::icCrosshairColor.Value.w };
    config["colors"]["icHeadConeColor"] = { settings::colors::icHeadConeColor.Value.x, settings::colors::icHeadConeColor.Value.y, settings::colors::icHeadConeColor.Value.z, settings::colors::icHeadConeColor.Value.w };
    config["colors"]["icDistanceColor"] = { settings::colors::icDistanceColor.Value.x, settings::colors::icDistanceColor.Value.y, settings::colors::icDistanceColor.Value.z, settings::colors::icDistanceColor.Value.w };
    config["colors"]["icNameColor"] = { settings::colors::icNameColor.Value.x, settings::colors::icNameColor.Value.y, settings::colors::icNameColor.Value.z, settings::colors::icNameColor.Value.w };

    config["radar"]["radar"] = settings::radar::radar;
    config["radar"]["cross"] = settings::radar::cross;
    config["radar"]["local_player"] = settings::radar::local_player;
    config["radar"]["RedDotSize"] = settings::radar::RedDotSize;
    config["radar"]["range"] = settings::radar::range;
    
    // Radar-Positionen speichern
    config["radar"]["position_x"] = settings::radar_position_x;
    config["radar"]["position_y"] = settings::radar_position_y;

    std::ofstream file(file_path);
    file << config.dump(4);
    file.close();
}

inline void settings::load_config(const std::string& config_name) {
    std::string file_path = GetConfigFilePath(config_name);
    std::ifstream file(file_path);
    if (!file.is_open()) return;

    json config;
    try {
        file >> config;
    }
    catch (json::parse_error& e) {
        file.close();
        return;
    }
    file.close();

    if (config.contains("aimbot")) {
        if (config["aimbot"].contains("enable")) settings::aimbot::enable = config["aimbot"]["enable"];
        if (config["aimbot"].contains("mouseAim")) settings::aimbot::mouseAim = config["aimbot"]["mouseAim"];
        if (config["aimbot"].contains("FOVArrows")) settings::aimbot::FOVArrows = config["aimbot"]["FOVArrows"];
        if (config["aimbot"].contains("FOVArrowsSize")) settings::aimbot::FOVArrowsSize = config["aimbot"]["FOVArrowsSize"];
        if (config["aimbot"].contains("Prediction")) settings::aimbot::Prediction = config["aimbot"]["Prediction"];
        if (config["aimbot"].contains("silentAim")) settings::aimbot::silent_aim = config["aimbot"]["silentAim"];
        if (config["aimbot"].contains("bulletPrediction")) settings::aimbot::bulletPrediction = config["aimbot"]["bulletPrediction"];
        if (config["aimbot"].contains("fov")) settings::aimbot::fov = config["aimbot"]["fov"];
        if (config["aimbot"].contains("smoothness")) settings::aimbot::smoothness = config["aimbot"]["smoothness"];
        if (config["aimbot"].contains("aimkey")) settings::aimbot::aimkey = config["aimbot"]["aimkey"];
        if (config["aimbot"].contains("hitbox")) settings::aimbot::hitbox = config["aimbot"]["hitbox"];
        if (config["aimbot"].contains("current_hitbox")) settings::aimbot::current_hitbox = config["aimbot"]["current_hitbox"];
        if (config["aimbot"].contains("visible_check")) settings::aimbot::visible_check = config["aimbot"]["visible_check"];
        if (config["aimbot"].contains("crosshair")) settings::aimbot::crosshair = config["aimbot"]["crosshair"];
        if (config["aimbot"].contains("show_fov")) settings::aimbot::show_fov = config["aimbot"]["show_fov"];
        if (config["aimbot"].contains("Rgb_Fov")) settings::aimbot::Rgb_Fov = config["aimbot"]["Rgb_Fov"];
        if (config["aimbot"].contains("fill_fov")) settings::aimbot::fill_fov = config["aimbot"]["fill_fov"];
        if (config["aimbot"].contains("aimbot_renderDistance")) settings::aimbot::aimbot_renderDistance = config["aimbot"]["aimbot_renderDistance"];
        if (config["aimbot"].contains("ignore_knocked")) settings::aimbot::ignore_knocked = config["aimbot"]["ignore_knocked"];
    }

    if (config.contains("triggerbot")) {
        if (config["triggerbot"].contains("enable")) settings::triggerbot::enable = config["triggerbot"]["enable"];
        if (config["triggerbot"].contains("distance")) settings::triggerbot::distance = config["triggerbot"]["distance"];
        if (config["triggerbot"].contains("key")) settings::triggerbot::key = config["triggerbot"]["key"];
    }

    if (config.contains("world")) {
        if (config["world"].contains("enable")) settings::world::enable = config["world"]["enable"];
        if (config["world"].contains("chests")) settings::world::chests = config["world"]["chests"];
        if (config["world"].contains("llamas")) settings::world::llamas = config["world"]["llamas"];
        if (config["world"].contains("SupplyDrop")) settings::world::SupplyDrop = config["world"]["SupplyDrop"];
        if (config["world"].contains("common_loot")) settings::world::common_loot = config["world"]["common_loot"];
        if (config["world"].contains("uncommon_loot")) settings::world::uncommon_loot = config["world"]["uncommon_loot"];
        if (config["world"].contains("rare_loot")) settings::world::rare_loot = config["world"]["rare_loot"];
        if (config["world"].contains("epic_loot")) settings::world::epic_loot = config["world"]["epic_loot"];
        if (config["world"].contains("legendary_loot")) settings::world::legendary_loot = config["world"]["legendary_loot"];
        if (config["world"].contains("mythic_loot")) settings::world::mythic_loot = config["world"]["mythic_loot"];
        if (config["world"].contains("render_distance")) settings::world::render_distance = config["world"]["render_distance"];
    }

    if (config.contains("radar")) {
        if (config["radar"].contains("radar")) settings::radar::radar = config["radar"]["radar"];
        if (config["radar"].contains("cross")) settings::radar::cross = config["radar"]["cross"];
        if (config["radar"].contains("local_player")) settings::radar::local_player = config["radar"]["local_player"];
        if (config["radar"].contains("RedDotSize")) settings::radar::RedDotSize = config["radar"]["RedDotSize"];
        if (config["radar"].contains("range")) settings::radar::range = config["radar"]["range"];
        
        // Radar-Positionen laden
        if (config["radar"].contains("position_x")) settings::radar_position_x = config["radar"]["position_x"];
        if (config["radar"].contains("position_y")) settings::radar_position_y = config["radar"]["position_y"];
    }

    if (config.contains("misc")) {
        if (config["misc"].contains("watermark_enabled")) settings::watermark_enabled = config["misc"]["watermark_enabled"];
        if (config["misc"].contains("esp_toggle_key")) settings::esp_toggle_key = config["misc"]["esp_toggle_key"];
        if (config["misc"].contains("esp_toggle_enabled")) settings::esp_toggle_enabled = config["misc"]["esp_toggle_enabled"];
    }

    if (config.contains("visuals")) {
        if (config["visuals"].contains("enable")) settings::visuals::enable = config["visuals"]["enable"];
        // if (config["visuals"].contains("radar")) settings::visuals::radar = config["visuals"]["radar"]; // Wird jetzt durch settings::radar::radar ersetzt
        if (config["visuals"].contains("ignore_teamates")) settings::visuals::ignore_teamates = config["visuals"]["ignore_teamates"];
        if (config["visuals"].contains("box")) settings::visuals::box = config["visuals"]["box"];
        if (config["visuals"].contains("Filled_box")) settings::visuals::Filled_box = config["visuals"]["Filled_box"];
        if (config["visuals"].contains("Gradient_filled_box")) settings::visuals::Gradient_filled_box = config["visuals"]["Gradient_filled_box"];
        if (config["visuals"].contains("Rainbow_esp")) settings::visuals::Rainbow_esp = config["visuals"]["Rainbow_esp"];
        if (config["visuals"].contains("Outlined_box")) settings::visuals::Outlined_box = config["visuals"]["Outlined_box"];
        if (config["visuals"].contains("Outlined_skeleton")) settings::visuals::Outlined_skeleton = config["visuals"]["Outlined_skeleton"];
        if (config["visuals"].contains("box_thickness")) settings::visuals::box_thickness = config["visuals"]["box_thickness"];
        if (config["visuals"].contains("box_width")) settings::visuals::box_width = config["visuals"]["box_width"];
        if (config["visuals"].contains("box_rounding")) settings::visuals::box_rounding = config["visuals"]["box_rounding"];
        if (config["visuals"].contains("skeleton_thickness")) settings::visuals::skeleton_thickness = config["visuals"]["skeleton_thickness"];
        if (config["visuals"].contains("head_circle_thickness")) settings::visuals::head_circle_thickness = config["visuals"]["head_circle_thickness"];
        if (config["visuals"].contains("Rounded")) settings::visuals::Rounded = config["visuals"]["Rounded"];
        if (config["visuals"].contains("skeleton")) settings::visuals::skeleton = config["visuals"]["skeleton"];
        if (config["visuals"].contains("line")) settings::visuals::line = config["visuals"]["line"];
        if (config["visuals"].contains("name")) settings::visuals::name = config["visuals"]["name"];
        if (config["visuals"].contains("platform")) settings::visuals::platform = config["visuals"]["platform"];
        if (config["visuals"].contains("distance")) settings::visuals::distance = config["visuals"]["distance"];
        if (config["visuals"].contains("rank")) settings::visuals::rank = config["visuals"]["rank"];
        if (config["visuals"].contains("weapon_name")) settings::visuals::weapon_name = config["visuals"]["weapon_name"];
        if (config["visuals"].contains("visible_check")) settings::visuals::visible_check = config["visuals"]["visible_check"];
        if (config["visuals"].contains("renderDistance")) settings::visuals::renderDistance = config["visuals"]["renderDistance"];
        if (config["visuals"].contains("boxType")) settings::visuals::boxType = config["visuals"]["boxType"];
        if (config["visuals"].contains("streamproof")) settings::visuals::streamproof = config["visuals"]["streamproof"];
        if (config["visuals"].contains("spectator_count")) settings::visuals::spectator_count = config["visuals"]["spectator_count"];
        if (config["visuals"].contains("spectator_list")) settings::visuals::spectator_list = config["visuals"]["spectator_list"];
        if (config["visuals"].contains("head_circle")) settings::visuals::head_circle = config["visuals"]["head_circle"];
        if (config["visuals"].contains("head_circle_radius")) settings::visuals::head_circle_radius = config["visuals"]["head_circle_radius"];
        if (config["visuals"].contains("head_cone")) settings::visuals::head_cone = config["visuals"]["head_cone"];
        if (config["visuals"].contains("snapline_thickness")) settings::visuals::snapline_thickness = config["visuals"]["snapline_thickness"];
        if (config["visuals"].contains("Outlined_snapline")) settings::visuals::Outlined_snapline = config["visuals"]["Outlined_snapline"];
        if (config["visuals"].contains("snaplineType")) settings::visuals::snaplineType = config["visuals"]["snaplineType"];
        if (config["visuals"].contains("skeleton_type")) settings::visuals::skeleton_type = config["visuals"]["skeleton_type"];
        if (config["visuals"].contains("target_line")) settings::visuals::target_line = config["visuals"]["target_line"];
    }
    settings::visuals::boxType = std::clamp(settings::visuals::boxType, 0, 2);

    if (config.contains("exploits")) {
        if (config["exploits"].contains("Spinbot")) settings::exploits::Spinbot = config["exploits"]["Spinbot"];
        if (config["exploits"].contains("SpinbotSpeed")) settings::exploits::SpinbotSpeed = config["exploits"]["SpinbotSpeed"];
        if (config["exploits"].contains("FOVChanger")) settings::exploits::FOVChanger = config["exploits"]["FOVChanger"];
        if (config["exploits"].contains("FOVVALUE")) settings::exploits::FOVVALUE = config["exploits"]["FOVVALUE"];
        if (config["exploits"].contains("Airstuck")) settings::exploits::Airstuck = config["exploits"]["Airstuck"];
        if (config["exploits"].contains("Carfly")) settings::exploits::Carfly = config["exploits"]["Carfly"];
        if (config["exploits"].contains("SpeedHack")) settings::exploits::SpeedHack = config["exploits"]["SpeedHack"];
    }

    if (config.contains("colors")) {
        if (config["colors"].contains("icFovColor")) {
            auto c = config["colors"]["icFovColor"];
            settings::colors::icFovColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icFovFillColor")) {
            auto c = config["colors"]["icFovFillColor"];
            settings::colors::icFovFillColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icFilledBoxColor")) {
            auto c = config["colors"]["icFilledBoxColor"];
            settings::colors::icFilledBoxColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icGradientBoxColorTop")) {
            auto c = config["colors"]["icGradientBoxColorTop"];
            settings::colors::icGradientBoxColorTop = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icGradientBoxColorBottom")) {
            auto c = config["colors"]["icGradientBoxColorBottom"];
            settings::colors::icGradientBoxColorBottom = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icBoxColorVisible")) {
            auto c = config["colors"]["icBoxColorVisible"];
            settings::colors::icBoxColorVisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icBoxColorInvisible")) {
            auto c = config["colors"]["icBoxColorInvisible"];
            settings::colors::icBoxColorInvisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icSkeletonColorVisible")) {
            auto c = config["colors"]["icSkeletonColorVisible"];
            settings::colors::icSkeletonColorVisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icSkeletonColorInvisible")) {
            auto c = config["colors"]["icSkeletonColorInvisible"];
            settings::colors::icSkeletonColorInvisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icTracerColorVisible")) {
            auto c = config["colors"]["icTracerColorVisible"];
            settings::colors::icTracerColorVisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icTracerColorInvisible")) {
            auto c = config["colors"]["icTracerColorInvisible"];
            settings::colors::icTracerColorInvisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icHeadCircleColorVisible")) {
            auto c = config["colors"]["icHeadCircleColorVisible"];
            settings::colors::icHeadCircleColorVisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icHeadCircleColorInvisible")) {
            auto c = config["colors"]["icHeadCircleColorInvisible"];
            settings::colors::icHeadCircleColorInvisible = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icCrosshairColor")) {
            auto c = config["colors"]["icCrosshairColor"];
            settings::colors::icCrosshairColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icHeadConeColor")) {
            auto c = config["colors"]["icHeadConeColor"];
            settings::colors::icHeadConeColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icDistanceColor")) {
            auto c = config["colors"]["icDistanceColor"];
            settings::colors::icDistanceColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
        if (config["colors"].contains("icNameColor")) {
            auto c = config["colors"]["icNameColor"];
            settings::colors::icNameColor = ImColor(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
        }
    }
}

inline void settings::delete_configs() {
    try {
        // Get the config directory path
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            std::string config_dir = std::string(path) + "\\FrozenSlotted";

            // Delete config.json if it exists
            std::string config_file = config_dir + "\\config.json";
            if (std::filesystem::exists(config_file)) {
                std::filesystem::remove(config_file);
            }

            // Delete the entire config directory if it's empty
            if (std::filesystem::exists(config_dir) && std::filesystem::is_empty(config_dir)) {
                std::filesystem::remove(config_dir);
            }
        }
    }
    catch (...) {
        // Ignore any errors during deletion
    }
}