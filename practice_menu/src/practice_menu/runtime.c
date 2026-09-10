#include "practice.h"

PracticeState gPracticeState;

static void bytes_clear(void* destination, u32 size) {
    u8* out = (u8*)destination;
    while (size-- != 0) {
        *out++ = 0;
    }
}

static void bytes_copy(const void* source, void* destination, u32 size) {
    const u8* in = (const u8*)source;
    u8* out = (u8*)destination;
    while (size-- != 0) {
        *out++ = *in++;
    }
}

static u16 local_coordinate(s32 fixed, u16 origin) {
    s32 coordinate = (fixed >> 16) - origin;
    if (coordinate < 0) {
        return 0;
    }
    if (coordinate > 0x7FFF) {
        return 0x7FFF;
    }
    return (u16)coordinate;
}

static void begin_transition(u8 area, u8 room, u16 x, u16 y, u8 layer) {
    volatile TmcRoomTransition* transition = &TMC_TRANSITION;
    transition->transitioningOut = 1;
    transition->type = 5;
    transition->playerStatus.areaNext = area;
    transition->playerStatus.roomNext = room;
    transition->playerStatus.startAnimation = 4;
    transition->playerStatus.spawnType = 0;
    transition->playerStatus.startX = (s16)x;
    transition->playerStatus.startY = (s16)y;
    transition->playerStatus.layer = layer == 0 ? 1 : layer;
}

void PracticeRuntime_Init(void) {
    if (gPracticeState.magic == PRACTICE_MAGIC) {
        return;
    }
    bytes_clear(&gPracticeState, sizeof(gPracticeState));
    gPracticeState.magic = PRACTICE_MAGIC;
    gPracticeState.selectedArea = TMC_ROOM.area;
    gPracticeState.selectedRoom = TMC_ROOM.room;
    gPracticeState.menuHotkey = PRACTICE_HOTKEY;
    gPracticeState.confirmHotkey = KEY_L | KEY_R | KEY_A;
    { u32 i; for(i=0;i<MAX_WARP_FAVORITES;i++) gPracticeState.favorites[i][0]=gPracticeState.favorites[i][1]=0xFF; }
    Settings_Load();
    PracticeRuntime_SetStatus("RUNTIME ONLY - NO AUTO SAVE");
}

void PracticeRuntime_SetStatus(const char* text) {
    u32 index = 0;
    while (index < 28 && text[index] != '\0') {
        gPracticeState.status[index] = text[index];
        index++;
    }
    gPracticeState.status[index] = '\0';
    while (++index < sizeof(gPracticeState.status)) {
        gPracticeState.status[index] = '\0';
    }
    gPracticeState.statusTimer = 180;
}

void PracticeRuntime_CaptureOpenContext(void) {
    PracticeRuntime_Init();
    gPracticeState.sceneSkipTicks=0;
    Movement_ReleaseCamera();
    if (gPracticeState.cameraMode == 2) gPracticeState.cameraMode = 0;
    PracticeHud_BeforeGame();
    bytes_copy((const void*)ADDR_G_PRIORITY_HANDLER,gPracticeState.pausedPriority,10);
    gPracticeState.pausedPlayerPriority = PLAYER8(0x11);
    gPracticeState.pausedPauseDisabled = *(volatile u8*)0x02034490u;
    gPracticeState.contextRestorePending = 1;
    if (gPracticeState.page >= MAX_MENU_PAGES) {
        gPracticeState.page = PAGE_ROOT;
    }
    gPracticeState.pausedMessageValid = 0;
    gPracticeState.pausedMessageRestorePending = 0;
    if ((*(volatile u8*)ADDR_G_MESSAGE & 0x7Fu) != 0) {
        bytes_copy((const void*)ADDR_G_MESSAGE, gPracticeState.pausedMessage,
                   sizeof(gPracticeState.pausedMessage));
        bytes_copy((const void*)ADDR_G_TEXT_RENDER, gPracticeState.pausedTextRender,
                   sizeof(gPracticeState.pausedTextRender));
        bytes_copy((const void*)ADDR_G_MESSAGE_CHOICES, gPracticeState.pausedMessageChoices,
                   sizeof(gPracticeState.pausedMessageChoices));
        bytes_copy((const void*)ADDR_G_MESSAGE_SCRATCH, gPracticeState.pausedMessageScratch,
                   sizeof(gPracticeState.pausedMessageScratch));
        bytes_copy((const void*)ADDR_G_CURRENT_WINDOW, gPracticeState.pausedCurrentWindow,
                   sizeof(gPracticeState.pausedCurrentWindow));
        bytes_copy((const void*)ADDR_G_NEW_WINDOW, gPracticeState.pausedNewWindow,
                   sizeof(gPracticeState.pausedNewWindow));
        bytes_copy((const void*)ADDR_G_TEXT_GFX_BUFFER, gPracticeState.pausedTextGfx,
                   sizeof(gPracticeState.pausedTextGfx));
        gPracticeState.pausedMessageValid = 1;
    }
    gPracticeState.menuArea = TMC_ROOM.area;
    gPracticeState.menuRoom = TMC_ROOM.room;
    gPracticeState.menuOriginX = TMC_ROOM.originX;
    gPracticeState.menuOriginY = TMC_ROOM.originY;
    gPracticeState.menuDungeonIndex = *(volatile u8*)(ADDR_G_AREA + 3);
    gPracticeState.menuLocalFlagOffset = *(volatile u16*)(ADDR_G_AREA + 4);
    gPracticeState.menuCollisionLayer = PLAYER8(0x38);
    gPracticeState.menuX = (s32)PLAYER32(0x2C);
    gPracticeState.menuY = (s32)PLAYER32(0x30);
    gPracticeState.menuZ = (s32)PLAYER32(0x34);
    gPracticeState.menuHeldInput = TMC_INPUT.held;
    gPracticeState.menuSpeed = PLAYER16(0x24);
    gPracticeState.menuDirection = PLAYER8(0x15);
    gPracticeState.menuAnimationState = PLAYER8(0x14);
    gPracticeState.menuAction = PLAYER8(0x0C);
    gPracticeState.menuFrameState = PSTATE8(0xA8);
    gPracticeState.menuFloorType = PSTATE8(0x12);
    gPracticeState.menuEntityCount = *(volatile u8*)ADDR_G_ENTITY_COUNT;
}

