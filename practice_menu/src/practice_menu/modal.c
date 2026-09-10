#include "practice.h"

/* USA native allocations end at 02038560 (including all 82 sound tracks).
 * This separately bounded scratch section never overlaps the existing Practice
 * ABI at 0203D000. Only display bytes overwritten by this modal are saved.
 * Do NOT use native MenuFadeIn here: it owns a single, non-nestable backup. */
typedef struct {
    u32 font[0x800/4], map[0x800/4], bg0[0x800/4];
    u32 palette[0x80/4], hardwarePalette[0x80/4];
    TmcScreen screen;
    u32 usedPalettes;
    u16 ticks;
    u8 task, subtask;
} ModalBackup;
__attribute__((section(".modal"),aligned(4))) ModalBackup gPracticeModalBackup;
_Static_assert(sizeof(ModalBackup)<=0x2000,"modal scratch exceeds reserved RAM");

static void words(const void* source,void* dest,u32 size) {
    const volatile u32* s=source; volatile u32* d=dest;
    /* GBA VRAM/palette must NOT be written as bytes. */
    for(size>>=2;size;size--) *d++=*s++;
}
static void consume_keys(void) {
    /* Keep held as the native edge detector's history. Clearing it would turn
     * a hotkey held for two frames into a second press and instantly close. */
    TMC_INPUT.pressed=TMC_INPUT.repeat=0;
}

static u32 can_open(void) {
    if(gPracticeState.menuOpen || *(volatile u8*)ADDR_G_FADE_CONTROL ||
       (TMC_SCREEN.displayControl&7)>1 || *(volatile u8*)0x03003DE0u ||
       *(volatile u8*)0x02000070u) return 0;
    if(TMC_MAIN.task==0) return TMC_MAIN.state==1; /* initialized logo/title */
    if(TMC_MAIN.task==4) {
        /* Never suspend the native save write itself. The preceding confirmation
         * and the final end screen are safe to inspect. */
        return TMC_MAIN.state!=0 && !(TMC_MAIN.state==2 && *(volatile u8*)0x02000086u==2);
    }
    if(TMC_MAIN.task==TASK_GAME && *(volatile u8*)(ADDR_G_UI+2)==1) {
        u32 screen=*(volatile u8*)0x02034491u;
        if((screen==10 || screen==11) && *(volatile u8*)0x02000085u==2) return 0;
    }
    return TMC_MAIN.task==TASK_GAME && TMC_MAIN.state==GAMETASK_MAIN &&
        TMC_MAIN.substate==GAMEMAIN_SUBTASK && *(volatile u8*)ADDR_G_UI==2 &&
        *(volatile u8*)(ADDR_G_UI+2)!=PRACTICE_SUBTASK;
}

static void capture_context(void) {
    volatile TmcRoomControls* room=&TMC_ROOM;
    if(TMC_MAIN.task==TASK_GAME && gPracticeModalBackup.subtask==1)
        room=(volatile TmcRoomControls*)(ADDR_G_UI+0x1C);
    gPracticeState.modalContext=TMC_MAIN.task==0?2:(TMC_MAIN.task==4?3:1);
    gPracticeState.modalSaveEdits=TMC_MAIN.task==TASK_GAME &&
        gPracticeModalBackup.subtask==1 && PLAYER8(8)==1 && TMC_SAVE.stats.maxHealth!=0;
    gPracticeState.menuArea=room->area;gPracticeState.menuRoom=room->room;
    gPracticeState.menuOriginX=room->originX;gPracticeState.menuOriginY=room->originY;
    gPracticeState.menuDungeonIndex=*(volatile u8*)(ADDR_G_AREA+3);
    gPracticeState.menuLocalFlagOffset=*(volatile u16*)(ADDR_G_AREA+4);
    gPracticeState.menuCollisionLayer=PLAYER8(0x38);
    gPracticeState.menuX=PLAYER32(0x2C);gPracticeState.menuY=PLAYER32(0x30);gPracticeState.menuZ=PLAYER32(0x34);
    gPracticeState.menuAction=PLAYER8(0x0C);gPracticeState.menuFrameState=PSTATE8(0xA8);
    gPracticeState.menuFloorType=PSTATE8(0x12);gPracticeState.menuEntityCount=*(volatile u8*)ADDR_G_ENTITY_COUNT;
    gPracticeState.menuSpeed=PLAYER16(0x24);gPracticeState.menuDirection=PLAYER8(0x15);
    gPracticeState.menuAnimationState=PLAYER8(0x14);gPracticeState.menuHeldInput=TMC_INPUT.held;
    gPracticeState.sceneSkipTicks=0;
}

