#pragma once
#include <d3d9.h>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include "../imgui/imgui.h"
#include "../driver/comm.hpp"
#include "../settings/settings.hpp"
#include "offsets.hpp"
#define M_PI 3.14159265358979323846264338327950288419716939937510

inline int width = GetSystemMetrics(SM_CXSCREEN);
inline int height = GetSystemMetrics(SM_CYSCREEN);
inline int screen_center_x = width / 2;
inline int screen_center_y = height / 2;

// FName Decryption Class
class FName
{
public:
    int32_t ComparisonIndex;

    std::string ToString() const
    {
        return ToString(ComparisonIndex);
    }

    static std::string ToString(int32_t index)
    {
        int32_t DecryptedIndex = DecryptIndex(index);
        uint64_t NamePoolChunk = read<uint64_t>(cr3 + (GNAMES + 8 * (DecryptedIndex >> 16) + 16)) + 2 * (uint16_t)DecryptedIndex;
        uint16_t Pool = read<uint16_t>(NamePoolChunk);

        if (((Pool ^ 0x64C0u)) < 0x40)
        {
            DecryptedIndex = DecryptIndex(read<int32_t>(NamePoolChunk + 2));
            NamePoolChunk = read<uint64_t>(cr3 + (GNAMES + 8 * (DecryptedIndex >> 16) + 16)) + 2 * (uint16_t)DecryptedIndex;
            Pool = read<uint16_t>(NamePoolChunk);
        }

        int32_t Length = ((Pool ^ 0x64C0u) >> 6);

        if (Length <= 0 || Length > 2048) {
            return "";
        }

        std::vector<char> NameBuffer(Length + 1);
        intel_driver::write_physical((PVOID)(NamePoolChunk + 2), NameBuffer.data(), Length);
        DecryptFName(NameBuffer.data(), Length);
        return std::string(NameBuffer.data());
    }

    static int32_t DecryptIndex(int32_t index)
    {
        if (index)
        {
            int32_t DecryptedIndex = -((index - 1) ^ -1436273837); 

            return DecryptedIndex ? DecryptedIndex : -1436273836;
        }

        return 0;
    }

    static void DecryptFName(char* buffer, int32_t length)
    {
        if (length > 0)
        {
            std::vector<uint8_t> EncryptedBuffer(buffer, buffer + length);
            int32_t key = 8656 * length + 22183891;
            for (int32_t i = 0; i < length; ++i)
            {
                buffer[i] = key - EncryptedBuffer[i] - 20;
                key = 8656 * key + 22183891;
            }
        }

        if (length >= 0)
        {
            buffer[length] = '\0';
        }
    }
};

// Current Patch: v39.11
inline uint64_t decryptUWorld(uint64_t encrypted) {
    // Manual implementation of rotl (rotate left) for compatibility
    constexpr int shift = 56;
    constexpr int bits = 64;
    return ((encrypted << shift) | (encrypted >> (bits - shift))) - 603662288ULL;
}

class Vector2 {
public:
    Vector2() : x(0.f), y(0.f) {}
    Vector2(double _x, double _y) : x(_x), y(_y) {}
    ~Vector2() {}
    double x, y;
};

