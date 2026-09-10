#ifndef PRACTICE_MENU_H
#define PRACTICE_MENU_H

#include "tmc_us_symbols.h"

#define PRACTICE_MAGIC 0x504D4331u
#define MAX_MENU_PAGES 31
#define MAX_WARP_FAVORITES 8
#define FULL_SAVE_SIZE 0x4B4
#define MAX_FLAG_PRESET_ENTRIES 16
#define CHEAT_HEARTS 1u
#define CHEAT_BOMBS 2u
#define CHEAT_ARROWS 4u
#define CHEAT_RUPEES 8u
#define CHEAT_MINIGAME_TIME 16u

typedef enum {
    PAGE_ROOT,
    PAGE_PRACTICE,
    PAGE_PLAYER,
    PAGE_INVENTORY,
    PAGE_WEAPONS,
    PAGE_BOTTLES,
    PAGE_UPGRADES,
    PAGE_QUEST,
    PAGE_WARP,
    PAGE_MOVEMENT,
    PAGE_ENEMIES,
    PAGE_FLAGS,
    PAGE_DEBUG,
    PAGE_SETTINGS,
    PAGE_CONFIRM_ALL,
    PAGE_ELEMENTS,
    PAGE_CHEATS,
    PAGE_CONFIRM_DELETE,
    PAGE_NUDGE,
    PAGE_KNOWN_FLAGS,
    PAGE_RAW_FLAGS,
    PAGE_BETA,
    PAGE_CONFIRM_COMPLETE,
    PAGE_FAVORITES,
    PAGE_BINDINGS,
    PAGE_CONFIRM_UNDO,
    PAGE_ACTORS,
    PAGE_ACTOR_LIST,
    PAGE_ACTOR_DETAIL,
    PAGE_ACTOR_SPAWN,
    PAGE_ACTOR_DELETE,
} MenuPage;

typedef enum {
    PENDING_NONE,
    PENDING_LOAD_POSITION,
    PENDING_RELOAD_ROOM,
    PENDING_RELOAD_AREA,
    PENDING_WARP,
    PENDING_APPLY_PRESET,
    PENDING_NUDGE,
    PENDING_GROUND_RESET,
    PENDING_BREAK_FREE,
    PENDING_OCARINA_GLITCH,
    PENDING_COMPLETE_GAME,
    PENDING_BETA_ENTER,
    PENDING_BETA_RETURN,
    PENDING_UNDO_COMPLETE,
    PENDING_ACTOR_SPAWN,
    PENDING_ACTOR_DELETE,
    PENDING_LINK_TO_ACTOR,
    PENDING_ACTOR_TO_LINK,
    PENDING_STORY_INTRO,
    PENDING_STORY_ENDING,
} PendingAction;

typedef struct {
    u8 valid;
    u8 area;
    u8 room;
    u8 layer;
    u8 direction;
    u8 animationState;
    u16 originX;
    u16 originY;
    s32 x;
    s32 y;
    s32 z;
} PracticePosition;

typedef struct {
    u16 index;
    u8 value;
    u8 bank;
} PresetFlag;

typedef struct {
    u8 valid;
    u8 flagCount;
    u8 reserved[2];
    PracticePosition position;
    TmcStats stats;
    u8 inventory[34];
    PresetFlag flags[MAX_FLAG_PRESET_ENTRIES];
} PracticePreset;

