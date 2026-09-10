#include "practice.h"

static u32 bit_read(volatile u8* bytes, u32 index) {
    return (bytes[index >> 3] >> (index & 7)) & 1u;
}

static void bit_write(volatile u8* bytes, u32 index, u32 value) {
    u8 mask = (u8)(1u << (index & 7));
    if (value) bytes[index >> 3] |= mask;
    else bytes[index >> 3] &= (u8)~mask;
}

u32 Flags_Read(void) {
    u32 index = gPracticeState.flagIndex;
    u32 dungeon = gPracticeState.menuDungeonIndex & 0xF;
    switch (gPracticeState.flagBank) {
        case 0:
            if (index >= 0x1000) return 0;
            return bit_read(TMC_SAVE.flags, index);
        case 1:
            if (gPracticeState.menuLocalFlagOffset + index >= 0x1000) return 0;
            return bit_read(TMC_SAVE.flags, gPracticeState.menuLocalFlagOffset + index);
        case 2:
            if (index >= 0x1A0) return 0;
            return bit_read((volatile u8*)(ADDR_G_ROOM_VARS + 0x14), index);
        case 3:
            if (index >= 8) return 0;
            return (TMC_SAVE.dungeonKeys[dungeon] >> index) & 1;
        case 4:
            if (index >= 8) return 0;
            return (TMC_SAVE.dungeonItems[dungeon] >> index) & 1;
        case 5:
            if (index >= 8) return 0;
            return (TMC_SAVE.dungeonWarps[dungeon] >> index) & 1;
        case 6:
            return bit_read(TMC_SAVE.flags, 0x55);
        default:
            return 0;
    }
}

void Flags_Write(u32 value) {
    u32 index = gPracticeState.flagIndex;
    u32 dungeon = gPracticeState.menuDungeonIndex & 0xF;
    u32 bank = gPracticeState.flagBank;
    if (bank > 6 || (bank == 0 && index >= 0x1000) ||
        (bank == 1 && gPracticeState.menuLocalFlagOffset + index >= 0x1000) ||
        (bank == 2 && index >= 0x1A0) || (bank >= 3 && bank <= 5 && index >= 8)) {
        PracticeRuntime_SetStatus("FLAG ID OUT OF RANGE"); return;
    }
    gPracticeState.flagBefore = Flags_Read();
    gPracticeState.flagChangedBank = gPracticeState.flagBank;
    gPracticeState.flagChangedIndex = bank == 6 ? 0x55 : index;
    switch (gPracticeState.flagBank) {
        case 0:
            if (index < 0x1000) bit_write(TMC_SAVE.flags, index, value);
            break;
        case 1:
            if (gPracticeState.menuLocalFlagOffset + index < 0x1000)
                bit_write(TMC_SAVE.flags, gPracticeState.menuLocalFlagOffset + index, value);
            break;
        case 2:
            if (index < 0x1A0) bit_write((volatile u8*)(ADDR_G_ROOM_VARS + 0x14), index, value);
            break;
        case 3:
            if (index < 8) bit_write(&TMC_SAVE.dungeonKeys[dungeon], index, value);
            break;
        case 4:
            if (index < 8) bit_write(&TMC_SAVE.dungeonItems[dungeon], index, value);
            break;
        case 5:
            if (index < 8) bit_write(&TMC_SAVE.dungeonWarps[dungeon], index, value);
            break;
        case 6:
            bit_write(TMC_SAVE.flags, 0x55, value);
            break;
        default:
            return;
    }
    gPracticeState.flagOperation = value ? 1 : 2;
    gPracticeState.flagAfter = Flags_Read();
    Flags_Record(bank,gPracticeState.flagChangedIndex,gPracticeState.flagBefore,gPracticeState.flagAfter);
    PracticeRuntime_SetStatus(gPracticeState.flagAfter == (value != 0) ? "FLAG WRITE READ BACK OK" : "FLAG WRITE FAILED");
}

void Flags_Record(u32 bank, u32 index, u32 before, u32 after) {
    u32 i;
    if (before == after) return;
    for (i=7;i>0;i--) Practice_CopyBytes(&gPracticeState.flagLog[i-1],&gPracticeState.flagLog[i],sizeof(gPracticeState.flagLog[0]));
    gPracticeState.flagLog[0].bank=bank;
    gPracticeState.flagLog[0].index=index;
    gPracticeState.flagLog[0].before=before;
    gPracticeState.flagLog[0].after=after;
    gPracticeState.flagLog[0].dungeon=gPracticeState.menuDungeonIndex;
    if (gPracticeState.flagLogCount<8) gPracticeState.flagLogCount++;
}

