#include "practice.h"

#define AUTOTEST_MAGIC 0x4F545541u /* "AUTO" little endian */

enum {
    AUTO_WAIT_GAME,
    AUTO_WAIT_OPEN,
    AUTO_ROOT_SETTLE,
    AUTO_WAIT_PRACTICE,
    AUTO_PRACTICE_SETTLE,
    AUTO_WAIT_BACK,
    AUTO_BACK_SETTLE,
    AUTO_WAIT_CLOSE,
    AUTO_FEATURE_TESTS,
    AUTO_REOPEN_SETTLE,
    AUTO_WAIT_REOPEN,
    AUTO_RECLOSE_SETTLE,
    AUTO_WAIT_RECLOSE,
    AUTO_DONE,
};

typedef struct {
    u32 magic;
    u32 status;
    u32 totalFrames;
    u32 taskFrames;
    u32 readyFrames;
    u32 waitFrames;
    u32 rootTileChecksum;
    u32 practiceTileChecksum;
    u32 backTileChecksum;
    u32 layoutStatus;
    u32 featureStatus;
    u32 featureFailures;
    u32 movementStatus;
    u8 phase;
    u8 lastTask;
    u8 lastPage;
    u8 lastMenuOpen;
} AutoTestState;

AutoTestState gPracticeAutoTest;
u8 gPracticeAutoSaveReadback[0x500];

static void clear_input(void) {
    TMC_INPUT.held = 0;
    TMC_INPUT.pressed = 0;
    TMC_INPUT.repeat = 0;
}

static void set_input(u16 keys) {
    TMC_INPUT.held = keys;
    TMC_INPUT.pressed = keys;
    TMC_INPUT.repeat = keys;
}

static u32 tilemap_checksum(void) {
    volatile const u16* map = (volatile const u16*)ADDR_G_BG0_BUFFER;
    u32 hash = 2166136261u;
    u32 index;
    for (index = 0; index < 0x400; index++) {
        hash ^= map[index];
        hash *= 16777619u;
    }
    return hash;
}

static u32 tilemap_has_text(u32 x, u32 y, const char* text) {
    volatile const u16* map = (volatile const u16*)ADDR_G_BG0_BUFFER;
    while (*text != '\0') {
        u8 character = (u8)*text++;
        if (character >= 'a' && character <= 'z') character -= 32;
        if ((map[y * 32 + x] & 0x3FF) != (u16)(character - 32)) return 0;
        x++;
    }
    return 1;
}

static u32 bytes_equal(const void* first, const void* second, u32 size) {
    const u8* a = (const u8*)first;
    const u8* b = (const u8*)second;
    while (size-- != 0) {
        if (*a++ != *b++) return 0;
    }
    return 1;
}

static u32 flags_checksum(void) {
    u32 hash = 2166136261u;
    u32 index;
    for (index = 0; index < sizeof(TMC_SAVE.flags); index++) {
        hash ^= TMC_SAVE.flags[index];
        hash *= 16777619u;
    }
    return hash;
}

static void feature_result(u32 bit, u32 passed) {
    if (passed) gPracticeAutoTest.featureStatus |= bit;
    else gPracticeAutoTest.featureFailures |= bit;
}

static void test_player_features(void) {
    TmcStats oldStats;
    u8 oldDisplayedHealth = PLAYER8(0x45);
    u32 passed = 1;
    Practice_CopyBytes((const void*)&TMC_SAVE.stats, &oldStats, sizeof(oldStats));
    TMC_SAVE.stats.maxHealth = 64;
    TMC_SAVE.stats.health = 8;
    Player_FullHealth();
    if (TMC_SAVE.stats.health != 64 || PLAYER8(0x45) != 64) passed = 0;
    TMC_SAVE.stats.maxHealth = 24;
    Player_AdjustMaxHealth(-1);
    if (TMC_SAVE.stats.maxHealth != 24) passed = 0;
    TMC_SAVE.stats.maxHealth = 160;
    Player_AdjustMaxHealth(1);
    if (TMC_SAVE.stats.maxHealth != 160) passed = 0;
    TMC_SAVE.stats.walletType = 0;
    TMC_SAVE.stats.rupees = 90;
    Player_AdjustRupees(100);
    if (TMC_SAVE.stats.rupees != 100) passed = 0;
    Practice_CopyBytes(&oldStats, (void*)&TMC_SAVE.stats, sizeof(oldStats));
    PLAYER8(0x45) = oldDisplayedHealth;
    feature_result(1u, passed);
}

static void test_inventory_features(void) {
    TmcStats oldStats;
    u8 oldInventory[34];
    u16 oldSkills = PSTATE16(0xAC);
    u32 oldFlags = flags_checksum();
    u8 oldProgress = TMC_SAVE.flags[0];
    u32 passed = 1;
    u32 item;
    Practice_CopyBytes((const void*)&TMC_SAVE.stats, &oldStats, sizeof(oldStats));
    Practice_CopyBytes((const void*)TMC_SAVE.inventory, oldInventory, sizeof(oldInventory));
    Inventory_SetWeaponGroup(1, 6, 6);
    for (item = 1; item < 6; item++) if (Inventory_Get(item) != 0) passed = 0;
    if (Inventory_Get(6) != 1 || Inventory_GroupSelection(1, 6) != 6) passed = 0;
    Inventory_GiveAll();
    if (Inventory_Get(6) != 1 || Inventory_Get(14) != 1 || Inventory_Get(20) != 1 ||
        TMC_SAVE.stats.walletType != 3 || TMC_SAVE.stats.bombCount != 99 ||
        TMC_SAVE.stats.arrowCount != 99 || (TMC_SAVE.flags[0] & 0x6C) != 0x6C) passed = 0;
    TMC_SAVE.flags[0] = oldProgress;
    if (flags_checksum() != oldFlags) passed = 0;
    Inventory_Set(64, 1);
    Inventory_Set(65, 0);
    Inventory_Set(66, 1);
    Inventory_Set(67, 0);
    if (Inventory_Get(64) != 1 || Inventory_Get(65) != 0 ||
        Inventory_Get(66) != 1 || Inventory_Get(67) != 0 ||
        (TMC_SAVE.flags[0] & 0x6C) != 0x24) passed = 0;
    TMC_SAVE.flags[0] = oldProgress;
    if (flags_checksum() != oldFlags) passed = 0;
    Practice_CopyBytes(&oldStats, (void*)&TMC_SAVE.stats, sizeof(oldStats));
    Practice_CopyBytes(oldInventory, (void*)TMC_SAVE.inventory, sizeof(oldInventory));
    PSTATE16(0xAC) = oldSkills;
    feature_result(2u, passed);
}