typedef struct {
    u32 magic;
    u32 frameCounter;
    u32 timerFrames;
    u8 timerRunning;
    u8 menuOpen;
    u8 noClip;
    u8 roomExplore;
    u8 invincibility;
    u8 invincibilityApplied;
    u8 speedMode;
    u8 freezeEnemies;
    u8 resourceCheats; /* Reuse reserved byte: existing runtime/test ABI stays fixed. */
    u8 menuTheme;
    u8 debugHud;
    u8 page;
    u8 cursor[MAX_MENU_PAGES];
    u8 pendingAction;
    u8 statusTimer;
    u8 selectedArea;
    u8 selectedRoom;
    u8 flagBank;
    u8 flagOperation;
    u16 flagIndex;
    u8 menuArea;
    u8 menuRoom;
    u16 menuOriginX;
    u16 menuOriginY;
    u16 menuLocalFlagOffset;
    u8 menuDungeonIndex;
    u8 menuCollisionLayer;
    s32 menuX;
    s32 menuY;
    s32 menuZ;
    u16 menuHeldInput;
    u16 menuSpeed;
    u8 menuDirection;
    u8 menuAnimationState;
    u8 menuAction;
    u8 menuFrameState;
    u8 menuFloorType;
    u8 menuEntityCount;
    char status[29];
    PracticePosition savedPosition;
    PracticePreset preset;
    u8 hudEntryCount;
    u8 hudFontReady;
    u16 hudPositions[64];
    u16 hudBackup[64];
    u8 pausedMessageValid;
    u8 pausedMessageRestorePending;
    u8 pausedMessage[0x20];
    u8 pausedTextRender[0xA8];
    u8 pausedMessageChoices[0x18];
    u8 pausedMessageScratch[0x10];
    u8 pausedCurrentWindow[8];
    u8 pausedNewWindow[8];
    u8 pausedTextGfx[0xD00];
    u8 inventoryDirty;
    u8 lockDirection;
    u8 facingApplied;
    u8 facing;
    u8 cameraMode;
    u8 cameraActive;
    u8 cameraArea;
    u8 cameraRoom;
    u8 nudgeDirection;
    u8 nudgeStep;
    u8 knownFlag;
    u8 flagBefore;
    u8 flagAfter;
    u8 flagChangedBank;
    u16 flagChangedIndex;
    u32 cameraOwner;
    u32 cameraEntity[34];
    u8 contextRestorePending;
    u8 pausedPriority[10];
    u8 pausedPlayerPriority;
    u8 pausedPauseDisabled;
    u8 flagGroup;
    u8 collectionAction;
    u8 flagLogCount;
    struct { u16 index; u8 bank, before, after, dungeon; } flagLog[8];
    PracticePosition betaReturn;
    u16 sceneSkipTicks;
    u16 menuHotkey, confirmHotkey, bindingDraft;
    u8 bindingTarget;
    u8 favorites[MAX_WARP_FAVORITES][2]; /* FF/FF = empty. */
    u8 completionUndoValid, completionUndoSlot;
    PracticePosition completionReturn;
    u8 completionBackup[FULL_SAVE_SIZE]; /* Includes native quest timers after 48C. */
    u32 actorSelected, actorIdentity;
    u8 actorMarks[112]; /* Normal pool 0..71, player/aux 72..79, managers 80..111. */
    u8 actorFilter, actorSpawn, actorArea, actorRoom, actorContext;
    s32 actorPose[80][3]; /* Exact 16.16 XYZ, including all eight player/aux slots. */
    u8 actorRaw, actorRawKind, actorRawId, actorRawType, actorRawType2;
    u8 actorRawTimer, actorRawSubtimer, actorRawFlags, actorRawParent, actorRawLayer;
    u8 actorConfirmSpawn, actorMoveStep, actorExpert, breakFreeTicks;
    u8 modalMode, modalSkipTail, modalContext, modalSaveEdits;
    u8 sceneReplay, sceneReturn, sceneSeen, sceneConfirm;
} PracticeState;

extern PracticeState gPracticeState;

void Practice_CopyBytes(const void* source, void* destination, u32 size);
void Practice_ClearBytes(void* destination, u32 size);

void PracticeRuntime_Init(void);
void Settings_Load(void);
void Settings_Save(void);
void Completion_Apply(void);
void Completion_Capture(void);
u32 Completion_CanUndo(void);
void Completion_Restore(void);
u32 Settings_ValidKeys(u16 menu, u16 confirm);
void Settings_KeyText(u16 keys, char* text);
void SceneSkip_Start(void);
void SceneSkip_Update(void);
void SceneWarp_Start(u32 ending);
u32 SceneWarp_Tick(void);
void SceneWarp_Return(void);
void PracticeRuntime_CaptureOpenContext(void);
void PracticeRuntime_Queue(PendingAction action);
void PracticeRuntime_ApplyPending(void);
void PracticeRuntime_BeforeGame(void);
void PracticeRuntime_AfterGame(void);
void PracticeRuntime_SetStatus(const char* text);
void PracticeRuntime_CloseMenu(void);
u32 PracticeRuntime_CanOpenMenu(void);
void PracticeRuntime_RestorePausedMessage(void);
u32 PracticeModal_Tick(void);
u32 PracticeModal_CanAct(u32 page,u32 row);
void PracticeScreenTaskWrapper(void);
void PracticeFrameTailDispatch(void);

void PracticeHud_BeforeGame(void);
void PracticeHud_AfterGame(void);

