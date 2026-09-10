#ifndef TMC_US_SYMBOLS_H
#define TMC_US_SYMBOLS_H

#include "types.h"

/* The Legend of Zelda: The Minish Cap (USA), SHA-1 B4BD50...7130 only. */
#define TMC_GAME_TASK                 0x08051989u
#define TMC_TITLE_TASK                0x080AD381u
#define TMC_FILE_SELECT_TASK          0x08050451u
#define TMC_MENU_FADE_IN              0x080A7139u
#define TMC_SUBTASK_EXIT              0x080A71DDu
#define TMC_SET_FADE_INVERTED         0x080500F5u
#define TMC_ORIGINAL_MOVEMENT         0x0800857Du
#define TMC_ORIGINAL_EXPLORATION      0x0807A5B9u
#define TMC_ORIGINAL_RESPAWN_GUARD    0x08008AC7u
#define TMC_RESET_ACTIVE_ITEMS        0x08077B2Du
#define TMC_PUT_AWAY_ITEMS            0x08077B21u
#define TMC_DELETE_CLONES             0x0807A109u
#define TMC_UPDATE_PLAYER_SKILLS      0x0807AEE5u
#define TMC_LINEAR_MOVE_UPDATE        0x0806F69Du
#define TMC_PIT_GUARD                 0x080741C5u
#define TMC_PIT_COLLISION_TEST        0x08079C31u
#define TMC_SURFACE_MINISH_FRONT      0x08074639u
#define TMC_SURFACE_21                0x0807479Du
#define TMC_WRITE_SAVE_FILE           0x0807CF09u
#define TMC_READ_SAVE_FILE            0x0807CF29u

#define TMC_PROVEN_MOVEMENT           0x09010065u
#define TMC_PROVEN_EXPLORATION        0x0901008Du

#define ADDR_G_SAVE                   0x02002A40u
#define ADDR_G_MESSAGE                0x02000050u
#define ADDR_G_MESSAGE_SCRATCH        0x02000040u
#define ADDR_G_TEXT_GFX_BUFFER        0x02000D00u
#define ADDR_G_TEXT_RENDER            0x02022780u
#define ADDR_G_MESSAGE_CHOICES        0x02024030u
#define ADDR_G_PALETTE_BUFFER         0x020176A0u
#define ADDR_G_USED_PALETTES          0x0200B644u
#define ADDR_G_UI                     0x02032EC0u
#define ADDR_G_AREA                   0x02033A90u
#define ADDR_G_ROOM_VARS              0x02034350u
#define ADDR_G_BG0_BUFFER             0x02034CB0u
#define ADDR_G_CURRENT_WINDOW         0x02036A38u
#define ADDR_G_NEW_WINDOW             0x02036A40u
#define ADDR_G_ROOM_CONTROLS          0x03000BF0u
#define ADDR_G_SCREEN                 0x03000F50u
#define ADDR_G_FADE_CONTROL           0x03000FD0u
#define ADDR_G_INPUT                  0x03000FF0u
#define ADDR_G_MAIN                   0x03001000u
#define ADDR_G_ROOM_TRANSITION        0x030010A0u
#define ADDR_G_PLAYER_ENTITY          0x03001160u
#define ADDR_G_ENTITIES               0x030015A0u
#define ADDR_G_PRIORITY_HANDLER       0x03003DC0u
#define ADDR_G_ENTITY_COUNT           0x03003DBCu
#define ADDR_G_PLAYER_STATE           0x03003F80u
#define ADDR_AREA_ROOM_HEADERS        0x0811E214u

#define ADDR_VRAM_CHARBLOCK3          0x0600C000u
#define ADDR_BG_PALETTE               0x05000000u

#define KEY_A                         0x0001u
#define KEY_B                         0x0002u
#define KEY_SELECT                    0x0004u
#define KEY_START                     0x0008u
#define KEY_RIGHT                     0x0010u
#define KEY_LEFT                      0x0020u
#define KEY_UP                        0x0040u
#define KEY_DOWN                      0x0080u
#define KEY_R                         0x0100u
#define KEY_L                         0x0200u
#define PRACTICE_HOTKEY               (KEY_L | KEY_R | KEY_SELECT)

#define TASK_GAME                     2u
#define GAMETASK_MAIN                 2u
#define GAMEMAIN_UPDATE               2u
#define GAMEMAIN_SUBTASK              7u
#define PRACTICE_SUBTASK              11u

typedef struct {
    volatile u16 held;
    volatile u16 pressed;
    volatile u16 repeat;
    volatile u8 unknown;
    volatile u8 repeatTimer;
} TmcInput;

typedef struct {
    volatile u8 interruptFlag;
    volatile u8 sleepStatus;
    volatile u8 task;
    volatile u8 state;
    volatile u8 substate;
    volatile u8 field5;
    volatile u8 muteAudio;
    volatile u8 field7;
    volatile u8 pauseFrames;
    volatile u8 pauseCount;
    volatile u8 pauseInterval;
    volatile u8 pad;
    volatile u16 ticks;
} TmcMain;