u32 PracticeRuntime_CanOpenMenu(void) {
    return (PLAYER8(0x08) == 1 || gPracticeState.actorExpert) && TMC_SAVE.stats.health != 0 &&
           TMC_TRANSITION.transitioningOut == 0 &&
           *(volatile u8*)ADDR_G_FADE_CONTROL == 0;
}

void PracticeRuntime_CloseMenu(void) {
    gPracticeState.menuOpen = 0;
    if (gPracticeState.modalMode) { gPracticeState.modalMode=2; return; }
    if (gPracticeState.pausedMessageValid) {
        if (gPracticeState.pendingAction == PENDING_NONE || gPracticeState.pendingAction == PENDING_BREAK_FREE ||
            gPracticeState.pendingAction == PENDING_ACTOR_SPAWN || gPracticeState.pendingAction == PENDING_ACTOR_DELETE ||
            gPracticeState.pendingAction == PENDING_LINK_TO_ACTOR || gPracticeState.pendingAction == PENDING_ACTOR_TO_LINK) {
            gPracticeState.pausedMessageRestorePending = 1;
        } else {
            gPracticeState.pausedMessageValid = 0;
            gPracticeState.pausedMessageRestorePending = 0;
        }
    }
    CALL_VOID0(TMC_SUBTASK_EXIT)();
}

void PracticeRuntime_RestorePausedMessage(void) {
    if (gPracticeState.contextRestorePending) {
        bytes_copy(gPracticeState.pausedPriority,(void*)ADDR_G_PRIORITY_HANDLER,10);
        PLAYER8(0x11) = gPracticeState.pausedPlayerPriority;
        *(volatile u8*)0x02034490u = gPracticeState.pausedPauseDisabled;
        gPracticeState.contextRestorePending = 0;
    }
    if (!gPracticeState.pausedMessageRestorePending ||
        !gPracticeState.pausedMessageValid) return;
    bytes_copy(gPracticeState.pausedMessage, (void*)ADDR_G_MESSAGE,
               sizeof(gPracticeState.pausedMessage));
    bytes_copy(gPracticeState.pausedTextRender, (void*)ADDR_G_TEXT_RENDER,
               sizeof(gPracticeState.pausedTextRender));
    bytes_copy(gPracticeState.pausedMessageChoices, (void*)ADDR_G_MESSAGE_CHOICES,
               sizeof(gPracticeState.pausedMessageChoices));
    bytes_copy(gPracticeState.pausedMessageScratch, (void*)ADDR_G_MESSAGE_SCRATCH,
               sizeof(gPracticeState.pausedMessageScratch));
    bytes_copy(gPracticeState.pausedCurrentWindow, (void*)ADDR_G_CURRENT_WINDOW,
               sizeof(gPracticeState.pausedCurrentWindow));
    bytes_copy(gPracticeState.pausedNewWindow, (void*)ADDR_G_NEW_WINDOW,
               sizeof(gPracticeState.pausedNewWindow));
    bytes_copy(gPracticeState.pausedTextGfx, (void*)ADDR_G_TEXT_GFX_BUFFER,
               sizeof(gPracticeState.pausedTextGfx));
    *(volatile u8*)(ADDR_G_TEXT_RENDER + 0x9Du) = 1;
    gPracticeState.pausedMessageValid = 0;
    gPracticeState.pausedMessageRestorePending = 0;
}

