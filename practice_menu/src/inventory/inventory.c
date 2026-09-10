#include "practice.h"

enum {
    ITEM_SMITH_SWORD = 1,
    ITEM_FOURSWORD = 6,
    ITEM_BOMBS = 7,
    ITEM_REMOTE_BOMBS = 8,
    ITEM_BOW = 9,
    ITEM_LIGHT_ARROW = 10,
    ITEM_BOOMERANG = 11,
    ITEM_MAGIC_BOOMERANG = 12,
    ITEM_SHIELD = 13,
    ITEM_MIRROR_SHIELD = 14,
    ITEM_LANTERN_OFF = 15,
    ITEM_GUST_JAR = 17,
    ITEM_PACCI_CANE = 18,
    ITEM_MOLE_MITTS = 19,
    ITEM_ROCS_CAPE = 20,
    ITEM_PEGASUS_BOOTS = 21,
    ITEM_OCARINA = 23,
    ITEM_BOTTLE1 = 28,
    ITEM_BOTTLE4 = 31,
    ITEM_BOTTLE_EMPTY = 32,
    ITEM_QST_SWORD = 52,
    ITEM_MAP = 71,
    ITEM_SKILL_SPIN = 72,
    ITEM_SKILL_PERIL = 79,
    ITEM_SHELLS = 63,
    ITEM_KINSTONE_BAG = 103,
    ITEM_ARROW_BUTTERFLY = 112,
    ITEM_SWIM_BUTTERFLY = 114,
    ITEM_SKILL_FAST_SPIN = 115,
    ITEM_SKILL_LONG_SPIN = 117,
};

u32 Inventory_Get(u32 item) {
    u32 shift;
    if (item >= 136) return 0;
    shift = (item & 3u) * 2u;
    return (TMC_SAVE.inventory[item >> 2] >> shift) & 3u;
}

void Inventory_ElementFlag(u32 item, u32 owned) {
    /* Native progression checks LV1/2/4/5_CLEAR independently of inventory.
     * MACHI_SET boss deaths, local puzzles and sword infusion are NOT these bits. */
    static const u8 flags[4] = {2,3,5,6};
    u32 index, before;
    u8 mask;
    if (item < 64 || item > 67) return;
    index = flags[item - 64]; mask = (u8)(1u << index);
    before = (TMC_SAVE.flags[0] >> index) & 1;
    if (owned) TMC_SAVE.flags[0] |= mask;
    else TMC_SAVE.flags[0] &= (u8)~mask;
    Flags_Record(0,index,before,owned != 0);
}

void Inventory_Set(u32 item, u32 value) {
    u32 shift;
    u8 mask;
    volatile u8* address;
    if (item >= 136 || value > 3) return;
    shift = (item & 3u) * 2u;
    mask = (u8)(3u << shift);
    address = &TMC_SAVE.inventory[item >> 2];
    *address = (u8)((*address & ~mask) | ((value << shift) & mask));
    Inventory_ElementFlag(item,value);
    gPracticeState.inventoryDirty = 1;
}

u32 Inventory_GroupSelection(u32 first, u32 last) {
    u32 item = last;
    while (item >= first) {
        if (Inventory_Get(item) != 0) return item - first + 1;
        if (item == first) break;
        item--;
    }
    return 0;
}

void Inventory_SetWeaponGroup(u32 first, u32 last, u32 selected) {
    u32 item;
    u32 slot;
    u32 replacement = selected != 0 && first + selected - 1 <= last ? first + selected - 1 : 0;
    if (first == 1 && replacement == 5) return; /* Native ItemIsSword rejects unused ID 5. */
    for (item = first; item <= last; item++) Inventory_Set(item, 0);
    if (selected != 0 && first + selected - 1 <= last) {
        Inventory_Set(first + selected - 1, 1);
    }
    for (slot = 0; slot < 2; slot++) {
        item = TMC_SAVE.stats.equipped[slot];
        if (item >= first && item <= last) TMC_SAVE.stats.equipped[slot] = (u8)replacement;
    }
    CALL_VOID0(TMC_UPDATE_PLAYER_SKILLS)();
}