void PracticeRender_Init(void);
void PracticeRender_Theme(void);
void PracticeRender_Clear(void);
void PracticeRender_Text(u32 x, u32 y, const char* text, u32 palette);
void PracticeRender_U32(u32 x, u32 y, u32 value, u32 digits, u32 palette);
void PracticeRender_S32(u32 x, u32 y, s32 value, u32 digits, u32 palette);
void PracticeRender_Hex(u32 x, u32 y, u32 value, u32 digits, u32 palette);
void PracticeRender_Frame(const char* title, const char* subtitle);
void PracticeRender_Commit(void);
void PracticeRender_BuildFontAt(u32 baseTile);
void PracticeRender_HudFont(void);

void PracticeMenu_Update(void);
void PracticeMenu_Draw(void);

void Player_FullHealth(void);
void Player_AdjustMaxHealth(s32 direction);
void Player_AdjustRupees(s32 amount);

u32 Inventory_Get(u32 item);
void Inventory_Set(u32 item, u32 value);
void Inventory_SetWeaponGroup(u32 first, u32 last, u32 selected);
void Inventory_GiveAll(void);
void Inventory_DeleteAll(void);
void Inventory_GiveCollection(u32 which);
u32 FigurineAvailableDispatch(void* device, u32 id);
void Inventory_Refresh(void);
u32 Inventory_GroupSelection(u32 first, u32 last);

u32 Warp_IsValid(u32 area, u32 room);
typedef struct {
    u8 area, room;
    u16 x, y;
    u8 layer, facing;
    const char* name;
} WarpRoomInfo;
const char* Warp_AreaName(u32 area);
const WarpRoomInfo* Warp_RoomInfo(u32 area, u32 room);
const TmcRoomHeader* Warp_GetHeader(u32 area, u32 room);
void Warp_AdjustArea(s32 direction);
void Warp_AdjustRoom(s32 direction);
s32 Warp_FavoriteIndex(void);
void Warp_ToggleFavorite(void);

u32 Flags_Read(void);
void Flags_Write(u32 value);
const char* Flags_KnownName(void);
u32 Flags_KnownRead(void);
void Flags_KnownWrite(u32 value);
void Flags_Record(u32 bank, u32 index, u32 before, u32 after);
u32 Flags_ListCount(void);
const char* Flags_ListName(u32 row);
u32 Flags_ListRead(u32 row);
void Flags_ListEdit(u32 row, s32 direction);

void Enemies_BeforeGame(void);
void Enemies_AfterGame(void);
void Actors_Context(void);
void Actors_Action(u32 row, u16 pressed, s32 direction);
void Actors_Draw(void);
u32 Actors_Count(void);
u32 Actors_Frozen(void* entity);
void Actors_Apply(u32 action);
void Actors_DeleteDispatch(void* entity);
void Actors_DeleteManagerDispatch(void* entity);
void Actors_ItemPickupDispatch(void* entity, u32 collected);
void Inventory_ElementFlag(u32 item, u32 owned);
void Actors_UpdateDispatch(void* entity);
void Actors_RestorePoses(void);
void MinigameTimerDispatch(void* entity);
void MinigameDarknutDispatch(void* timer);
void Minigame_ActorDispatch(void* entity,void (*native)(void*));
const char* Actors_NativeName(u32 kind,u32 id);
const char* Actors_GroundItemName(u32 type);
void Cheats_BreakFreeUpdate(void);
void EnemyBehaviorDispatch(void* entity, void (*behavior)(void*));

void GameTaskWrapper(void);
void AutoTestTaskWrapper(void);
void PracticeDebugTask(void);
void MovementDispatch(void* entity);
void ExplorationDispatch(u32 direction);
void RespawnGuardDispatch(void* entity);
void ConveyorDispatch(void* entity);
void PitDispatch(void* entity);
void SurfaceMinishFrontDispatch(void* entity);
void Surface21Dispatch(void* entity);
void ScrollFollowDispatch(void* controls);
u32 Movement_IsControlled(void);
u32 Movement_IsSafeGround(s32 x, s32 y);
void Movement_BeforeGame(void);
void Movement_AfterGame(void);
void Movement_ReleaseCamera(void);
void Movement_ApplyNudge(void);
void Movement_GroundReset(void);
void Cheats_BreakFree(void);
void Cheats_OcarinaGlitch(void);
void Cheats_RefillResources(void);
s32 Cheats_ModHealth(s32 delta);

#endif