static void open_modal(void) {
    ModalBackup* b=&gPracticeModalBackup;
    b->task=TMC_MAIN.task;b->subtask=*(volatile u8*)(ADDR_G_UI+2);
    b->ticks=TMC_MAIN.ticks-1; /* main loop increment, but task did not execute */
    words((const void*)&TMC_SCREEN,&b->screen,sizeof(TmcScreen));
    b->usedPalettes=*(volatile u32*)ADDR_G_USED_PALETTES;
    words((const void*)0x0600C000u,b->font,sizeof(b->font));
    words((const void*)0x0600F800u,b->map,sizeof(b->map));
    words((const void*)ADDR_G_BG0_BUFFER,b->bg0,sizeof(b->bg0));
    words((const void*)ADDR_G_PALETTE_BUFFER,b->palette,sizeof(b->palette));
    words((const void*)ADDR_BG_PALETTE,b->hardwarePalette,sizeof(b->hardwarePalette));
    capture_context();
    /* Blank while replacing the two VRAM ranges. Native VBlank commits the
     * menu's display registers. No OAM, native UI, script or fade reset. */
    *(volatile u16*)0x04000000u|=0x80;
    Practice_ClearBytes((void*)&TMC_SCREEN,sizeof(TmcScreen));
    gPracticeState.modalMode=1;
    consume_keys();
}

static void restore_modal(void) {
    ModalBackup* b=&gPracticeModalBackup;
    *(volatile u16*)0x04000000u|=0x80;
    words(b->font,(void*)0x0600C000u,sizeof(b->font));
    words(b->map,(void*)0x0600F800u,sizeof(b->map));
    words(b->bg0,(void*)ADDR_G_BG0_BUFFER,sizeof(b->bg0));
    words(b->palette,(void*)ADDR_G_PALETTE_BUFFER,sizeof(b->palette));
    words(b->hardwarePalette,(void*)ADDR_BG_PALETTE,sizeof(b->hardwarePalette));
    words(&b->screen,(void*)&TMC_SCREEN,sizeof(TmcScreen));
    *(volatile u32*)ADDR_G_USED_PALETTES=b->usedPalettes;
    TMC_MAIN.ticks=b->ticks;
    gPracticeState.menuOpen=gPracticeState.modalMode=gPracticeState.modalSaveEdits=0;
    gPracticeState.modalContext=0;
    consume_keys();
}

u32 PracticeModal_Tick(void) {
    gPracticeState.modalSkipTail=0;
    if(!gPracticeState.modalMode) {
        if((TMC_INPUT.held&gPracticeState.menuHotkey)!=gPracticeState.menuHotkey ||
           !(TMC_INPUT.pressed&gPracticeState.menuHotkey) || !can_open()) return 0;
        open_modal();
    }
    gPracticeState.modalSkipTail=1;
    TMC_MAIN.ticks=gPracticeModalBackup.ticks;
    if(gPracticeState.modalMode==2) { restore_modal();return 1; }
    PracticeMenu_Update();
    /* The native per-palette fade values are preserved, not suitable for the
     * temporary theme. Theme colors were written directly to palette RAM. */
    *(volatile u32*)ADDR_G_USED_PALETTES=0;
    return 1;
}

void PracticeScreenTaskWrapper(void) {
    PracticeRuntime_Init();
    if(PracticeModal_Tick()) return;
    if(SceneWarp_Tick()) return;
    if(TMC_MAIN.task==0) CALL_VOID0(TMC_TITLE_TASK)();
    else if(TMC_MAIN.task==4) ((void(*)(void))0x080A35E1u)();
}

void PracticeFrameTailDispatch(void) {
    /* Same two native calls, same order, except while a modal owns the screen
     * (including its restoration frame). Audio and VBlank still run normally. */
    if(gPracticeState.magic==PRACTICE_MAGIC &&
       (gPracticeState.modalMode || gPracticeState.modalSkipTail)) return;
    ((void(*)(void))0x08056459u)();
    ((void(*)(void))0x08050155u)();
}

u32 PracticeModal_CanAct(u32 page,u32 row) {
    u32 edit=gPracticeState.modalSaveEdits;
    if(gPracticeState.sceneReplay) return page==PAGE_ROOT || (page==PAGE_WARP && row==7);
    if(!gPracticeState.modalMode) return 1;
    switch(page) {
        case PAGE_ROOT: case PAGE_SETTINGS: case PAGE_BINDINGS: case PAGE_INVENTORY:
        case PAGE_FLAGS: case PAGE_FAVORITES: case PAGE_DEBUG: return 1;
        case PAGE_PRACTICE: return row>=4 && row<=5;
        case PAGE_MOVEMENT: return row<=3 || row==5;
        case PAGE_CHEATS: return row!=2 && row!=3;
        case PAGE_WARP: return row!=2 && row<5;
        case PAGE_NUDGE: return row==4;
        case PAGE_WEAPONS: case PAGE_BOTTLES: case PAGE_UPGRADES:
        case PAGE_ELEMENTS: case PAGE_PLAYER: case PAGE_ENEMIES: return edit;
        case PAGE_QUEST: return row==9 || edit;
        case PAGE_CONFIRM_ALL: return row==1 || edit;
        case PAGE_CONFIRM_DELETE: return row==0 || edit;
        case PAGE_KNOWN_FLAGS: return row==0 || (edit && row<3);
        case PAGE_RAW_FLAGS: return row<2 || edit;
        case PAGE_CONFIRM_COMPLETE: case PAGE_CONFIRM_UNDO: return row==0;
        case PAGE_ACTORS: return row<3;
        case PAGE_ACTOR_LIST: return 1;
        case PAGE_ACTOR_SPAWN: return row<9 && gPracticeState.actorRaw;
        case PAGE_ACTOR_DETAIL: return row==6;
        case PAGE_ACTOR_DELETE: return row==0;
        case PAGE_BETA: return row==2;
        default: return 0;
    }
}