void Inventory_GiveAll(void) {
    u32 item;
    Inventory_SetWeaponGroup(ITEM_SMITH_SWORD, ITEM_FOURSWORD, 6);
    Inventory_SetWeaponGroup(ITEM_BOMBS, ITEM_REMOTE_BOMBS, 2);
    Inventory_SetWeaponGroup(ITEM_BOW, ITEM_LIGHT_ARROW, 2);
    Inventory_SetWeaponGroup(ITEM_BOOMERANG, ITEM_MAGIC_BOOMERANG, 2);
    Inventory_SetWeaponGroup(ITEM_SHIELD, ITEM_MIRROR_SHIELD, 2);
    Inventory_Set(ITEM_LANTERN_OFF, 1);
    Inventory_Set(16, 0);
    Inventory_Set(ITEM_GUST_JAR, 1);
    Inventory_Set(ITEM_PACCI_CANE, 1);
    Inventory_Set(ITEM_MOLE_MITTS, 1);
    Inventory_Set(ITEM_ROCS_CAPE, 1);
    Inventory_Set(ITEM_PEGASUS_BOOTS, 1);
    Inventory_Set(ITEM_OCARINA, 1);
    for (item = ITEM_BOTTLE1; item <= ITEM_BOTTLE4; item++) Inventory_Set(item, 1);
    for (item = 0; item < 4; item++) TMC_SAVE.stats.bottles[item] = ITEM_BOTTLE_EMPTY;
    for (item = ITEM_QST_SWORD; item <= ITEM_MAP; item++) Inventory_Set(item, 1);
    for (item = ITEM_SKILL_SPIN; item <= ITEM_SKILL_PERIL; item++) Inventory_Set(item, 1);
    Inventory_Set(ITEM_SHELLS, 1);
    Inventory_Set(ITEM_KINSTONE_BAG, 1);
    for (item = ITEM_ARROW_BUTTERFLY; item <= ITEM_SWIM_BUTTERFLY; item++) Inventory_Set(item, 1);
    for (item = ITEM_SKILL_FAST_SPIN; item <= ITEM_SKILL_LONG_SPIN; item++) Inventory_Set(item, 1);
    TMC_SAVE.stats.walletType = 3;
    TMC_SAVE.stats.bombBagType = 3;
    TMC_SAVE.stats.quiverType = 3;
    TMC_SAVE.stats.bombCount = 99;
    TMC_SAVE.stats.arrowCount = 99;
    TMC_SAVE.stats.shells = 999;
    TMC_SAVE.stats.equipped[0] = ITEM_FOURSWORD;
    TMC_SAVE.stats.equipped[1] = ITEM_MIRROR_SHIELD;
    CALL_VOID0(TMC_UPDATE_PLAYER_SKILLS)();
    PracticeRuntime_SetStatus("VALID ALL-ITEM LOADOUT SET");
}

void Inventory_DeleteAll(void) {
    /* ITEM_NONE is the native menu-availability sentinel, not an owned item.
     * Keep it so deleting equipment does not also disable native START/save. */
    u8 menuSentinel = TMC_SAVE.inventory[0] & 3;
    Practice_ClearBytes((void*)TMC_SAVE.inventory, sizeof(TMC_SAVE.inventory));
    TMC_SAVE.inventory[0] = menuSentinel;
    for (u32 item = 64; item <= 67; item++) Inventory_ElementFlag(item,0);
    Practice_ClearBytes((void*)TMC_SAVE.stats.equipped, sizeof(TMC_SAVE.stats.equipped));
    Practice_ClearBytes((void*)TMC_SAVE.stats.bottles, sizeof(TMC_SAVE.stats.bottles));
    TMC_SAVE.stats.bombCount = 0;
    TMC_SAVE.stats.arrowCount = 0;
    gPracticeState.inventoryDirty = 1;
    CALL_VOID0(TMC_UPDATE_PLAYER_SKILLS)();
    PracticeRuntime_SetStatus("OTHER STORY FLAGS KEPT");
}

void Inventory_Refresh(void) {
    u32 slot, item;
    if (!gPracticeState.inventoryDirty) return;
    if (PSTATE8(0x8B) || (*(volatile u8*)ADDR_G_MESSAGE & 0x7F) ||
        PLAYER8(0x0C) == 8 || PLAYER8(0x0C) == 22) return;
    /* Run after the native subtask has restored the gameplay actors. */
    CALL_VOID0(TMC_DELETE_CLONES)();
    CALL_VOID0(TMC_PUT_AWAY_ITEMS)();
    Practice_ClearBytes((void*)(ADDR_G_PLAYER_STATE + 0xA0), 8);
    for (slot = 0; slot < 2; slot++) {
        item = TMC_SAVE.stats.equipped[slot];
        if (item && !Inventory_Get(item)) TMC_SAVE.stats.equipped[slot] = 0;
    }
    CALL_VOID0(TMC_UPDATE_PLAYER_SKILLS)();
    *(volatile u8*)(0x0200AF00u + 0x13) = 0x7F;
    *(volatile u8*)(0x0200AF00u + 0x14) = 0x7F;
    gPracticeState.inventoryDirty = 0;
}