class Vector3 {
public:
    Vector3() : x(0.f), y(0.f), z(0.f) {}
    Vector3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
    ~Vector3() {}
    double x, y, z;
    inline double dot(Vector3 v) { return x * v.x + y * v.y + z * v.z; }
    inline double distance(Vector3 v) { return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0))); }
    Vector3 operator-(Vector3 v) { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator+(Vector3 v) { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator*(double scalar) { return Vector3(x * scalar, y * scalar, z * scalar); }
};

struct FQuat { double x, y, z, w; };
struct FTransform {
    FQuat rotation;
    Vector3 translation;
    uint8_t pad1c[0x8];
    Vector3 scale3d;
    uint8_t pad2c[0x8];

    D3DMATRIX to_matrix_with_scale()
    {
        D3DMATRIX m{};

        const Vector3 Scale
        (
            (scale3d.x == 0.0) ? 1.0 : scale3d.x,
            (scale3d.y == 0.0) ? 1.0 : scale3d.y,
            (scale3d.z == 0.0) ? 1.0 : scale3d.z
        );

        const double x2 = rotation.x + rotation.x;
        const double y2 = rotation.y + rotation.y;
        const double z2 = rotation.z + rotation.z;
        const double xx2 = rotation.x * x2;
        const double yy2 = rotation.y * y2;
        const double zz2 = rotation.z * z2;
        const double yz2 = rotation.y * z2;
        const double wx2 = rotation.w * x2;
        const double xy2 = rotation.x * y2;
        const double wz2 = rotation.w * z2;
        const double xz2 = rotation.x * z2;
        const double wy2 = rotation.w * y2;

        m._41 = translation.x;
        m._42 = translation.y;
        m._43 = translation.z;
        m._11 = (1.0f - (yy2 + zz2)) * Scale.x;
        m._22 = (1.0f - (xx2 + zz2)) * Scale.y;
        m._33 = (1.0f - (xx2 + yy2)) * Scale.z;
        m._32 = (yz2 - wx2) * Scale.z;
        m._23 = (yz2 + wx2) * Scale.y;
        m._21 = (xy2 - wz2) * Scale.y;
        m._12 = (xy2 + wz2) * Scale.x;
        m._31 = (xz2 + wy2) * Scale.z;
        m._13 = (xz2 - wy2) * Scale.x;
        m._14 = 0.0f;
        m._24 = 0.0f;
        m._34 = 0.0f;
        m._44 = 1.0f;

        return m;
    }
};

template<class type>
class TArray {
public:
    TArray() : data(nullptr), count(std::int32_t()), maxx(std::int32_t()) {}
    TArray(type* data, std::int32_t count, std::int32_t maxx) : data(data), count(count), maxx(maxx) {}

    const bool is_valid() const noexcept { return !(this->data == nullptr); }
    const std::int32_t size() const noexcept { return this->count; }
    type& operator[](std::int32_t index) noexcept { return this->data[index]; }
    const type& operator[](std::int32_t index) const noexcept { return this->data[index]; }
    bool is_valid_index(std::int32_t index) const noexcept { return index < this->size(); }

protected:
    type* data;
    std::int32_t count;
    std::int32_t maxx;
};

D3DMATRIX matrix_multiplication(D3DMATRIX pm1, D3DMATRIX pm2) {
    D3DMATRIX pout{};
    pout._11 = pm1._11 * pm2._11 + pm1._12 * pm2._21 + pm1._13 * pm2._31 + pm1._14 * pm2._41;
    pout._12 = pm1._11 * pm2._12 + pm1._12 * pm2._22 + pm1._13 * pm2._32 + pm1._14 * pm2._42;
    pout._13 = pm1._11 * pm2._13 + pm1._12 * pm2._23 + pm1._13 * pm2._33 + pm1._14 * pm2._43;
    pout._14 = pm1._11 * pm2._14 + pm1._12 * pm2._24 + pm1._13 * pm2._34 + pm1._14 * pm2._44;
    pout._21 = pm1._21 * pm2._11 + pm1._22 * pm2._21 + pm1._23 * pm2._31 + pm1._24 * pm2._41;
    pout._22 = pm1._21 * pm2._12 + pm1._22 * pm2._22 + pm1._23 * pm2._32 + pm1._24 * pm2._42;
    pout._23 = pm1._21 * pm2._13 + pm1._22 * pm2._23 + pm1._23 * pm2._33 + pm1._24 * pm2._43;
    pout._24 = pm1._21 * pm2._14 + pm1._22 * pm2._24 + pm1._23 * pm2._34 + pm1._24 * pm2._44;
    pout._31 = pm1._31 * pm2._11 + pm1._32 * pm2._21 + pm1._33 * pm2._31 + pm1._34 * pm2._41;
    pout._32 = pm1._31 * pm2._12 + pm1._32 * pm2._22 + pm1._33 * pm2._32 + pm1._34 * pm2._42;
    pout._33 = pm1._31 * pm2._13 + pm1._32 * pm2._23 + pm1._33 * pm2._33 + pm1._34 * pm2._43;
    pout._34 = pm1._31 * pm2._14 + pm1._32 * pm2._24 + pm1._33 * pm2._34 + pm1._34 * pm2._44;
    pout._41 = pm1._41 * pm2._11 + pm1._42 * pm2._21 + pm1._43 * pm2._31 + pm1._44 * pm2._41;
    pout._42 = pm1._41 * pm2._12 + pm1._42 * pm2._22 + pm1._43 * pm2._32 + pm1._44 * pm2._42;
    pout._43 = pm1._41 * pm2._13 + pm1._42 * pm2._23 + pm1._43 * pm2._33 + pm1._44 * pm2._43;
    pout._44 = pm1._41 * pm2._14 + pm1._42 * pm2._24 + pm1._43 * pm2._34 + pm1._44 * pm2._44;
    return pout;
}

D3DMATRIX to_matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0)) {
    float radpitch = (rot.x * M_PI / 180);
    float radyaw = (rot.y * M_PI / 180);
    float radroll = (rot.z * M_PI / 180);
    float sp = sinf(radpitch);
    float cp = cosf(radpitch);
    float sy = sinf(radyaw);
    float cy = cosf(radyaw);
    float sr = sinf(radroll);
    float cr = cosf(radroll);
    D3DMATRIX matrix{};
    matrix.m[0][0] = cp * cy;
    matrix.m[0][1] = cp * sy;
    matrix.m[0][2] = sp;
    matrix.m[0][3] = 0.f;
    matrix.m[1][0] = sr * sp * cy - cr * sy;
    matrix.m[1][1] = sr * sp * sy + cr * cy;
    matrix.m[1][2] = -sr * cp;
    matrix.m[1][3] = 0.f;
    matrix.m[2][0] = -(cr * sp * cy + sr * sy);
    matrix.m[2][1] = cy * sr - cr * sp * sy;
    matrix.m[2][2] = cr * cp;
    matrix.m[2][3] = 0.f;
    matrix.m[3][0] = origin.x;
    matrix.m[3][1] = origin.y;
    matrix.m[3][2] = origin.z;
    matrix.m[3][3] = 1.f;
    return matrix;
}