static void test_flag_features(void) {
    volatile u8* roomFlags = (volatile u8*)(ADDR_G_ROOM_VARS + 0x14);
    u8 oldGlobal = TMC_SAVE.flags[1];
    u8 oldLocal = TMC_SAVE.flags[4];
    u8 oldRoom = roomFlags[1];
    u8 oldClear = TMC_SAVE.flags[0x55 >> 3];
    u8 dungeon = gPracticeState.menuDungeonIndex & 0xF;
    u8 oldKey = TMC_SAVE.dungeonKeys[dungeon];
    u8 oldItem = TMC_SAVE.dungeonItems[dungeon];
    u8 oldWarp = TMC_SAVE.dungeonWarps[dungeon];
    u8 oldBank = gPracticeState.flagBank;
    u16 oldIndex = gPracticeState.flagIndex;
    u16 oldLocalOffset = gPracticeState.menuLocalFlagOffset;
    u8 oldOperation = gPracticeState.flagOperation;
    u32 passed = 1;

    gPracticeState.flagBank = 0;
    gPracticeState.flagIndex = 10;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.flags[1] != (u8)(oldGlobal | 4) ||
        gPracticeState.flagOperation != 1) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.flags[1] != (u8)(oldGlobal & (u8)~4) ||
        gPracticeState.flagOperation != 2) passed = 0;

    gPracticeState.flagBank = 1;
    gPracticeState.menuLocalFlagOffset = 0x20;
    gPracticeState.flagIndex = 3;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.flags[4] != (u8)(oldLocal | 8)) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.flags[4] != (u8)(oldLocal & (u8)~8)) passed = 0;

    gPracticeState.flagBank = 2;
    gPracticeState.flagIndex = 9;
    Flags_Write(1);
    if (Flags_Read() != 1 || roomFlags[1] != (u8)(oldRoom | 2)) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || roomFlags[1] != (u8)(oldRoom & (u8)~2)) passed = 0;

    gPracticeState.flagIndex = 2;
    gPracticeState.flagBank = 3;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.dungeonKeys[dungeon] != (u8)(oldKey | 4)) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.dungeonKeys[dungeon] != (u8)(oldKey & (u8)~4)) passed = 0;
    gPracticeState.flagBank = 4;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.dungeonItems[dungeon] != (u8)(oldItem | 4)) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.dungeonItems[dungeon] != (u8)(oldItem & (u8)~4)) passed = 0;
    gPracticeState.flagBank = 5;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.dungeonWarps[dungeon] != (u8)(oldWarp | 4)) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.dungeonWarps[dungeon] != (u8)(oldWarp & (u8)~4)) passed = 0;

    gPracticeState.flagBank = 6;
    gPracticeState.flagIndex = 0;
    Flags_Write(1);
    if (Flags_Read() != 1 || TMC_SAVE.flags[0x55 >> 3] != (u8)(oldClear | (1u << (0x55 & 7)))) passed = 0;
    Flags_Write(0);
    if (Flags_Read() != 0 || TMC_SAVE.flags[0x55 >> 3] != (u8)(oldClear & (u8)~(1u << (0x55 & 7)))) passed = 0;

    TMC_SAVE.flags[1] = oldGlobal;
    TMC_SAVE.flags[4] = oldLocal;
    roomFlags[1] = oldRoom;
    TMC_SAVE.flags[0x55 >> 3] = oldClear;
    TMC_SAVE.dungeonKeys[dungeon] = oldKey;
    TMC_SAVE.dungeonItems[dungeon] = oldItem;
    TMC_SAVE.dungeonWarps[dungeon] = oldWarp;
    gPracticeState.flagBank = oldBank;
    gPracticeState.flagIndex = oldIndex;
    gPracticeState.menuLocalFlagOffset = oldLocalOffset;
    gPracticeState.flagOperation = oldOperation;
    feature_result(4u, passed);
}

static void test_warp_features(void) {
    TmcRoomTransition oldTransition;
    u8 oldArea = gPracticeState.selectedArea;
    u8 oldRoom = gPracticeState.selectedRoom;
    u8 oldPending = gPracticeState.pendingAction;
    u32 passed = Warp_IsValid(TMC_ROOM.area, TMC_ROOM.room) &&
                 !Warp_IsValid(0x90, 0) && !Warp_IsValid(TMC_ROOM.area, 0x40);
    Practice_CopyBytes((const void*)&TMC_TRANSITION, &oldTransition, sizeof(oldTransition));
    gPracticeState.selectedArea = TMC_ROOM.area;
    gPracticeState.selectedRoom = TMC_ROOM.room;
    gPracticeState.pendingAction = PENDING_WARP;
    PracticeRuntime_ApplyPending();
    if (TMC_TRANSITION.transitioningOut != 1 || TMC_TRANSITION.type != 5 ||
        TMC_TRANSITION.playerStatus.areaNext != TMC_ROOM.area ||
        TMC_TRANSITION.playerStatus.roomNext != TMC_ROOM.room ||
        TMC_TRANSITION.playerStatus.layer == 0) passed = 0;
    Practice_CopyBytes(&oldTransition, (void*)&TMC_TRANSITION, sizeof(oldTransition));
    gPracticeState.selectedArea = oldArea;
    gPracticeState.selectedRoom = oldRoom;
    gPracticeState.pendingAction = oldPending;
    feature_result(8u, passed);
}

