#include "practice.h"

__attribute__((section(".entry")))
void GameTaskWrapper(void) {
    u32 normalBefore;
    PracticeRuntime_Init();
    if (PracticeModal_Tick()) return;
    if (SceneWarp_Tick()) return;
    Actors_Context();
    /* State 0 is file-select -> game, not an ordinary area reload (state 1).
     * Never carry an undo snapshot into a freshly loaded save, even same slot. */
    if (TMC_MAIN.state==0 || TMC_MAIN.state==3) gPracticeState.completionUndoValid=0;
    normalBefore = TMC_MAIN.task == TASK_GAME && TMC_MAIN.state == GAMETASK_MAIN &&
                   TMC_MAIN.substate == GAMEMAIN_UPDATE;
    /* Consume the complete hotkey before native R/L handling can pick up an
     * object, trigger a hint, or advance a conversation on the opening frame. */
    if (normalBefore && (TMC_INPUT.held & gPracticeState.menuHotkey) == gPracticeState.menuHotkey &&
        (TMC_INPUT.pressed & gPracticeState.menuHotkey) != 0 && PracticeRuntime_CanOpenMenu()) {
        PracticeRuntime_CaptureOpenContext();
        TMC_INPUT.held = TMC_INPUT.pressed = TMC_INPUT.repeat = 0;
        CALL_VOID2(TMC_MENU_FADE_IN)(PRACTICE_SUBTASK,0);
        normalBefore = 0;
    }
    if (normalBefore) {
        PracticeRuntime_BeforeGame();
    }
    CALL_VOID0(TMC_GAME_TASK)();
    if (TMC_MAIN.task == TASK_GAME && TMC_MAIN.state == GAMETASK_MAIN &&
        TMC_MAIN.substate == GAMEMAIN_UPDATE) {
        PracticeRuntime_RestorePausedMessage();
        PracticeRuntime_AfterGame();
        PracticeRuntime_ApplyPending();
    }
    SceneSkip_Update();
}

void PracticeDebugTask(void) {
    TMC_MAIN.task = 0;
    TMC_MAIN.state = 0;
    TMC_MAIN.substate = 0;
}

void MovementDispatch(void* entity) {
    u16 speed = PLAYER16(0x24);
    u32 adjusted = entity == (void*)ADDR_G_PLAYER_ENTITY && Movement_IsControlled();
    if (adjusted && gPracticeState.speedMode) {
        PLAYER16(0x24) = gPracticeState.speedMode == 1 ? speed + speed / 2 : speed * 2;
    }
    if (gPracticeState.noClip) {
        CALL_VOID1(TMC_PROVEN_MOVEMENT)(entity);
    } else {
        CALL_VOID1(TMC_ORIGINAL_MOVEMENT)(entity);
    }
    if (adjusted) PLAYER16(0x24) = speed;
}

void ExplorationDispatch(u32 direction) {
    if (gPracticeState.noClip) {
        ((void (*)(u32))(uptr)TMC_PROVEN_EXPLORATION)(direction);
    } else {
        ((void (*)(u32))(uptr)TMC_ORIGINAL_EXPLORATION)(direction);
    }
}

void RespawnGuardDispatch(void* entity) {
    if (!gPracticeState.noClip) {
        CALL_VOID1(TMC_ORIGINAL_RESPAWN_GUARD)(entity);
    }
}

void ConveyorDispatch(void* entity) {
    volatile u8* raw = (volatile u8*)entity;
    if (gPracticeState.noClip) {
        return;
    }
    CALL_VOID1(TMC_RESET_ACTIVE_ITEMS)(entity);
    raw[0x29] &= 0xC7;
    *(volatile u16*)(raw + 0x24) = 0x140;
    PSTATE32(0x30) |= 0x02000000u;
    PSTATE8(0x0A) |= 0x80;
    PSTATE8(0x1A) |= 0x80;
    PSTATE8(0x27)++;
    CALL_VOID1(TMC_LINEAR_MOVE_UPDATE)(entity);
}

void PitDispatch(void* entity) {
    volatile u8* raw = (volatile u8*)entity;
    if (gPracticeState.noClip) {
        return;
    }
    if (CALL_U32_0(TMC_PIT_GUARD)() == 0 && CALL_U32_1(TMC_PIT_COLLISION_TEST)(entity) != 0) {
        if (raw[0x0C] != 3) {
            CALL_VOID1(TMC_RESET_ACTIVE_ITEMS)(entity);
            PSTATE8(0x0C) = 3;
        }
    }
}

void SurfaceMinishFrontDispatch(void* entity) {
    if (!gPracticeState.noClip) {
        CALL_VOID1(TMC_SURFACE_MINISH_FRONT)(entity);
    }
}

void Surface21Dispatch(void* entity) {
    if (!gPracticeState.noClip) {
        CALL_VOID1(TMC_SURFACE_21)(entity);
    }
}

const uptr gPracticeSubtasks[12] = {
    0x080A71DDu, 0x080A4EA1u, 0x080A71DDu, 0x080A64FDu,
    0x080A3B85u, 0x08051E69u, 0x0804AB55u, 0x080A45A5u,
    0x08054871u, 0x080A6C75u, 0x080A6AB9u, (uptr)&PracticeMenu_Update,
};