struct Camera {
    Vector3 location = {0.0f, 0.0f, 0.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f};
    float fov = 0.0f;
};

struct FNRot {
    double a;
    char pad_0008[24];
    double b;
    char pad_0028[424];
    double c;
};

namespace cache {
    inline uintptr_t uworld;
    inline uintptr_t game_instance;
    inline uintptr_t local_players;
    inline uintptr_t player_controller;
    inline uintptr_t local_pawn;
    inline uintptr_t root_component;
    inline uintptr_t player_state;
    inline Vector3 relative_location;
    inline Vector3 plocaldistance;
    inline uintptr_t closest_pawn;
    inline int my_team_id;
    inline uintptr_t game_state;
    inline uintptr_t player_array;
    inline int player_count;
    inline float closest_distance;
    inline uintptr_t closest_mesh;
    inline uintptr_t closest_aactor;
    inline uintptr_t current_weapon;
    inline uintptr_t persistent_level;
    inline Camera local_camera;
    inline Vector3 rotation;
    inline std::vector<ImVec2> radarPoints;
}

Camera get_view_point() {
    Camera view_point{};
    uintptr_t location_pointer = read<uintptr_t>(cache::uworld + CAMERA_LOCATION);
    uintptr_t rotation_pointer = read<uintptr_t>(cache::uworld + CAMERA_ROTATION);
    FNRot fnrot{};
    fnrot.a = read<double>(rotation_pointer);
    fnrot.b = read<double>(rotation_pointer + 0x20);
    fnrot.c = read<double>(rotation_pointer + 0x1D0);
    view_point.location = read<Vector3>(location_pointer);
    view_point.rotation.x = asin(fnrot.c) * (180.0 / M_PI);
    view_point.rotation.y = ((atan2(fnrot.a * -1, fnrot.b) * (180.0 / M_PI)) * -1) * -1;
    view_point.fov = read<float>(cache::player_controller + CAMERA_FOV) * 90.0f;
    return view_point;
}

Vector2 project_world_to_screen(Vector3 world_location) {
    cache::local_camera = get_view_point();
    cache::rotation = cache::local_camera.rotation;
    D3DMATRIX temp_matrix = to_matrix(cache::local_camera.rotation);
    Vector3 vaxisx = Vector3(temp_matrix.m[0][0], temp_matrix.m[0][1], temp_matrix.m[0][2]);
    Vector3 vaxisy = Vector3(temp_matrix.m[1][0], temp_matrix.m[1][1], temp_matrix.m[1][2]);
    Vector3 vaxisz = Vector3(temp_matrix.m[2][0], temp_matrix.m[2][1], temp_matrix.m[2][2]);
    Vector3 vdelta = world_location - cache::local_camera.location;
    Vector3 vtransformed = Vector3(vdelta.dot(vaxisy), vdelta.dot(vaxisz), vdelta.dot(vaxisx));
    if (vtransformed.z < 1) vtransformed.z = 1;
    return Vector2(screen_center_x + vtransformed.x * ((screen_center_x / tanf(cache::local_camera.fov * M_PI / 360))) / vtransformed.z, screen_center_y - vtransformed.y * ((screen_center_x / tanf(cache::local_camera.fov * M_PI / 360))) / vtransformed.z);
}