void PracticeRuntime_Queue(PendingAction action) {
    if (gPracticeState.sceneReplay) {
        PracticeRuntime_SetStatus("REPLAY - RETURN IN WORLD"); return;
    }
    if (gPracticeState.modalMode) {
        PracticeRuntime_SetStatus("USE FROM ACTIVE GAMEPLAY"); return;
    }
    /* A menu can inspect a paused conversation/scene, but leaving that scene
     * through a queued player or room mutation is not a supported recovery. */
    if (action != PENDING_BREAK_FREE && action != PENDING_ACTOR_SPAWN && action != PENDING_ACTOR_DELETE &&
        action != PENDING_LINK_TO_ACTOR && action != PENDING_ACTOR_TO_LINK &&
        (gPracticeState.pausedMessageValid || PSTATE8(0x8B))) {
        PracticeRuntime_SetStatus("ACTION BLOCKED DURING SCENE"); return;
    }
    gPracticeState.pendingAction = (u8)action;
    PracticeRuntime_CloseMenu();
}

static void restore_position_now(const PracticePosition* position) {
    PLAYER32(0x2C) = (u32)position->x;
    PLAYER32(0x30) = (u32)position->y;
    PLAYER32(0x34) = (u32)position->z;
    PLAYER32(0x20) = 0;
    PLAYER16(0x24) = 0;
    PLAYER16(0x2A) = 0;
    PLAYER8(0x0C) = 1;
    PLAYER8(0x0D) = 0;
    PLAYER8(0x14) = position->animationState;
    PLAYER8(0x15) = position->direction;
    PLAYER8(0x38) = position->layer == 0 ? 1 : position->layer;
    PSTATE8(0x02) = 0;
    PSTATE8(0x0C) = 0;
    PSTATE8(0x3C) = 0;
}

static void transition_to_position(const PracticePosition* position) {
    begin_transition(position->area, position->room,
                     local_coordinate(position->x, position->originX),
                     local_coordinate(position->y, position->originY), position->layer);
}

void PracticeRuntime_ApplyPending(void) {
    PendingAction action = (PendingAction)gPracticeState.pendingAction;
    const TmcRoomHeader* header;
    const WarpRoomInfo* info;
    u16 x;
    u16 y;
    if (action == PENDING_NONE) {
        return;
    }
    gPracticeState.pendingAction = PENDING_NONE;
    switch (action) {
        case PENDING_STORY_INTRO: SceneWarp_Start(0); break;
        case PENDING_STORY_ENDING: SceneWarp_Start(1); break;
        case PENDING_ACTOR_SPAWN:
        case PENDING_LINK_TO_ACTOR:
        case PENDING_ACTOR_TO_LINK:
        case PENDING_ACTOR_DELETE: Actors_Apply(action); break;
        case PENDING_NUDGE: Movement_ApplyNudge(); break;
        case PENDING_GROUND_RESET: Movement_GroundReset(); break;
        case PENDING_BREAK_FREE: Cheats_BreakFree(); break;
        case PENDING_OCARINA_GLITCH: Cheats_OcarinaGlitch(); break;
        case PENDING_COMPLETE_GAME:
            Completion_Capture();
            Completion_Apply();
            begin_transition(gPracticeState.menuArea,gPracticeState.menuRoom,
                local_coordinate(gPracticeState.menuX,gPracticeState.menuOriginX),
                local_coordinate(gPracticeState.menuY,gPracticeState.menuOriginY),gPracticeState.menuCollisionLayer);
            TMC_MAIN.state=1; TMC_MAIN.substate=0;
            break;
        case PENDING_UNDO_COMPLETE:
            if (Completion_CanUndo()) {
                Completion_Restore();
                transition_to_position(&gPracticeState.completionReturn);
                TMC_MAIN.state=1; TMC_MAIN.substate=0;
            } else PracticeRuntime_SetStatus("NO VALID 100% BACKUP");
            break;
        case PENDING_BETA_ENTER:
            begin_transition(0x24,0x1F,120,112,1);
            break;
        case PENDING_BETA_RETURN:
            if (gPracticeState.betaReturn.valid) {
                transition_to_position(&gPracticeState.betaReturn);
                gPracticeState.betaReturn.valid=0;
            }
            break;
        case PENDING_LOAD_POSITION:
            if (!gPracticeState.savedPosition.valid) {
                return;
            }
            if (gPracticeState.savedPosition.area == TMC_ROOM.area &&
                gPracticeState.savedPosition.room == TMC_ROOM.room) {
                restore_position_now(&gPracticeState.savedPosition);
            } else {
                transition_to_position(&gPracticeState.savedPosition);
            }
            break;
        case PENDING_APPLY_PRESET:
            if (!gPracticeState.preset.valid) {
                return;
            }
            if (gPracticeState.preset.position.area == TMC_ROOM.area &&
                gPracticeState.preset.position.room == TMC_ROOM.room) {
                restore_position_now(&gPracticeState.preset.position);
            } else {
                transition_to_position(&gPracticeState.preset.position);
            }
            break;
        case PENDING_RELOAD_ROOM:
            begin_transition(gPracticeState.menuArea, gPracticeState.menuRoom,
                             local_coordinate(gPracticeState.menuX, gPracticeState.menuOriginX),
                             local_coordinate(gPracticeState.menuY, gPracticeState.menuOriginY),
                             PLAYER8(0x38));
            break;
        case PENDING_RELOAD_AREA:
            begin_transition(gPracticeState.menuArea, gPracticeState.menuRoom,
                             local_coordinate(gPracticeState.menuX, gPracticeState.menuOriginX),
                             local_coordinate(gPracticeState.menuY, gPracticeState.menuOriginY),
                             PLAYER8(0x38));
            TMC_MAIN.state = 1;
            TMC_MAIN.substate = 0;
            break;
        case PENDING_WARP:
            header = Warp_GetHeader(gPracticeState.selectedArea, gPracticeState.selectedRoom);
            if (header == 0) {
                return;
            }
            x = header->width > 32 ? (u16)(header->width >> 1) : 8;
            y = header->height > 32 ? (u16)(header->height >> 1) : 8;
            info = Warp_RoomInfo(gPracticeState.selectedArea,gPracticeState.selectedRoom);
            if (info && info->layer && info->x < header->width && info->y < header->height)
                begin_transition(gPracticeState.selectedArea,gPracticeState.selectedRoom,info->x,info->y,info->layer);
            else begin_transition(gPracticeState.selectedArea, gPracticeState.selectedRoom, x, y, 1);
            break;
        default:
            break;
    }
}