static void test_position_feature(void) {
    PracticePosition oldSaved = gPracticeState.savedPosition;
    u32 oldX = PLAYER32(0x2C);
    u32 oldY = PLAYER32(0x30);
    u32 oldZ = PLAYER32(0x34);
    u32 oldLinear = PLAYER32(0x20);
    u16 oldSpeed = PLAYER16(0x24);
    u16 oldField2A = PLAYER16(0x2A);
    u8 oldAction = PLAYER8(0x0C), oldSubAction = PLAYER8(0x0D);
    u8 oldAnimation = PLAYER8(0x14), oldDirection = PLAYER8(0x15), oldLayer = PLAYER8(0x38);
    u8 oldP2 = PSTATE8(0x02), oldPC = PSTATE8(0x0C), oldP3C = PSTATE8(0x3C);
    u32 targetX = oldX + 0x10000u;
    u32 targetY = oldY + 0x10000u;
    u32 passed;
    gPracticeState.savedPosition.valid = 1;
    gPracticeState.savedPosition.area = TMC_ROOM.area;
    gPracticeState.savedPosition.room = TMC_ROOM.room;
    gPracticeState.savedPosition.layer = oldLayer;
    gPracticeState.savedPosition.direction = oldDirection;
    gPracticeState.savedPosition.animationState = oldAnimation;
    gPracticeState.savedPosition.x = (s32)targetX;
    gPracticeState.savedPosition.y = (s32)targetY;
    gPracticeState.savedPosition.z = (s32)oldZ;
    gPracticeState.pendingAction = PENDING_LOAD_POSITION;
    PracticeRuntime_ApplyPending();
    passed = PLAYER32(0x2C) == targetX && PLAYER32(0x30) == targetY &&
             gPracticeState.pendingAction == PENDING_NONE;
    PLAYER32(0x2C) = oldX; PLAYER32(0x30) = oldY; PLAYER32(0x34) = oldZ;
    PLAYER32(0x20) = oldLinear; PLAYER16(0x24) = oldSpeed; PLAYER16(0x2A) = oldField2A;
    PLAYER8(0x0C) = oldAction; PLAYER8(0x0D) = oldSubAction;
    PLAYER8(0x14) = oldAnimation; PLAYER8(0x15) = oldDirection; PLAYER8(0x38) = oldLayer;
    PSTATE8(0x02) = oldP2; PSTATE8(0x0C) = oldPC; PSTATE8(0x3C) = oldP3C;
    gPracticeState.savedPosition = oldSaved;
    feature_result(16u, passed);
}

static void test_noclip_guard(void) {
    u8 before[0x20];
    u8 after[0x20];
    u8 oldNoClip = gPracticeState.noClip;
    Practice_CopyBytes((const void*)ADDR_G_PLAYER_ENTITY, before, sizeof(before));
    gPracticeState.noClip = 1;
    RespawnGuardDispatch((void*)ADDR_G_PLAYER_ENTITY);
    Practice_CopyBytes((const void*)ADDR_G_PLAYER_ENTITY, after, sizeof(after));
    gPracticeState.noClip = oldNoClip;
    feature_result(32u, bytes_equal(before, after, sizeof(before)));
}

static void test_practice_runtime(void) {
    TmcStats oldStats;
    TmcRoomTransition oldTransition;
    PracticePreset oldPreset;
    u8 oldInventory[34];
    u32 oldFrame = gPracticeState.frameCounter;
    u32 oldTimer = gPracticeState.timerFrames;
    u8 oldTimerRunning = gPracticeState.timerRunning;
    u8 oldPending = gPracticeState.pendingAction;
    u8 oldMainState = TMC_MAIN.state;
    u8 oldMainSubstate = TMC_MAIN.substate;
    u32 oldX = PLAYER32(0x2C), oldY = PLAYER32(0x30), oldZ = PLAYER32(0x34);
    u32 oldLinear = PLAYER32(0x20);
    u16 oldSpeed = PLAYER16(0x24), oldField2A = PLAYER16(0x2A);
    u8 oldAction = PLAYER8(0x0C), oldSubAction = PLAYER8(0x0D);
    u8 oldAnimation = PLAYER8(0x14), oldDirection = PLAYER8(0x15), oldLayer = PLAYER8(0x38);
    u8 oldP2 = PSTATE8(0x02), oldPC = PSTATE8(0x0C), oldP3C = PSTATE8(0x3C);
    u32 passed = gPracticeState.savedPosition.valid != 0;

    Practice_CopyBytes((const void*)&TMC_SAVE.stats, &oldStats, sizeof(oldStats));
    Practice_CopyBytes((const void*)TMC_SAVE.inventory, oldInventory, sizeof(oldInventory));
    Practice_CopyBytes(&gPracticeState.preset, &oldPreset, sizeof(oldPreset));
    Practice_CopyBytes((const void*)&TMC_TRANSITION, &oldTransition, sizeof(oldTransition));

    gPracticeState.timerRunning = 1;
    PracticeRuntime_AfterGame();
    if (gPracticeState.frameCounter != oldFrame + 1 || gPracticeState.timerFrames != oldTimer + 1) passed = 0;

    gPracticeState.preset.valid = 1;
    gPracticeState.preset.position.valid = 1;
    gPracticeState.preset.position.area = TMC_ROOM.area;
    gPracticeState.preset.position.room = TMC_ROOM.room;
    gPracticeState.preset.position.layer = oldLayer;
    gPracticeState.preset.position.direction = oldDirection;
    gPracticeState.preset.position.animationState = oldAnimation;
    gPracticeState.preset.position.x = (s32)(oldX + 0x10000u);
    gPracticeState.preset.position.y = (s32)(oldY + 0x10000u);
    gPracticeState.preset.position.z = (s32)oldZ;
    Practice_CopyBytes(&oldStats, &gPracticeState.preset.stats, sizeof(oldStats));
    Practice_CopyBytes(oldInventory, gPracticeState.preset.inventory, sizeof(oldInventory));
    gPracticeState.preset.stats.health = oldStats.health == 1 ? 2 : 1;
    gPracticeState.preset.inventory[0] ^= 1;
    Practice_CopyBytes(&gPracticeState.preset.stats, (void*)&TMC_SAVE.stats, sizeof(TmcStats));
    Practice_CopyBytes(gPracticeState.preset.inventory, (void*)TMC_SAVE.inventory, 34);
    gPracticeState.pendingAction = PENDING_APPLY_PRESET;
    PracticeRuntime_ApplyPending();
    if (TMC_SAVE.stats.health != gPracticeState.preset.stats.health ||
        TMC_SAVE.inventory[0] != gPracticeState.preset.inventory[0] ||
        PLAYER32(0x2C) != oldX + 0x10000u || PLAYER32(0x30) != oldY + 0x10000u) passed = 0;

    Practice_CopyBytes(&oldTransition, (void*)&TMC_TRANSITION, sizeof(oldTransition));
    gPracticeState.pendingAction = PENDING_RELOAD_ROOM;
    PracticeRuntime_ApplyPending();
    if (TMC_TRANSITION.transitioningOut != 1 || TMC_TRANSITION.type != 5 ||
        TMC_TRANSITION.playerStatus.areaNext != gPracticeState.menuArea ||
        TMC_TRANSITION.playerStatus.roomNext != gPracticeState.menuRoom) passed = 0;
    Practice_CopyBytes(&oldTransition, (void*)&TMC_TRANSITION, sizeof(oldTransition));
    gPracticeState.pendingAction = PENDING_RELOAD_AREA;
    PracticeRuntime_ApplyPending();
    if (TMC_TRANSITION.transitioningOut != 1 || TMC_MAIN.state != 1 || TMC_MAIN.substate != 0) passed = 0;

    Practice_CopyBytes(&oldStats, (void*)&TMC_SAVE.stats, sizeof(oldStats));
    Practice_CopyBytes(oldInventory, (void*)TMC_SAVE.inventory, sizeof(oldInventory));
    Practice_CopyBytes(&oldPreset, &gPracticeState.preset, sizeof(oldPreset));
    Practice_CopyBytes(&oldTransition, (void*)&TMC_TRANSITION, sizeof(oldTransition));
    PLAYER32(0x2C) = oldX; PLAYER32(0x30) = oldY; PLAYER32(0x34) = oldZ;
    PLAYER32(0x20) = oldLinear; PLAYER16(0x24) = oldSpeed; PLAYER16(0x2A) = oldField2A;
    PLAYER8(0x0C) = oldAction; PLAYER8(0x0D) = oldSubAction;
    PLAYER8(0x14) = oldAnimation; PLAYER8(0x15) = oldDirection; PLAYER8(0x38) = oldLayer;
    PSTATE8(0x02) = oldP2; PSTATE8(0x0C) = oldPC; PSTATE8(0x3C) = oldP3C;
    TMC_MAIN.state = oldMainState; TMC_MAIN.substate = oldMainSubstate;
    gPracticeState.frameCounter = oldFrame;
    gPracticeState.timerFrames = oldTimer;
    gPracticeState.timerRunning = oldTimerRunning;
    gPracticeState.pendingAction = oldPending;
    feature_result(64u, passed);
}