Vector3 get_entity_bone(uintptr_t mesh, int bone_id) {
    uintptr_t bone_array = read<uintptr_t>(mesh + BONE_ARRAY);
    if (bone_array == 0) bone_array = read<uintptr_t>(mesh + BONE_ARRAY_CACHE);
    FTransform bone = read<FTransform>(bone_array + (bone_id * 0x60));
    FTransform component_to_world = read<FTransform>(mesh + COMPONENT_TO_WORLD);
    D3DMATRIX matrix = matrix_multiplication(bone.to_matrix_with_scale(), component_to_world.to_matrix_with_scale());
    return Vector3(matrix._41, matrix._42, matrix._43);
}

bool is_dead(uintptr_t pawn_private) {
    return (read<char>(pawn_private + 0x728) & 0x20) != 0;
}

bool is_visible(uintptr_t mesh) {
    auto Seconds = read<double>(cache::uworld + SECONDS);
    auto LastRenderTime = read<float>(mesh + 0x32c);
    return Seconds - LastRenderTime <= 0.06f;
}

std::string GetWeaponName(uintptr_t Player) {
    uint64_t current_weapon = read<uint64_t>(Player + CURRENT_WEAPON);
    if (!current_weapon) return "E1:NoCWeapon";

    uint64_t weapon_data = read<uint64_t>(current_weapon + WEAPON_DATA);
    if (!weapon_data) return "HarvestingTool";

    FName fname;
    fname.ComparisonIndex = read<int32_t>(weapon_data + DISPLAY_NAME);
    
    if (fname.ComparisonIndex == 0) return "E3:NoDNPtr";
    
    std::string weapon_name = fname.ToString();
    
    if (weapon_name.empty()) {
        return "E8:FailedRead";
    }

    std::string weapon_name_str;
    if (weapon_name.rfind("Athena_", 0) == 0) {
        weapon_name_str = weapon_name.substr(7);
    }
    else {
        weapon_name_str = weapon_name;
    }

    if (weapon_name_str.empty()) {
        return "E7:EmptyFinal";
    }

    return weapon_name_str;
}

Vector2 worldToRadar(Vector3 location, Vector3 origin, Vector3 rotation, int width, float scale = 16.f) {
    float x_diff = location.x - origin.x;
    float z_diff = location.z - origin.z;
    float yaw = rotation.y * M_PI / 180.0f;

    float tempX = x_diff * cosf(yaw) + z_diff * sinf(yaw);
    float tempZ = -x_diff * sinf(yaw) + z_diff * cosf(yaw);
    x_diff = tempX;
    z_diff = tempZ;

    x_diff /= scale;
    z_diff /= scale;

    float radarCenter = width * 0.5f;
    float xnew_diff = radarCenter + x_diff;
    float ynew_diff = radarCenter + z_diff;

    xnew_diff = std::clamp(xnew_diff, 4.0f, width - 4.0f);
    ynew_diff = std::clamp(ynew_diff, 4.0f, width - 4.0f);

    return Vector2(static_cast<int>(xnew_diff), static_cast<int>(ynew_diff));
}

void radar_range(float* x, float* y, float range)
{
	if (fabs((*x)) > range || fabs((*y)) > range)
	{
		if ((*y) > (*x))
		{
			if ((*y) > -(*x))
			{
				(*x) = range * (*x) / (*y);
				(*y) = range;
			}
			else
			{
				(*y) = -range * (*y) / (*x);
				(*x) = -range;
			}
		}
		else
		{
			if ((*y) > -(*x))
			{
				(*y) = range * (*y) / (*x);
				(*x) = range;
			}
			else
			{
				(*x) = -range * (*x) / (*y);
				(*y) = -range;
			}
		}
	}
}