void PracticeRuntime_BeforeGame(void) {
    Cheats_BreakFreeUpdate();
    Inventory_Refresh();
    Cheats_RefillResources();
    PracticeHud_BeforeGame();
    if (gPracticeState.invincibility) {
        PLAYER8(0x3D) = 0xFE;
        gPracticeState.invincibilityApplied = 1;
    } else if (gPracticeState.invincibilityApplied) {
        if (PLAYER8(0x3D) == 0xFE) {
            PLAYER8(0x3D) = 0;
        }
        gPracticeState.invincibilityApplied = 0;
    }
    Movement_BeforeGame();
    Enemies_BeforeGame();
}

void PracticeRuntime_AfterGame(void) {
    Movement_AfterGame();
    Cheats_RefillResources();
    gPracticeState.frameCounter++;
    if (gPracticeState.timerRunning) {
        gPracticeState.timerFrames++;
    }
    if (gPracticeState.statusTimer != 0) {
        gPracticeState.statusTimer--;
    }
    Enemies_AfterGame();
    PracticeHud_AfterGame();
}

void Player_FullHealth(void) {
    TMC_SAVE.stats.health = TMC_SAVE.stats.maxHealth;
    PLAYER8(0x45) = TMC_SAVE.stats.maxHealth;
    PracticeRuntime_SetStatus("FULL HEALTH");
}

void Player_AdjustMaxHealth(s32 direction) {
    s32 value = TMC_SAVE.stats.maxHealth;
    value += direction * 8;
    if (value < 24) value = 24;
    if (value > 160) value = 160;
    TMC_SAVE.stats.maxHealth = (u8)value;
    if (TMC_SAVE.stats.health > value) TMC_SAVE.stats.health = (u8)value;
    PracticeRuntime_SetStatus("MAX HEARTS CHANGED");
}

void Player_AdjustRupees(s32 amount) {
    static const u16 walletSizes[4] = { 100, 300, 500, 999 };
    s32 value = TMC_SAVE.stats.rupees + amount;
    u32 wallet = TMC_SAVE.stats.walletType;
    if (wallet > 3) wallet = 3;
    if (value < 0) value = 0;
    if (value > walletSizes[wallet]) value = walletSizes[wallet];
    TMC_SAVE.stats.rupees = (u16)value;
}

void Practice_CopyBytes(const void* source, void* destination, u32 size) {
    bytes_copy(source, destination, size);
}

void Practice_ClearBytes(void* destination, u32 size) {
    bytes_clear(destination, size);
}
