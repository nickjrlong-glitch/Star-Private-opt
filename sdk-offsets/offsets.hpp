// ===== main =====
#define UWORLD 0x175C9368          // updated
#define GAME_INSTANCE 0x250        // updated
#define GAME_STATE 0x1D8           // updated
#define LEVELS 0x1F0               // updated
#define ACTORS 0xA0
#define GNAMES 0x17541600          // updated

// ===== Player System =====
#define LOCAL_PLAYERS 0x38
#define PLAYER_CONTROLLER 0x30
#define PLAYERCAMERAMANAGER 0x368
#define LOCAL_PAWN 0x358
#define ACKNOWLEDGED_PAWN 0x358
#define PAWN_PRIVATE 0x328
#define PLAYER_STATE 0x2D0
#define TEAM_INDEX 0x11A9
#define PLAYER_ARRAY 0x2C8

// ===== Player Info =====
#define PLAYERNAME 0xA00
#define DISPLAY_NAME 0x40
#define PLATFORM 0x440
#define RANKED_PROGRESS 0xD8
#define TIER 0xA2

// ===== Camera & View =====
#define VIEW_TARGET 0x1840         // updated
#define CAMERA_LOCATION 0x178      // updated
#define CAMERA_ROTATION 0x188      // updated
#define CAMERA_FOV 0x3B4
#define SECONDS 0x198              // updated
#define DEFAULTFOV 0x2C4
#define BASEFOV 0x380

// ===== Mesh & Bones =====
#define MESH 0x330
#define COMPONENT_TO_WORLD 0x1E0
#define BONE_ARRAY 0x5F0
#define BONE_ARRAY_CACHE 0x5F8     // updated
#define LAST_SUBMIT_TIME 0x1A0
#define LAST_SUBMIT_TIME_ON_SCREEN 0x328

// ===== Actor Components =====
#define ROOT_COMPONENT 0x1B0
#define RELATIVE_LOCATION 0x140
#define HABANERO_COMPONENT 0x940
#define CHARACTER_MOVEMENT 0x3A8
#define CUSTOM_TIME_DILATION 0x68
#define LAST_UPDATE_LOCATION 0x3C0
#define VELOCITY_OFFSET 0x188

// ===== Weapon / World ESP =====
#define CURRENT_WEAPON 0x990
#define WEAPON_DATA 0x2BA          // updated
#define AMMO_COUNT 0x150C
#define PROJECTILE_SPEED 0x24F4    // updated
#define PROJECTILE_GRAVITY 0x24F8  // updated
#define NAME_PRIVATE 0x8
#define PRIMARY_PICKUP_ITEM_ENTRY 0x3A8
#define RARITY 0xAA
#define ITEMNAME 0x40
#define bAlreadySearched 0xD52

// ===== FName Decryption =====
#define COMPARISON_INDEX 0x8

// ===== Aimbot / Networking =====
#define NetConnection 0x528

// ===== Bone IDs (for Aimbot) =====
#define FN_PELVIS 6
#define FN_NECK 67
#define FN_HEAD 110