void project_world_to_radar(Vector3 vOrigin, int& screenx, int& screeny, ImVec2 pos)
{
	Vector3 vAngle = cache::local_camera.rotation;
	auto fYaw = vAngle.y * M_PI / 180.0f;
	float dx = vOrigin.x - cache::local_camera.location.x;
	float dy = vOrigin.y - cache::local_camera.location.y;
	float fsin_yaw = sinf(fYaw);
	float fminus_cos_yaw = -cosf(fYaw);
	float x = dy * fminus_cos_yaw + dx * fsin_yaw;
	x = -x;
	float y = dx * fminus_cos_yaw - dy * fsin_yaw;
	float range = (float)(settings::radar::range / 5) * 1000.f;

	radar_range(&x, &y, range);

	ImVec2 DrawPos = ImVec2(pos.x, pos.y);
	ImVec2 DrawSize = ImVec2(200.0f, 200.0f);
	float radarRadius = 200.0f / 2;
	float distance = sqrt(pow(screenx - DrawPos.x, 2) + pow(screeny - DrawPos.y, 2));

	if (distance > radarRadius)
	{
		float angle = atan2(screeny - DrawPos.y, screenx - DrawPos.x);
		screenx = static_cast<int>(DrawPos.x + radarRadius * cos(angle));
		screeny = static_cast<int>(DrawPos.y + radarRadius * sin(angle));
	}

	int rad_x = (int)DrawPos.x;
	int rad_y = (int)DrawPos.y;
	float r_siz_x = DrawSize.x;
	float r_siz_y = DrawSize.y;
	int x_max = (int)r_siz_x + rad_x - 5;
	int y_max = (int)r_siz_y + rad_y - 5;

	screenx = rad_x + ((int)r_siz_x / 2 + int(x / range * r_siz_x));
	screeny = rad_y + ((int)r_siz_y / 2 + int(y / range * r_siz_y));

	if (screenx > x_max)
		screenx = x_max;

	if (screenx < rad_x)
		screenx = rad_x;

	if (screeny > y_max)
		screeny = y_max;

	if (screeny < rad_y)
		screeny = rad_y;
}

double getCrossDistance(double x1, double y1, double x2, double y2) {
    return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

float getBulletDrop(float height, float depthPlayerTarget, float velocity, float gravity) {
    float pitch = (atan2(height, depthPlayerTarget));
    float BulletVelocityXY = velocity * cos(pitch);
    float Time = depthPlayerTarget / BulletVelocityXY;
    float TotalVerticalDrop = (0.5f * gravity * Time * Time);
    return TotalVerticalDrop;
}

Vector3 calculateNewRotation(Vector3& zaz, Vector3& daz) {
    Vector3 delta = zaz - daz;
    float hyp = sqrtf(delta.x * delta.x + delta.y * delta.y);
    Vector3 ret;
    ret.x = -asinf(delta.z / hyp) * 57.295779513082f;
    ret.y = atanf(delta.y / delta.x) * 57.295779513082f;
    ret.z = 0.0f;
    if (delta.x >= 0.0f) ret.y += 180.0f;
    return ret;
}

Vector3 getLoc(Vector3 Loc) {
    return Vector3(Loc.x, Loc.y, Loc.z);
}

std::string getRank(int32_t tier)
{
    static const std::string ranks[] = {
        "Bronze 1", "Bronze 2", "Bronze 3",
        "Silver 1", "Silver 2", "Silver 3",
        "Gold 1", "Gold 2", "Gold 3",
        "Platinum 1", "Platinum 2", "Platinum 3",
        "Diamond 1", "Diamond 2", "Diamond 3",
        "Elite", "Champion", "Unreal", "Unranked"
    };

    if (tier >= 0 && tier < 18)
    {
        return ranks[tier];
    }
    else
    {
        return ranks[18];
    }
}

static std::string GetPlayerName(uintptr_t playerState) {
    auto Name = read<uintptr_t>(playerState + PLAYERNAME);
    auto length = read<int>(Name + 0x10);
    auto v6 = (__int64)length;

    if (length <= 0 || length > 255) return std::string("BOT");

    auto FText = (uintptr_t)read<__int64>(Name + 0x8);

    wchar_t* Buffer = new wchar_t[length];
    intel_driver::read_physical(PVOID(static_cast<ULONGLONG>(FText)), Buffer, length * sizeof(wchar_t));

    char v21;
    int v22;
    int i;

    int v25;
    UINT16* v23;

    v21 = v6 - 1;
    if (!(UINT32)v6)
        v21 = 0;
    v22 = 0;
    v23 = (UINT16*)Buffer;
    for (i = (v21) & 3; ; *v23++ += i & 7)
    {
        v25 = v6 - 1;
        if (!(UINT32)v6)
            v25 = 0;
        if (v22 >= v25)
            break;
        i += 3;
        ++v22;
    }

    std::wstring PlayerName{ Buffer };
    delete[] Buffer;
    return std::string(PlayerName.begin(), PlayerName.end());
}