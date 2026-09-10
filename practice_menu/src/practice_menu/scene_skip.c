#include "practice.h"

static u32 active_scene(void) {
    return PSTATE8(0x8B) || (*(volatile u8*)ADDR_G_MESSAGE&0x7F);
}
static u32 choice_active(void) {
    return *(volatile u8*)(ADDR_G_MESSAGE_CHOICES+2) ||
        *(volatile u8*)(ADDR_G_TEXT_RENDER+0x89)==5;
}
void SceneSkip_Start(void) {
    if (!gPracticeState.pausedMessageValid && !active_scene()) {
        PracticeRuntime_SetStatus("NO ACTIVE SCENE OR DIALOG"); return;
    }
    if (choice_active() || (gPracticeState.pausedMessageValid && gPracticeState.pausedMessageChoices[2])) {
        PracticeRuntime_SetStatus("CHOOSE DIALOG ANSWER FIRST"); return;
    }
    gPracticeState.sceneSkipTicks=3601; /* Arm only after native menu fade-out. */
    PracticeRuntime_CloseMenu(); /* Restore paused text, do not delete it. */
}

/* Bounded fast-skip through native logic, NOT a script-PC jump or flag preset.
 * Additional main-loop game/message/fade ticks with no player input.
 * Choices and transitions stop the skip. No synthetic A reaches GameTask.
 * Native calls confirmed at Main loop 08055F54/58/5C in the USA ROM. */
void SceneSkip_Update(void) {
    u32 i;
    u16 held=TMC_INPUT.held,pressed=TMC_INPUT.pressed,repeat=TMC_INPUT.repeat;
    if (!gPracticeState.sceneSkipTicks) return;
    if (pressed&KEY_B) { gPracticeState.sceneSkipTicks=0; PracticeRuntime_SetStatus("SCENE SKIP CANCELLED"); return; }
    if (gPracticeState.sceneSkipTicks==3601) {
        if (gPracticeState.menuOpen || TMC_MAIN.task!=TASK_GAME || TMC_MAIN.state!=GAMETASK_MAIN ||
            TMC_MAIN.substate!=GAMEMAIN_UPDATE || *(volatile u8*)0x03000FD0u) return;
        gPracticeState.sceneSkipTicks=3600;
    }
    for(i=0;i<7 && gPracticeState.sceneSkipTicks;i++) {
        const char* stop=0;
        if (gPracticeState.menuOpen || TMC_MAIN.task!=TASK_GAME || TMC_MAIN.state!=GAMETASK_MAIN ||
            TMC_MAIN.substate!=GAMEMAIN_UPDATE || TMC_TRANSITION.transitioningOut ||
            *(volatile u8*)0x03000FD0u) stop="SKIP STOPPED AT TRANSITION";
        else if (choice_active()) stop="SKIP STOPPED: DIALOG CHOICE";
        else if (!active_scene()) stop="SCENE SKIP FINISHED";
        if (stop) { gPracticeState.sceneSkipTicks=0; PracticeRuntime_SetStatus(stop); break; }
        gPracticeState.sceneSkipTicks--;
        TMC_INPUT.held=TMC_INPUT.pressed=TMC_INPUT.repeat=0;
        (*(volatile u16*)0x0300100Cu)++;
        PracticeRuntime_BeforeGame();
        CALL_VOID0(TMC_GAME_TASK)();
        /* No auto-answer, including a choice newly created by this tick. */
        if (!choice_active()) {
            u8 render=*(volatile u8*)(ADDR_G_TEXT_RENDER+0x89);
            TMC_INPUT.held=KEY_B; /* Native fast text, not gameplay input. */
            TMC_INPUT.pressed=(render==2 || render==3)?KEY_A:0;
        }
        ((void(*)(void))0x08056459u)();
        ((void(*)(void))0x08050155u)();
        TMC_INPUT.held=TMC_INPUT.pressed=TMC_INPUT.repeat=0;
        if (TMC_MAIN.task==TASK_GAME && TMC_MAIN.state==GAMETASK_MAIN && TMC_MAIN.substate==GAMEMAIN_UPDATE)
            PracticeRuntime_AfterGame();
        if (!gPracticeState.sceneSkipTicks) PracticeRuntime_SetStatus("SKIP TIME LIMIT - RESUMED");
    }
    TMC_INPUT.held=held; TMC_INPUT.pressed=pressed; TMC_INPUT.repeat=repeat;
}