static void test_player_runtime(void) {
    u8 oldIframes = PLAYER8(0x3D);
    u16 oldModifier = PSTATE16(0x80);
    u8 oldInvincibility = gPracticeState.invincibility;
    u8 oldApplied = gPracticeState.invincibilityApplied;
    u8 oldSpeedMode = gPracticeState.speedMode;
    u32 passed = 1;

    gPracticeState.invincibilityApplied = 0;
    gPracticeState.invincibility = 1;
    PracticeRuntime_BeforeGame();
    if (PLAYER8(0x3D) != 0xFE || !gPracticeState.invincibilityApplied) passed = 0;
    gPracticeState.invincibility = 0;
    PracticeRuntime_BeforeGame();
    if (PLAYER8(0x3D) != 0 || gPracticeState.invincibilityApplied) passed = 0;
    gPracticeState.speedMode = 1;
    PracticeRuntime_BeforeGame();
    if (PSTATE16(0x80) != oldModifier) passed = 0;
    gPracticeState.speedMode = 2;
    PracticeRuntime_BeforeGame();
    if (PSTATE16(0x80) != oldModifier) passed = 0;

    PLAYER8(0x3D) = oldIframes;
    PSTATE16(0x80) = oldModifier;
    gPracticeState.invincibility = oldInvincibility;
    gPracticeState.invincibilityApplied = oldApplied;
    gPracticeState.speedMode = oldSpeedMode;
    feature_result(128u, passed);
}

static u32 sEnemyBehaviorCalls;
static void count_enemy_behavior(void* entity) { sEnemyBehaviorCalls++; (void)entity; }

static void test_enemy_features(void) {
    volatile u8* priority = (volatile u8*)ADDR_G_PRIORITY_HANDLER;
    volatile u8* entity = (volatile u8*)(ADDR_G_ENTITIES + 71u * 0x88u);
    u8 oldEntity[0x88];
    u8 oldPriority = priority[0];
    u8 oldFreeze = gPracticeState.freezeEnemies;
    u32 passed = 1;

    Practice_CopyBytes((const void*)entity, oldEntity, sizeof(oldEntity));
    Practice_ClearBytes((void*)entity, sizeof(oldEntity));
    entity[0x08] = 3;
    entity[0x0C] = 1;
    entity[0x10] = 0x80;
    entity[0x45] = 4;
    entity[0x18] = 0xAB;
    gPracticeState.freezeEnemies = 1;
    sEnemyBehaviorCalls = 0;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    if (sEnemyBehaviorCalls != 0) passed = 0;
    entity[0x41] = 0x80;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    entity[0x41] = 0; entity[0x45] = 0;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    entity[0x45] = 4; entity[0x42] = 4;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    entity[0x42] = 0; entity[0x6D] = 1;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    gPracticeState.freezeEnemies = 0;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    /* Collision, death, knockback and OFF dispatch; the boss-flag tick freezes. */
    if (sEnemyBehaviorCalls != 4 || priority[0] != oldPriority || entity[0x18] != 0xAB) passed = 0;
    gPracticeState.freezeEnemies = 1; entity[0x6D] = 0; entity[0x10] = 0;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    entity[0x10] = 0x80; entity[0x18] = 0;
    EnemyBehaviorDispatch((void*)entity,count_enemy_behavior);
    /* Visible non-collidable body parts now freeze; invisible ordinary init runs. */
    if (sEnemyBehaviorCalls != 5) passed = 0;

    Practice_CopyBytes(oldEntity, (void*)entity, sizeof(oldEntity));
    priority[0] = oldPriority;
    gPracticeState.freezeEnemies = oldFreeze;
    feature_result(256u, passed);
}