typedef struct {
    volatile u16 control;
    volatile s16 xOffset;
    volatile s16 yOffset;
    volatile u16 updated;
    volatile void* subTileMap;
} TmcBgSettings;

typedef struct {
    volatile u16 displayControl;
    volatile u16 filler2;
    volatile u16 unknown4;
    volatile u16 displayControlMask;
    TmcBgSettings bg0;
    TmcBgSettings bg1;
    TmcBgSettings bg2;
    TmcBgSettings bg3;
    volatile u8 controls[0x34];
    volatile u8 vblankDma[0x10];
} TmcScreen;

typedef struct {
    u8 walletType;
    u8 heartPieces;
    u8 health;
    u8 maxHealth;
    u8 bombCount;
    u8 arrowCount;
    u8 bombBagType;
    u8 quiverType;
    u8 figurineCount;
    u8 hasAllFigurines0;
    u8 charm;
    u8 picolyteType;
    u8 equipped[2];
    u8 bottles[4];
    u8 effect;
    u8 hasAllFigurines;
    u8 filler14[4];
    u16 rupees;
    u16 shells;
    u16 charmTimer;
    u16 picolyteTimer;
    u16 effectTimer;
    u8 filler22[2];
} TmcStats;

typedef struct {
    u8 prefix[0xA8];
    TmcStats stats;
    u8 fillerCC[2];
    /* Native ROM literal 08088398 = 02002B0E (Save + CE).
     * The decompilation's D0 offset comment is stale; its actual C layout is CE. */
    u8 figurines[0x24];
    u8 inventory[34];
    u8 kinstones[0x148];
    u8 flags[0x200];
    u8 dungeonKeys[0x10];
    u8 dungeonItems[0x10];
    u8 dungeonWarps[0x10];
} TmcSave;

typedef struct {
    volatile u16 reloadFlags;
    volatile u8 scrollAction;
    volatile u8 scrollSubAction;
    volatile u8 area;
    volatile u8 room;
    volatile u16 originX;
    volatile u16 originY;
    volatile u8 rest[0x2E];
} TmcRoomControls;

typedef struct {
    u8 areaNext;
    u8 roomNext;
    u8 startAnimation;
    u8 spawnType;
    s16 startX;
    s16 startY;
    u8 layer;
    u8 filler9;
    u8 remainder[0x16];
} TmcRoomStatus;

typedef struct {
    volatile u32 frameCount;
    volatile u8 field4[4];
    volatile u8 transitioningOut;
    volatile u8 type;
    volatile u16 stairsIndex;
    TmcRoomStatus playerStatus;
    volatile u8 rest[0x84];
} TmcRoomTransition;

typedef struct {
    u16 mapX;
    u16 mapY;
    u16 width;
    u16 height;
    u16 tileSet;
} __attribute__((packed, aligned(2))) TmcRoomHeader;

#define TMC_INPUT           (*(volatile TmcInput*)ADDR_G_INPUT)
#define TMC_MAIN            (*(volatile TmcMain*)ADDR_G_MAIN)
#define TMC_SCREEN          (*(volatile TmcScreen*)ADDR_G_SCREEN)
#define TMC_SAVE            (*(volatile TmcSave*)ADDR_G_SAVE)
#define TMC_ROOM            (*(volatile TmcRoomControls*)ADDR_G_ROOM_CONTROLS)
#define TMC_TRANSITION      (*(volatile TmcRoomTransition*)ADDR_G_ROOM_TRANSITION)

#define PLAYER8(offset)     (*(volatile u8*)(ADDR_G_PLAYER_ENTITY + (offset)))
#define PLAYER16(offset)    (*(volatile u16*)(ADDR_G_PLAYER_ENTITY + (offset)))
#define PLAYER32(offset)    (*(volatile u32*)(ADDR_G_PLAYER_ENTITY + (offset)))
#define PSTATE8(offset)     (*(volatile u8*)(ADDR_G_PLAYER_STATE + (offset)))
#define PSTATE16(offset)    (*(volatile u16*)(ADDR_G_PLAYER_STATE + (offset)))
#define PSTATE32(offset)    (*(volatile u32*)(ADDR_G_PLAYER_STATE + (offset)))

#define CALL_VOID0(address) ((void (*)(void))(uptr)(address))
#define CALL_VOID1(address) ((void (*)(void*))(uptr)(address))
#define CALL_U32_0(address) ((u32 (*)(void))(uptr)(address))
#define CALL_U32_1(address) ((u32 (*)(void*))(uptr)(address))
#define CALL_VOID2(address) ((void (*)(u32, u32))(uptr)(address))
#define CALL_VOID_U32(address) ((void (*)(u32))(uptr)(address))
#define CALL_U32_2(address)  ((u32 (*)(u32, void*))(uptr)(address))

#endif
