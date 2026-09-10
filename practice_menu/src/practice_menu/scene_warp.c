#include "practice.h"

/* A replay owns a separate full native save snapshot, never the 100% undo.
 * The linker bounds this plus ModalBackup below the existing Practice ABI. */
typedef struct {
    u8 save[FULL_SAVE_SIZE];
    PracticePosition origin;
    u8 cheats[7], lockDirection, timerRunning, debugHud;
} SceneBackup;
__attribute__((section(".modal"),aligned(4))) SceneBackup gPracticeSceneBackup;

static void flag(u32 bit,u32 value) {
    volatile u8* p=&TMC_SAVE.flags[bit>>3];u8 mask=1u<<(bit&7);
    *p=value?(*p|mask):(*p&~mask);
}

static void load_room(u32 area,u32 room,u32 x,u32 y,u32 layer) {
    /* Native GameTask_Init, not a synthetic room driver. Discard only the old
     * transition; its frame counter remains monotonic for scripts/audio. */
    Practice_ClearBytes((void*)&TMC_TRANSITION.field4,sizeof(TmcRoomTransition)-4);
    TMC_TRANSITION.type=5;
    TMC_TRANSITION.playerStatus.areaNext=area;
    TMC_TRANSITION.playerStatus.roomNext=room;
    TMC_TRANSITION.playerStatus.startAnimation=4;
    TMC_TRANSITION.playerStatus.startX=x;TMC_TRANSITION.playerStatus.startY=y;
    TMC_TRANSITION.playerStatus.layer=layer?layer:1;
    TMC_MAIN.task=TASK_GAME;TMC_MAIN.state=1;TMC_MAIN.substate=0;
    gPracticeState.pausedMessageValid=gPracticeState.pausedMessageRestorePending=0;
    gPracticeState.contextRestorePending=gPracticeState.sceneSkipTicks=0;
    gPracticeState.breakFreeTicks=0;
}

void SceneWarp_Start(u32 ending) {
    SceneBackup* b=&gPracticeSceneBackup;PracticePosition* p=&b->origin;
    if(gPracticeState.sceneReplay) return;
    Practice_CopyBytes((void*)ADDR_G_SAVE,b->save,FULL_SAVE_SIZE);
    p->area=gPracticeState.menuArea;p->room=gPracticeState.menuRoom;
    p->originX=gPracticeState.menuOriginX;p->originY=gPracticeState.menuOriginY;
    p->x=gPracticeState.menuX;p->y=gPracticeState.menuY;p->z=gPracticeState.menuZ;
    p->layer=gPracticeState.menuCollisionLayer;
    p->direction=gPracticeState.menuDirection;p->animationState=gPracticeState.menuAnimationState;
    Practice_CopyBytes(&gPracticeState.noClip,b->cheats,7);
    b->lockDirection=gPracticeState.lockDirection;b->timerRunning=gPracticeState.timerRunning;
    b->debugHud=gPracticeState.debugHud;
    Practice_ClearBytes(&gPracticeState.noClip,7);
    gPracticeState.lockDirection=gPracticeState.timerRunning=gPracticeState.debugHud=0;
    Practice_ClearBytes(gPracticeState.actorMarks,sizeof(gPracticeState.actorMarks));
    gPracticeState.sceneReplay=ending?2:1;
    gPracticeState.sceneSeen=gPracticeState.sceneReturn=gPracticeState.sceneConfirm=0;
    gPracticeState.page=PAGE_WARP;gPracticeState.cursor[PAGE_WARP]=7;
    if(ending) {
        /* Native Garden RoomInit: ENDING and first-visit branch, bank3. */
        flag(0x51,1);flag(0x37F,0);
        load_room(0x89,1,136,168,1);
    } else {
        /* Native bedroom RoomInit starts the storybook and player wake-up
         * scripts itself. HouseInteriors2 uses local bank2 (0x200). */
        flag(0x13,0);flag(0x246,0);
        load_room(0x22,0x15,88,40,1);
    }
}

void SceneWarp_Return(void) {
    if(!gPracticeState.sceneReplay) {
        PracticeRuntime_SetStatus("NO ACTIVE STORY REPLAY");return;
    }
    gPracticeState.sceneReturn=1;
    PracticeRuntime_CloseMenu();
}

u32 SceneWarp_Tick(void) {
    SceneBackup* b=&gPracticeSceneBackup;PracticePosition* p=&b->origin;
    u32 mode=gPracticeState.sceneReplay;
    u32 game=TMC_MAIN.task==TASK_GAME && TMC_MAIN.state==GAMETASK_MAIN;
    u32 ready=game && TMC_MAIN.substate==GAMEMAIN_UPDATE &&
        !*(volatile u8*)ADDR_G_FADE_CONTROL;
    if(!mode || gPracticeState.menuOpen || gPracticeState.modalMode) return 0;
    if(mode==3) {
        if(ready) {
            /* Room initialization can set visit/map flags; these are not part
             * of the user's replay either. Restore the snapshot once more. */
            Practice_CopyBytes(b->save,(void*)ADDR_G_SAVE,FULL_SAVE_SIZE);
            Practice_CopyBytes(b->cheats,&gPracticeState.noClip,7);
            gPracticeState.invincibilityApplied=0;
            gPracticeState.lockDirection=b->lockDirection;
            gPracticeState.timerRunning=b->timerRunning;gPracticeState.debugHud=b->debugHud;
            PLAYER32(0x2C)=p->x;PLAYER32(0x30)=p->y;PLAYER32(0x34)=p->z;
            PLAYER8(0x15)=p->direction;PLAYER8(0x14)=p->animationState;
            gPracticeState.sceneReplay=gPracticeState.sceneReturn=gPracticeState.sceneSeen=0;
            PracticeRuntime_SetStatus("REPLAY ENDED - SAVE RESTORED");
            return 1;
        }
        return 0;
    }
    if(mode==1 && game && TMC_MAIN.substate==GAMEMAIN_SUBTASK &&
       *(volatile u8*)(ADDR_G_UI+2)==5) gPracticeState.sceneSeen=1;
    if(mode==1 && gPracticeState.sceneSeen && ready && !PSTATE8(0x8B) &&
       !*(volatile u8*)0x02034490u) gPracticeState.sceneReturn=1;
    /* Stop a replay before the native ending-save prompt; normal endings are
     * untouched. No replay ever invokes the EEPROM save path. */
    if(mode==2 && TMC_MAIN.task==4 && TMC_MAIN.state==2) {
        gPracticeState.sceneReturn=1;
        /* FadeMain lives outside this task wrapper and keeps advancing. Do
         * not let Staffroll_State2 prepare/accept its save prompt meanwhile. */
        if(*(volatile u8*)ADDR_G_FADE_CONTROL) return 1;
    }
    if(gPracticeState.sceneReturn && !*(volatile u8*)ADDR_G_FADE_CONTROL &&
       !(game && TMC_MAIN.substate==GAMEMAIN_SUBTASK && *(volatile u8*)(ADDR_G_UI+2)==PRACTICE_SUBTASK)) {
        Practice_CopyBytes(b->save,(void*)ADDR_G_SAVE,FULL_SAVE_SIZE);
        gPracticeState.sceneReplay=3;
        load_room(p->area,p->room,(p->x>>16)-p->originX,(p->y>>16)-p->originY,p->layer);
        return 1;
    }
    return 0;
}