static void test_debug_capture(void) {
    u8 oldPage = gPracticeState.page;
    u8 oldSelectedArea = gPracticeState.selectedArea, oldSelectedRoom = gPracticeState.selectedRoom;
    u8 oldMenuArea = gPracticeState.menuArea, oldMenuRoom = gPracticeState.menuRoom;
    s32 oldMenuX = gPracticeState.menuX, oldMenuY = gPracticeState.menuY, oldMenuZ = gPracticeState.menuZ;
    u16 oldHeld = gPracticeState.menuHeldInput, oldSpeed = gPracticeState.menuSpeed;
    u8 oldDirection = gPracticeState.menuDirection, oldAction = gPracticeState.menuAction;
    u32 passed;
    gPracticeState.page = PAGE_PLAYER;
    PracticeRuntime_CaptureOpenContext();
    passed = gPracticeState.page == PAGE_PLAYER && gPracticeState.menuArea == TMC_ROOM.area &&
             gPracticeState.menuRoom == TMC_ROOM.room && gPracticeState.menuX == (s32)PLAYER32(0x2C) &&
             gPracticeState.menuY == (s32)PLAYER32(0x30) && gPracticeState.menuZ == (s32)PLAYER32(0x34) &&
             gPracticeState.menuSpeed == PLAYER16(0x24) && gPracticeState.menuDirection == PLAYER8(0x15) &&
             gPracticeState.menuAction == PLAYER8(0x0C);
    gPracticeState.page = oldPage;
    gPracticeState.selectedArea = oldSelectedArea; gPracticeState.selectedRoom = oldSelectedRoom;
    gPracticeState.menuArea = oldMenuArea; gPracticeState.menuRoom = oldMenuRoom;
    gPracticeState.menuX = oldMenuX; gPracticeState.menuY = oldMenuY; gPracticeState.menuZ = oldMenuZ;
    gPracticeState.menuHeldInput = oldHeld; gPracticeState.menuSpeed = oldSpeed;
    gPracticeState.menuDirection = oldDirection; gPracticeState.menuAction = oldAction;
    feature_result(512u, passed);
}

static void test_menu_access(void) {
    u8 oldKind = PLAYER8(0x08);
    u8 oldAction = PLAYER8(0x0C);
    u8 oldCutscene = PSTATE8(0x8B);
    u8 oldHealth = TMC_SAVE.stats.health;
    u8 oldTransition = TMC_TRANSITION.transitioningOut;
    u8 oldFade = *(volatile u8*)ADDR_G_FADE_CONTROL;
    u8 oldMessage = *(volatile u8*)ADDR_G_MESSAGE;
    u8 oldText = *(volatile u8*)(ADDR_G_TEXT_RENDER + 5);
    u8 oldUpdate = *(volatile u8*)(ADDR_G_TEXT_RENDER + 0x9D);
    u8 oldChoice = *(volatile u8*)ADDR_G_MESSAGE_CHOICES;
    u8 oldScratch = *(volatile u8*)ADDR_G_MESSAGE_SCRATCH;
    u8 oldCurrentWindow = *(volatile u8*)(ADDR_G_CURRENT_WINDOW + 1);
    u8 oldNewWindow = *(volatile u8*)(ADDR_G_NEW_WINDOW + 1);
    u8 oldTextGfx = *(volatile u8*)ADDR_G_TEXT_GFX_BUFFER;
    u32 passed = 1;

    PLAYER8(0x08) = 1;
    PLAYER8(0x0C) = 9;
    PSTATE8(0x8B) = 7;
    if (TMC_SAVE.stats.health == 0) TMC_SAVE.stats.health = 8;
    TMC_TRANSITION.transitioningOut = 0;
    *(volatile u8*)ADDR_G_FADE_CONTROL = 0;
    if (!PracticeRuntime_CanOpenMenu()) passed = 0;
    *(volatile u8*)ADDR_G_FADE_CONTROL = 1;
    if (PracticeRuntime_CanOpenMenu()) passed = 0;
    *(volatile u8*)ADDR_G_FADE_CONTROL = 0;
    TMC_TRANSITION.transitioningOut = 1;
    if (PracticeRuntime_CanOpenMenu()) passed = 0;

    *(volatile u8*)ADDR_G_MESSAGE = 3;
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 5) = 0x51;
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 0x9D) = 0;
    *(volatile u8*)ADDR_G_MESSAGE_CHOICES = 0x52;
    *(volatile u8*)ADDR_G_MESSAGE_SCRATCH = 0x53;
    *(volatile u8*)(ADDR_G_CURRENT_WINDOW + 1) = 0x54;
    *(volatile u8*)(ADDR_G_NEW_WINDOW + 1) = 0x55;
    *(volatile u8*)ADDR_G_TEXT_GFX_BUFFER = 0x56;
    PracticeRuntime_CaptureOpenContext();
    if (!gPracticeState.pausedMessageValid) passed = 0;
    *(volatile u8*)ADDR_G_MESSAGE = 0;
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 5) = 0;
    *(volatile u8*)ADDR_G_MESSAGE_CHOICES = 0;
    *(volatile u8*)ADDR_G_MESSAGE_SCRATCH = 0;
    *(volatile u8*)(ADDR_G_CURRENT_WINDOW + 1) = 0;
    *(volatile u8*)(ADDR_G_NEW_WINDOW + 1) = 0;
    *(volatile u8*)ADDR_G_TEXT_GFX_BUFFER = 0;
    gPracticeState.pausedMessageRestorePending = 1;
    PracticeRuntime_RestorePausedMessage();
    if (*(volatile u8*)ADDR_G_MESSAGE != 3 ||
        *(volatile u8*)(ADDR_G_TEXT_RENDER + 5) != 0x51 ||
        *(volatile u8*)(ADDR_G_TEXT_RENDER + 0x9D) != 1 ||
        *(volatile u8*)ADDR_G_MESSAGE_CHOICES != 0x52 ||
        *(volatile u8*)ADDR_G_MESSAGE_SCRATCH != 0x53 ||
        *(volatile u8*)(ADDR_G_CURRENT_WINDOW + 1) != 0x54 ||
        *(volatile u8*)(ADDR_G_NEW_WINDOW + 1) != 0x55 ||
        *(volatile u8*)ADDR_G_TEXT_GFX_BUFFER != 0x56 ||
        gPracticeState.pausedMessageValid ||
        gPracticeState.pausedMessageRestorePending) passed = 0;

    PLAYER8(0x08) = oldKind;
    PLAYER8(0x0C) = oldAction;
    PSTATE8(0x8B) = oldCutscene;
    TMC_SAVE.stats.health = oldHealth;
    TMC_TRANSITION.transitioningOut = oldTransition;
    *(volatile u8*)ADDR_G_FADE_CONTROL = oldFade;
    *(volatile u8*)ADDR_G_MESSAGE = oldMessage;
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 5) = oldText;
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 0x9D) = oldUpdate;
    *(volatile u8*)ADDR_G_MESSAGE_CHOICES = oldChoice;
    *(volatile u8*)ADDR_G_MESSAGE_SCRATCH = oldScratch;
    *(volatile u8*)(ADDR_G_CURRENT_WINDOW + 1) = oldCurrentWindow;
    *(volatile u8*)(ADDR_G_NEW_WINDOW + 1) = oldNewWindow;
    *(volatile u8*)ADDR_G_TEXT_GFX_BUFFER = oldTextGfx;
    feature_result(4096u, passed);
}

