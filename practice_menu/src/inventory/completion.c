#include "practice.h"
#include "completion_flags.h"

void Completion_Capture(void) {
    PracticePosition* p=&gPracticeState.completionReturn;
    /* Reapplying 100% must not replace the original pre-completion snapshot. */
    if (Completion_CanUndo()) return;
    Practice_CopyBytes((const void*)ADDR_G_SAVE,gPracticeState.completionBackup,FULL_SAVE_SIZE);
    p->valid=1; p->area=gPracticeState.menuArea; p->room=gPracticeState.menuRoom;
    p->layer=gPracticeState.menuCollisionLayer;
    p->originX=gPracticeState.menuOriginX; p->originY=gPracticeState.menuOriginY;
    p->x=gPracticeState.menuX; p->y=gPracticeState.menuY; p->z=gPracticeState.menuZ;
    p->direction=gPracticeState.menuDirection; p->animationState=gPracticeState.menuAnimationState;
    gPracticeState.completionUndoSlot=*(volatile u8*)0x02000004u;
    gPracticeState.completionUndoValid=1;
}

u32 Completion_CanUndo(void) {
    u32 i;
    if (!gPracticeState.completionUndoValid || gPracticeState.completionUndoSlot!=*(volatile u8*)0x02000004u) return 0;
    for(i=0;i<6;i++) if(gPracticeState.completionBackup[0x80+i]!=TMC_SAVE.prefix[0x80+i]) return 0;
    return 1;
}

void Completion_Restore(void) {
    Practice_CopyBytes(gPracticeState.completionBackup,(void*)ADDR_G_SAVE,FULL_SAVE_SIZE);
    gPracticeState.completionUndoValid=0;
    gPracticeState.inventoryDirty=1;
    /* Resource cheats would immediately overwrite restored money/ammo/health. */
    gPracticeState.resourceCheats=0;
    PracticeRuntime_SetStatus("100% UNDONE - SAVE IN GAME");
}

/* Apply only after explicit guarded confirmation and after the native menu
 * has restored gameplay. The caller reloads the area to refresh actors. */
void Completion_Apply(void) {
    u32 i;
    Inventory_GiveAll();
    /* Handed-in quest items are state 2, not active inventory objects. */
    for (i=52;i<=60;i++) Inventory_Set(i,2);
    Inventory_Set(0x5B,1); /* Jabber Nut: understands Minish. */
    Inventory_Set(0x64,1); /* Wallet acquisition marker. */
    Inventory_Set(0x65,1); /* Bomb-bag acquisition marker. */
    Inventory_Set(0x66,1); /* Quiver acquisition marker. */
    Inventory_GiveCollection(2);
    TMC_SAVE.stats.maxHealth=160;
    TMC_SAVE.stats.health=160;
    TMC_SAVE.stats.heartPieces=0;
    TMC_SAVE.stats.rupees=999;
    TMC_SAVE.prefix[6]=1; /* Permanent credits/clear state, not live ENDING. */
    TMC_SAVE.prefix[7]=2; /* Deepwood barrel's native resting orientation. */
    TMC_SAVE.prefix[8]=9; /* Recomputed to 9 natively from stained-glass flag. */
    *(volatile u32*)(TMC_SAVE.prefix+0x40) |= 0xFF000000u;
    for(i=0;i<ARRAY_COUNT(sCompletionFlags);i++) {
        u32 bit=sCompletionFlags[i]; TMC_SAVE.flags[bit>>3] |= 1u<<(bit&7);
    }
    /* Every legitimate fusion ID, no out-of-range bits or arbitrary fuser
     * progress. F3 is native KINSTONE_FUSER_DONE, not an array index. */
    TMC_SAVE.kinstones[2]=1;
    TMC_SAVE.kinstones[3]=100;
    for(i=1;i<=100;i++) TMC_SAVE.kinstones[0x12D+(i>>3)] |= 1u<<(i&7);
    for(i=0;i<128;i++) TMC_SAVE.kinstones[173+i]=0xF3;
    /* Native dungeon_idx = location - 23: six main dungeons are 1..6.
     * Do not fill invalid/unused dungeon slots with arbitrary bytes. */
    for(i=1;i<=6;i++) TMC_SAVE.dungeonItems[i] |= 7;
    /* Native quest cleanup on normal load clears the transient ending and
     * final battle state. Do not set those merely to fake boss completion. */
    PracticeRuntime_SetStatus("100% APPLIED - SAVE IN GAME");
}
