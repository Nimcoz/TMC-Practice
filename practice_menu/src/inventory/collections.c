#include "practice.h"
_Static_assert(__builtin_offsetof(TmcSave, figurines)==0xCE,"native figurine bitset offset");

void Inventory_GiveCollection(u32 which) {
    u32 i;
    if (which==1) {
        /* Native bag: 19 type slots + 19 quantities; first four bytes and
         * all fusion/progress arrays are deliberately untouched. */
        for (i=0;i<19;i++) {
            TMC_SAVE.kinstones[4+i]=i<17 ? 0x65+i : 0;
            TMC_SAVE.kinstones[23+i]=i<17 ? 99 : 0;
        }
        Inventory_Set(103,1);
        PracticeRuntime_SetStatus("17 FRAGMENT TYPES X99");
    } else if (which==2) {
        for (i=1;i<=136;i++) TMC_SAVE.figurines[i>>3] |= 1u<<(i&7);
        TMC_SAVE.stats.figurineCount=136;
        TMC_SAVE.stats.hasAllFigurines0=0xFF;
        TMC_SAVE.stats.hasAllFigurines=1;
        TMC_SAVE.prefix[9]=136;
        Inventory_Set(62,1); /* Carlov Medal, collection's native reward. */
        TMC_SAVE.flags[0x59>>3] |= 1u<<(0x59&7); /* FIGURE_ALLCOMP */
        TMC_SAVE.flags[0x25F>>3] |= 1u<<(0x25F&7); /* bank 2 SHOP07_COMPLETE */
        TMC_SAVE.flags[0x25E>>3] |= 1u<<(0x25E&7); /* SHOP07_TANA: collection viewer available */
        /* Refresh an already-loaded dispenser as well as future room loads. */
        for (i=0;i<72;i++) {
            volatile u8* e=(volatile u8*)(ADDR_G_ENTITIES+i*0x88);
            if (*(volatile u32*)(e+4) && e[8]==6 && e[9]==0x22 && e[10]==3 && e[12]==4) {
                e[0x7B]=4; e[0x80]=136; e[0x82]=0; e[0x83]=0;
            }
        }
        PracticeRuntime_SetStatus("136 FIGURINES + CARLOV MEDAL");
    }
}

/* Native eligibility count must include already-owned figures. Otherwise an
 * early-game complete collection makes (available - owned) negative in the
 * dispenser. Unowned entries retain the unmodified native story/fusion test.
 * This invariant also survives a native save/load; no RAM toggle is required. */
u32 FigurineAvailableDispatch(void* device, u32 id) {
    if (id>=1 && id<=136 && ((TMC_SAVE.figurines[id>>3]>>(id&7))&1)) return 1;
    return ((u32(*)(void*,u32))0x08088161u)(device,id);
}