static void test_native_save_roundtrip(void) {
    u32 slot = *(volatile u8*)0x02000004u;
    u32 writeResult = 0;
    u32 readResult = 0;
    u32 passed = slot < 3;
    Practice_ClearBytes(gPracticeAutoSaveReadback, sizeof(gPracticeAutoSaveReadback));
    if (passed) {
        writeResult = CALL_U32_2(TMC_WRITE_SAVE_FILE)(slot, (void*)ADDR_G_SAVE);
        readResult = CALL_U32_2(TMC_READ_SAVE_FILE)(slot, gPracticeAutoSaveReadback);
        passed = writeResult == 1 && readResult == 1 &&
                 bytes_equal((const void*)ADDR_G_SAVE, gPracticeAutoSaveReadback,
                             sizeof(gPracticeAutoSaveReadback));
    }
    feature_result(2048u, passed);
}

static void test_resource_health_hook(void) {
    TmcStats stats;
    u8 health = PLAYER8(0x45), action = PLAYER8(0x0C), kind = PLAYER8(8);
    u8 control = PSTATE8(0x8B), killed = PSTATE8(0x3C);
    u8 message = *(volatile u8*)ADDR_G_MESSAGE, fade = *(volatile u8*)ADDR_G_FADE_CONTROL;
    u8 transition = TMC_TRANSITION.transitioningOut, opened = gPracticeState.menuOpen;
    u8 bits = gPracticeState.resourceCheats;
    u32 passed = 1, i, j;
    static const s32 changes[] = {-255,-8,0,8,255};
    s32 (*nativeHealth)(s32) = (s32(*)(s32))0x080526A1u;
    Practice_CopyBytes((const void*)&TMC_SAVE.stats,&stats,sizeof(stats));
    PLAYER8(8) = 1; PLAYER8(0x0C) = 1; PSTATE8(0x8B) = PSTATE8(0x3C) = 0;
    *(volatile u8*)ADDR_G_MESSAGE = *(volatile u8*)ADDR_G_FADE_CONTROL = 0;
    TMC_TRANSITION.transitioningOut = gPracticeState.menuOpen = 0;
    /* Exercise the patched native entry, including return value and mirrors.
     * Boundary fixtures only: never resume gameplay in the synthetic states. */
    for (i = 0; i < 2; i++) {
        gPracticeState.resourceCheats = i ? CHEAT_HEARTS : 0;
        for (j = 0; j < ARRAY_COUNT(changes); j++) {
            s32 expected = 16 + changes[j];
            TMC_SAVE.stats.health = PLAYER8(0x45) = 16; TMC_SAVE.stats.maxHealth = 24;
            if (i && changes[j] < 0) expected = 24;
            if (expected < 0) expected = 0;
            if (expected > 24) expected = 24;
            if (nativeHealth(changes[j]) != expected || TMC_SAVE.stats.health != expected ||
                PLAYER8(0x45) != expected) passed = 0;
        }
    }
    gPracticeState.resourceCheats = CHEAT_HEARTS;
    TMC_SAVE.stats.health = PLAYER8(0x45) = 16; PSTATE8(0x8B) = 1;
    if (nativeHealth(-8) != 8) passed = 0; /* Script owns health. */
    PSTATE8(0x8B) = 0; TMC_SAVE.stats.health = PLAYER8(0x45) = 0;
    if (nativeHealth(-8) != 0) passed = 0; /* Never resurrect a dead player. */
    PSTATE8(0x3C) = 1; TMC_SAVE.stats.health = PLAYER8(0x45) = 16;
    if (nativeHealth(-8) != 8) passed = 0;
    Practice_CopyBytes(&stats,(void*)&TMC_SAVE.stats,sizeof(stats));
    PLAYER8(0x45) = health; PLAYER8(0x0C) = action; PLAYER8(8) = kind;
    PSTATE8(0x8B) = control; PSTATE8(0x3C) = killed;
    *(volatile u8*)ADDR_G_MESSAGE = message; *(volatile u8*)ADDR_G_FADE_CONTROL = fade;
    TMC_TRANSITION.transitioningOut = transition; gPracticeState.menuOpen = opened;
    gPracticeState.resourceCheats = bits;
    feature_result(8192u,passed);
}

static void run_feature_tests(void) {
    test_player_features();
    test_inventory_features();
    test_flag_features();
    test_warp_features();
    test_position_feature();
    test_noclip_guard();
    test_practice_runtime();
    test_player_runtime();
    test_enemy_features();
    test_debug_capture();
    test_native_save_roundtrip();
    test_menu_access();
    test_resource_health_hook();
}