/* Names verified from native flag definitions; no invented boss/quest reset. */
static const u8 generalIds[] = {0x13,0x14,0x15,0x1D,0x24,0x0A,0x0B,0x0D,0x0E};
static const char* const generalNames[] = {
    "MET ZELDA", "MET EZLO", "SPOKE DALTUS / SMITH", "GAVE TALON KEY",
    "CLOUD VORTEX SPAWNED", "GREEN CHUCHU DEFEATED", "GLEEROK DEFEATED",
    "BIG OCTOROK DEFEATED", "GYORG PAIR DEFEATED"
};
static const char* const portals[] = {
    "MT CRENEL", "VEIL FALLS", "CLOUD TOPS", "HYRULE TOWN",
    "LAKE HYLIA", "CASTOR WILDS", "SOUTH HYRULE FIELD", "MINISH WOODS"
};
u32 Flags_ListCount(void) { return (const u8[]){9,4,8,0}[gPracticeState.flagGroup % 4]; }
const char* Flags_ListName(u32 row) {
    if (gPracticeState.flagGroup==0) return generalNames[row%9];
    if (gPracticeState.flagGroup==1) return (const char*[]){"SMALL KEYS","DUNGEON MAP","COMPASS","BIG KEY"}[row%4];
    return portals[row%8];
}
u32 Flags_ListRead(u32 row) {
    u32 dungeon=gPracticeState.menuDungeonIndex;
    if (gPracticeState.flagGroup==0) return bit_read(TMC_SAVE.flags,generalIds[row%9]);
    if (gPracticeState.flagGroup==2) return bit_read(TMC_SAVE.prefix+0x40,24+row%8);
    if (dungeon>=16) return 0;
    if (row==0) return TMC_SAVE.dungeonKeys[dungeon];
    return (TMC_SAVE.dungeonItems[dungeon]>>(row-1))&1;
}
void Flags_ListEdit(u32 row, s32 direction) {
    u32 before=Flags_ListRead(row), after=!before, bank, id;
    u32 dungeon=gPracticeState.menuDungeonIndex;
    if (gPracticeState.flagGroup==0) {
        bank=0; id=generalIds[row%9]; bit_write(TMC_SAVE.flags,id,after);
    } else if (gPracticeState.flagGroup==2) {
        bank=7; id=24+row%8; bit_write(TMC_SAVE.prefix+0x40,id,after);
    } else if (gPracticeState.flagGroup==1 && dungeon<16) {
        if (!row) {
            bank=8; id=dungeon;
            after=direction<0 ? (before ? before-1 : 0) : (before<99 ? before+1 : 99);
            TMC_SAVE.dungeonKeys[dungeon]=after;
        } else {
            bank=4; id=row-1; bit_write(&TMC_SAVE.dungeonItems[dungeon],id,after);
        }
    } else { PracticeRuntime_SetStatus("ENTER A DUNGEON FIRST"); return; }
    Flags_Record(bank,id,before,Flags_ListRead(row));
    PracticeRuntime_SetStatus("APPLIED - SEE EDIT HISTORY");
}

/* Verified in flags.h and golden enemy native OnDeath / init handlers.
 * Editing death flags is not equivalent to replaying a fight or item event. */
static const char* const knownNames[] = {
    "GOLD OCTOROK A DEFEATED", "GOLD TEKTITE B DEFEATED", "GOLD ROPE C DEFEATED",
    "GOLD ROPE D DEFEATED", "GOLD ROPE E DEFEATED", "GOLD TEKTITE F DEFEATED",
    "GOLD TEKTITE G DEFEATED", "GOLD OCTOROK H DEFEATED", "GOLD OCTOROK I DEFEATED"
};
const char* Flags_KnownName(void) { return knownNames[gPracticeState.knownFlag % 9]; }
u32 Flags_KnownRead(void) { return bit_read(TMC_SAVE.flags,0x31 + gPracticeState.knownFlag % 9); }
void Flags_KnownWrite(u32 value) {
    u8 bank = gPracticeState.flagBank;
    u16 index = gPracticeState.flagIndex;
    gPracticeState.flagBank = 0;
    gPracticeState.flagIndex = 0x31 + gPracticeState.knownFlag % 9;
    Flags_Write(value);
    gPracticeState.flagBank = bank;
    gPracticeState.flagIndex = index;
}
