#include "practice.h"

static u32 resources_active(void) {
    return gPracticeState.magic == PRACTICE_MAGIC && !gPracticeState.menuOpen &&
        TMC_MAIN.task == TASK_GAME && TMC_MAIN.state == GAMETASK_MAIN &&
        TMC_MAIN.substate == GAMEMAIN_UPDATE && PLAYER8(8) == 1 &&
        !TMC_TRANSITION.transitioningOut && !*(volatile u8*)ADDR_G_FADE_CONTROL &&
        !PSTATE8(0x8B) && !PSTATE8(0x3C) && PLAYER8(0x0C) != 10 &&
        !(*(volatile u8*)ADDR_G_MESSAGE & 0x7F);
}

void Cheats_RefillResources(void) {
    /* Native itemUtils.c capacities; no upgrade/item/story writes. */
    static const u8 bombs[4] = {10,30,50,99};
    static const u8 arrows[4] = {30,50,70,99};
    static const u16 wallets[4] = {100,300,500,999};
    u32 bits = gPracticeState.resourceCheats;
    if (!bits || !resources_active()) return;
    if ((bits & CHEAT_HEARTS) && TMC_SAVE.stats.health && TMC_SAVE.stats.maxHealth) {
        TMC_SAVE.stats.health = TMC_SAVE.stats.maxHealth;
        PLAYER8(0x45) = TMC_SAVE.stats.maxHealth;
    }
    if ((bits & CHEAT_BOMBS) && TMC_SAVE.stats.bombBagType < 4 &&
        (Inventory_Get(7) || Inventory_Get(8)))
        TMC_SAVE.stats.bombCount = bombs[TMC_SAVE.stats.bombBagType];
    if ((bits & CHEAT_ARROWS) && TMC_SAVE.stats.quiverType < 4 &&
        (Inventory_Get(9) || Inventory_Get(10)))
        TMC_SAVE.stats.arrowCount = arrows[TMC_SAVE.stats.quiverType];
    if ((bits & CHEAT_RUPEES) && TMC_SAVE.stats.walletType < 4)
        TMC_SAVE.stats.rupees = wallets[TMC_SAVE.stats.walletType];
}

s32 Cheats_ModHealth(s32 delta) {
    /* Native ModHealth 080526A0 / gameUtils.c semantics, including return and
     * both health mirrors. Only negative health changes are intercepted when
     * enabled/alive/controlled. Native collision, knockback and iframes remain.
     * This prevents lethal native health deltas before death is queued rather
     * than reviving/forcing Link's action after he is already dead. */
    s32 value = TMC_SAVE.stats.health + delta;
    if (delta < 0 && (gPracticeState.resourceCheats & CHEAT_HEARTS) &&
        TMC_SAVE.stats.health && resources_active()) value = TMC_SAVE.stats.maxHealth;
    if (value < 0) value = 0;
    if (value > TMC_SAVE.stats.maxHealth) value = TMC_SAVE.stats.maxHealth;
    TMC_SAVE.stats.health = (u8)value;
    PLAYER8(0x45) = (u8)value;
    return value;
}