static void initialize(void) {
    u8* bytes;
    u32 index;
    if (gPracticeAutoTest.magic == AUTOTEST_MAGIC) return;
    bytes = (u8*)&gPracticeAutoTest;
    for (index = 0; index < sizeof(gPracticeAutoTest); index++) bytes[index] = 0;
    gPracticeAutoTest.magic = AUTOTEST_MAGIC;
    gPracticeAutoTest.lastTask = TMC_MAIN.task;
}

static void before_task(void) {
    u32 frame;
    clear_input();
    gPracticeAutoTest.totalFrames++;
    if (gPracticeAutoTest.lastTask != TMC_MAIN.task) {
        gPracticeAutoTest.lastTask = TMC_MAIN.task;
        gPracticeAutoTest.taskFrames = 0;
    } else {
        gPracticeAutoTest.taskFrames++;
    }
    frame = gPracticeAutoTest.taskFrames;

    if (TMC_MAIN.task == 0) {
        if (frame >= 650 && frame < 800) set_input(KEY_START);
        return;
    }
    if (TMC_MAIN.task == 1) {
        /* The task-table indirection used only by this test ROM prevents the
         * engine's fade-completion callback from advancing this empty wait
         * state. Resume at Nintendo's normal completion state after the fade. */
        if (TMC_MAIN.state == 1 && frame >= 120) TMC_MAIN.state = 2;
        /* Separate four-frame presses keep native edge/repeat behavior intact
         * across file-card, Start-button, and fade timing variations. */
        if (frame >= 90 && ((frame - 90) % 120) < 4) set_input(KEY_A);
        return;
    }
    if (TMC_MAIN.task != TASK_GAME) return;

    switch (gPracticeAutoTest.phase) {
        case AUTO_WAIT_GAME:
            if (TMC_MAIN.state == GAMETASK_MAIN && TMC_MAIN.substate == GAMEMAIN_UPDATE) {
                gPracticeAutoTest.status |= 1u;
                if (PLAYER8(0x08) == 1 && PLAYER8(0x0C) == 1 && PSTATE8(0x8B) == 0 &&
                    TMC_SAVE.stats.health != 0 && TMC_TRANSITION.transitioningOut == 0) {
                    gPracticeAutoTest.waitFrames = 0;
                    gPracticeAutoTest.readyFrames++;
                    if (gPracticeAutoTest.readyFrames >= 45) {
                        set_input(PRACTICE_HOTKEY);
                        gPracticeAutoTest.phase = AUTO_WAIT_OPEN;
                    }
                } else {
                    gPracticeAutoTest.readyFrames = 0;
                    gPracticeAutoTest.waitFrames++;
                    if (gPracticeAutoTest.waitFrames >= 120) {
                        PLAYER8(0x0C) = 1;
                        PLAYER8(0x0D) = 0;
                        PSTATE8(0x8B) = 0;
                    }
                }
            } else {
                gPracticeAutoTest.readyFrames = 0;
                if (TMC_MAIN.state == GAMETASK_MAIN && TMC_MAIN.substate == GAMEMAIN_SUBTASK &&
                    !gPracticeState.menuOpen) {
                    gPracticeAutoTest.waitFrames++;
                    if (gPracticeAutoTest.waitFrames == 120) CALL_VOID0(TMC_SUBTASK_EXIT)();
                } else {
                    gPracticeAutoTest.waitFrames = 0;
                }
            }
            break;
        case AUTO_ROOT_SETTLE:
            gPracticeAutoTest.waitFrames++;
            if (gPracticeAutoTest.waitFrames == 30) set_input(KEY_A);
            if (gPracticeAutoTest.waitFrames >= 31) gPracticeAutoTest.phase = AUTO_WAIT_PRACTICE;
            break;
        case AUTO_PRACTICE_SETTLE:
            gPracticeAutoTest.waitFrames++;
            if (gPracticeAutoTest.waitFrames == 10) set_input(KEY_A);
            if (gPracticeAutoTest.waitFrames == 30) set_input(KEY_B);
            if (gPracticeAutoTest.waitFrames >= 31) gPracticeAutoTest.phase = AUTO_WAIT_BACK;
            break;
        case AUTO_BACK_SETTLE:
            gPracticeAutoTest.waitFrames++;
            if (gPracticeAutoTest.waitFrames == 30) set_input(PRACTICE_HOTKEY);
            if (gPracticeAutoTest.waitFrames >= 31) gPracticeAutoTest.phase = AUTO_WAIT_CLOSE;
            break;
        case AUTO_REOPEN_SETTLE:
            gPracticeAutoTest.waitFrames++;
            if (gPracticeAutoTest.waitFrames >= 30 && *(volatile u8*)ADDR_G_UI == 0) {
                PracticeRuntime_CaptureOpenContext();
                CALL_VOID2(TMC_MENU_FADE_IN)(PRACTICE_SUBTASK, 0);
                gPracticeAutoTest.phase = AUTO_WAIT_REOPEN;
            }
            break;
        case AUTO_RECLOSE_SETTLE:
            gPracticeAutoTest.waitFrames++;
            if (gPracticeAutoTest.waitFrames == 10 || gPracticeAutoTest.waitFrames == 20 ||
                gPracticeAutoTest.waitFrames == 40)
                set_input(KEY_DOWN);
            if (gPracticeAutoTest.waitFrames == 30) set_input(KEY_UP);
            if (gPracticeAutoTest.waitFrames == 50 || gPracticeAutoTest.waitFrames == 60 ||
                gPracticeAutoTest.waitFrames == 80 || gPracticeAutoTest.waitFrames == 90 ||
                gPracticeAutoTest.waitFrames == 110)
                set_input(KEY_A);
            if (gPracticeAutoTest.waitFrames == 70) set_input(KEY_DOWN);
            if (gPracticeAutoTest.waitFrames == 100) set_input(KEY_UP);
            if (gPracticeAutoTest.waitFrames == 120) set_input(KEY_B);
            if (gPracticeAutoTest.waitFrames == 140) set_input(PRACTICE_HOTKEY);
            if (gPracticeAutoTest.waitFrames >= 141) gPracticeAutoTest.phase = AUTO_WAIT_RECLOSE;
            break;
        default:
            break;
    }
}

static void after_task(void) {
    if (TMC_MAIN.task != TASK_GAME) return;
    gPracticeAutoTest.lastPage = gPracticeState.page;
    gPracticeAutoTest.lastMenuOpen = gPracticeState.menuOpen;
    switch (gPracticeAutoTest.phase) {
        case AUTO_RECLOSE_SETTLE:
            if (gPracticeAutoTest.waitFrames == 10 && gPracticeState.cursor[PAGE_ROOT] == 1)
                gPracticeAutoTest.layoutStatus |= 32u;
            if (gPracticeAutoTest.waitFrames == 20 && gPracticeState.cursor[PAGE_ROOT] == 2)
                gPracticeAutoTest.layoutStatus |= 64u;
            if (gPracticeAutoTest.waitFrames == 30 && gPracticeState.cursor[PAGE_ROOT] == 1)
                gPracticeAutoTest.layoutStatus |= 128u;
            if (gPracticeAutoTest.waitFrames == 40 && gPracticeState.cursor[PAGE_ROOT] == 2)
                gPracticeAutoTest.layoutStatus |= 256u;
            if (gPracticeAutoTest.waitFrames == 50 && gPracticeState.page == PAGE_MOVEMENT)
                gPracticeAutoTest.layoutStatus |= 512u;
            if (gPracticeAutoTest.waitFrames == 60 && gPracticeState.noClip && gPracticeState.roomExplore)
                gPracticeAutoTest.movementStatus |= 1u;
            if (gPracticeAutoTest.waitFrames == 80 && gPracticeState.noClip && gPracticeState.speedMode == 1)
                gPracticeAutoTest.movementStatus |= 2u;
            if (gPracticeAutoTest.waitFrames == 90 && gPracticeState.noClip && gPracticeState.speedMode == 2)
                gPracticeAutoTest.movementStatus |= 4u;
            if (gPracticeAutoTest.waitFrames == 110 && !gPracticeState.noClip && !gPracticeState.roomExplore)
                gPracticeAutoTest.movementStatus |= 8u;
            if (gPracticeAutoTest.waitFrames == 110)
                feature_result(1024u, gPracticeAutoTest.movementStatus == 15u);
            if (gPracticeAutoTest.waitFrames == 120 && gPracticeState.page == PAGE_ROOT)
                gPracticeAutoTest.layoutStatus |= 1024u;
            break;
        case AUTO_WAIT_OPEN:
            if (gPracticeState.menuOpen && gPracticeState.page == PAGE_ROOT) {
                gPracticeAutoTest.status |= 2u;
                gPracticeAutoTest.rootTileChecksum = tilemap_checksum();
                if (tilemap_has_text(2, 1, "THE MINISH CAP")) gPracticeAutoTest.layoutStatus |= 1u;
                if (tilemap_has_text(3, 3, "PRACTICE")) gPracticeAutoTest.layoutStatus |= 2u;
                gPracticeAutoTest.waitFrames = 0;
                gPracticeAutoTest.phase = AUTO_ROOT_SETTLE;
            }
            break;
        case AUTO_WAIT_PRACTICE:
            if (gPracticeState.menuOpen && gPracticeState.page == PAGE_PRACTICE) {
                gPracticeAutoTest.status |= 4u;
                gPracticeAutoTest.practiceTileChecksum = tilemap_checksum();
                if (tilemap_has_text(2, 1, "PRACTICE")) gPracticeAutoTest.layoutStatus |= 4u;
                if (tilemap_has_text(3, 3, "SAVE POSITION")) gPracticeAutoTest.layoutStatus |= 8u;
                gPracticeAutoTest.waitFrames = 0;
                gPracticeAutoTest.phase = AUTO_PRACTICE_SETTLE;
            }
            break;
        case AUTO_WAIT_BACK:
            if (gPracticeState.menuOpen && gPracticeState.page == PAGE_ROOT) {
                gPracticeAutoTest.status |= 8u;
                gPracticeAutoTest.backTileChecksum = tilemap_checksum();
                if (tilemap_has_text(2, 1, "THE MINISH CAP") &&
                    tilemap_has_text(3, 3, "PRACTICE"))
                    gPracticeAutoTest.layoutStatus |= 16u;
                gPracticeAutoTest.waitFrames = 0;
                gPracticeAutoTest.phase = AUTO_BACK_SETTLE;
            }
            break;
        case AUTO_WAIT_CLOSE:
            if (!gPracticeState.menuOpen && TMC_MAIN.state == GAMETASK_MAIN &&
                TMC_MAIN.substate == GAMEMAIN_UPDATE) {
                gPracticeAutoTest.status |= 16u;
                gPracticeAutoTest.phase = AUTO_FEATURE_TESTS;
            }
            break;
        case AUTO_FEATURE_TESTS:
            run_feature_tests();
            gPracticeAutoTest.waitFrames = 0;
            gPracticeAutoTest.phase = AUTO_REOPEN_SETTLE;
            break;
        case AUTO_WAIT_REOPEN:
            if (gPracticeState.menuOpen && gPracticeState.page == PAGE_ROOT) {
                gPracticeAutoTest.status |= 32u;
                gPracticeAutoTest.waitFrames = 0;
                gPracticeAutoTest.phase = AUTO_RECLOSE_SETTLE;
            }
            break;
        case AUTO_WAIT_RECLOSE:
            if (!gPracticeState.menuOpen && TMC_MAIN.state == GAMETASK_MAIN &&
                TMC_MAIN.substate == GAMEMAIN_UPDATE && PLAYER8(0x0C) == 1 &&
                PSTATE8(0x8B) == 0 && TMC_TRANSITION.transitioningOut == 0) {
                gPracticeAutoTest.status |= 64u;
                gPracticeAutoTest.movementStatus |= 0x100u;
                gPracticeAutoTest.phase = AUTO_DONE;
            }
            break;
        default:
            break;
    }
}

__attribute__((used, section(".entry")))
void AutoTestTaskWrapper(void) {
    initialize();
    before_task();
    switch (TMC_MAIN.task) {
        case 0:
            CALL_VOID0(TMC_TITLE_TASK)();
            break;
        case 1:
            CALL_VOID0(TMC_FILE_SELECT_TASK)();
            break;
        case TASK_GAME:
            GameTaskWrapper();
            break;
        default:
            break;
    }
    after_task();
}
