#include "practice.h"

static const char* const sRootItems[] = {
    "PRACTICE", "PLAYER", "MOVEMENT", "INVENTORY", "WORLD / WARP",
    "FLAGS", "DEBUG", "CHEATS", "SETTINGS", "BETA / UNUSED", "ACTORS / OBJECTS",
};

static const char* const sSwordNames[] = { "NONE", "SMITH", "GREEN", "RED", "BLUE", "UNUSED", "FOUR" };
static const char* const sShieldNames[] = { "NONE", "SHIELD", "MIRROR" };
static const char* const sBombNames[] = { "NONE", "BOMBS", "REMOTE" };
static const char* const sBowNames[] = { "NONE", "BOW", "LIGHT" };
static const char* const sBoomNames[] = { "NONE", "NORMAL", "MAGIC" };
static const char* const sBottleNames[] = { "EMPTY", "RED", "BLUE", "FAIRY", "MILK", "WATER" };
static const u8 sBottleValues[] = { 32, 36, 37, 40, 34, 38 };
static const char* const sFlagBanks[] = {
    "GLOBAL", "LOCAL", "ROOM", "DGN KEY", "DGN ITEM", "EVENT/WARP", "GAMECLEAR",
};
static const char* const sFlagHelp[] = {
    "SAVE-WIDE CONDITION", "CURRENT AREA CONDITION", "CURRENT ROOM RAM",
    "CURRENT DUNGEON KEY BIT", "MAP / COMPASS / BIGKEY",
    "CURRENT DUNGEON WARP", "FIXED GAME-CLEAR FLAG",
};
static const char* const sElementNames[] = { "EARTH", "FIRE", "WATER", "WIND" };

static u32 page_count(u32 page) {
    if (page == PAGE_ENEMIES) return Flags_ListCount();
    if (page == PAGE_ACTOR_LIST) return Actors_Count();
    if (page == PAGE_ACTOR_SPAWN && gPracticeState.actorRaw) return 10;
    static const u8 counts[MAX_MENU_PAGES] = {
        11, 11, 3, 6, 12, 5, 7, 13, 8, 7, 0, 6, 12, 7, 2, 4,
        13, 2, 5, 4, 4, 3, 2, 8, 9, 2, 4, 0, 9, 2, 2,
    };
    if (page >= MAX_MENU_PAGES) return 0;
    return counts[page];
}

static void draw_cursor(u32 row) {
    if (gPracticeState.cursor[gPracticeState.page] == row) {
        PracticeRender_Text(1, 3 + row, ">", 1);
    }
}

static void draw_item(u32 row, const char* label, const char* value, u32 palette) {
    draw_cursor(row);
    PracticeRender_Text(3, 3 + row, label, palette);
    if (value != 0) PracticeRender_Text(20, 3 + row, value, palette);
}

static void draw_toggle(u32 row, const char* label, u32 enabled) {
    draw_item(row, label, enabled ? "ON" : "OFF", enabled ? 2 : 0);
}

static void draw_number(u32 row, const char* label, u32 value, u32 digits) {
    draw_item(row, label, 0, 0);
    PracticeRender_U32(20, 3 + row, value, digits, 0);
}

static void capture_position(PracticePosition* position) {
    position->valid = 1;
    position->area = gPracticeState.menuArea;
    position->room = gPracticeState.menuRoom;
    position->layer = gPracticeState.menuCollisionLayer;
    position->direction = gPracticeState.menuDirection;
    position->animationState = gPracticeState.menuAnimationState;
    position->originX = gPracticeState.menuOriginX;
    position->originY = gPracticeState.menuOriginY;
    position->x = gPracticeState.menuX;
    position->y = gPracticeState.menuY;
    position->z = gPracticeState.menuZ;
}

static void draw_timer(u32 row, const char* label, u32 frames) {
    u32 minutes = frames / 3600;
    u32 seconds = (frames / 60) % 60;
    u32 remainder = frames % 60;
    draw_item(row, label, 0, 0);
    PracticeRender_U32(18, 3 + row, minutes, 2, 0);
    PracticeRender_Text(20, 3 + row, ":", 0);
    PracticeRender_U32(21, 3 + row, seconds, 2, 0);
    PracticeRender_Text(23, 3 + row, ":", 0);
    PracticeRender_U32(24, 3 + row, remainder, 2, 0);
}

static u32 cycle_value(u32 value, u32 maximum, s32 direction) {
    if (direction > 0) return value >= maximum ? 0 : value + 1;
    return value == 0 ? maximum : value - 1;
}

static s32 horizontal_direction(u16 keys) {
    if ((keys & KEY_LEFT) != 0) return -1;
    if ((keys & KEY_RIGHT) != 0) return 1;
    return 0;
}

static void go_back(void) {
    switch (gPracticeState.page) {
        case PAGE_WEAPONS:
        case PAGE_BOTTLES:
        case PAGE_UPGRADES:
        case PAGE_QUEST:
        case PAGE_CONFIRM_ALL:
            if (gPracticeState.page == PAGE_CONFIRM_ALL && gPracticeState.collectionAction) {
                gPracticeState.collectionAction=0; gPracticeState.page=PAGE_CHEATS; break;
            }
            /* fall through */
        case PAGE_CONFIRM_DELETE:
            gPracticeState.page = PAGE_INVENTORY;
            break;
        case PAGE_ELEMENTS:
            gPracticeState.page = PAGE_QUEST;
            break;
        case PAGE_NUDGE: gPracticeState.page = PAGE_MOVEMENT; break;
        case PAGE_KNOWN_FLAGS: gPracticeState.page = PAGE_FLAGS; break;
        case PAGE_RAW_FLAGS: gPracticeState.page = PAGE_FLAGS; break;
        case PAGE_ENEMIES: gPracticeState.page = PAGE_FLAGS; break;
        case PAGE_CONFIRM_COMPLETE: gPracticeState.page = PAGE_CHEATS; break;
        case PAGE_CONFIRM_UNDO: gPracticeState.page = PAGE_CHEATS; break;
        case PAGE_FAVORITES: gPracticeState.page = PAGE_WARP; break;
        case PAGE_BINDINGS: gPracticeState.page = PAGE_SETTINGS; break;
        case PAGE_ACTOR_LIST:
        case PAGE_ACTOR_SPAWN: gPracticeState.page = PAGE_ACTORS; break;
        case PAGE_ACTOR_DETAIL: gPracticeState.page = PAGE_ACTOR_LIST; break;
        case PAGE_ACTOR_DELETE: gPracticeState.page = gPracticeState.actorConfirmSpawn?PAGE_ACTOR_SPAWN:PAGE_ACTOR_DETAIL; break;
        case PAGE_ROOT:
            PracticeRuntime_CloseMenu();
            break;
        default:
            gPracticeState.page = PAGE_ROOT;
            break;
    }
}

static void action_practice(u32 row, u16 pressed) {
    if ((pressed & KEY_A) == 0) return;
    switch (row) {
        case 0:
            capture_position(&gPracticeState.savedPosition);
            PracticeRuntime_SetStatus("POSITION SAVED");
            break;
        case 1:
            if (gPracticeState.savedPosition.valid) PracticeRuntime_Queue(PENDING_LOAD_POSITION);
            else PracticeRuntime_SetStatus("NO SAVED POSITION");
            break;
        case 2: PracticeRuntime_Queue(PENDING_RELOAD_ROOM); break;
        case 3: PracticeRuntime_Queue(PENDING_RELOAD_AREA); break;
        case 4:
            gPracticeState.timerRunning ^= 1;
            PracticeRuntime_SetStatus(gPracticeState.timerRunning ? "TIMER STARTED" : "TIMER PAUSED");
            break;
        case 5:
            gPracticeState.timerFrames = 0;
            PracticeRuntime_SetStatus("TIMER RESET");
            break;
        case 6:
            capture_position(&gPracticeState.preset.position);
            Practice_CopyBytes((const void*)&TMC_SAVE.stats, &gPracticeState.preset.stats, sizeof(TmcStats));
            Practice_CopyBytes((const void*)TMC_SAVE.inventory, gPracticeState.preset.inventory, 34);
            gPracticeState.preset.flagCount = 0;
            gPracticeState.preset.valid = 1;
            PracticeRuntime_SetStatus("RUNTIME PRESET CAPTURED");
            break;
        case 7:
            if (!gPracticeState.preset.valid) {
                PracticeRuntime_SetStatus("NO CAPTURED PRESET");
                break;
            }
            Practice_CopyBytes(&gPracticeState.preset.stats, (void*)&TMC_SAVE.stats, sizeof(TmcStats));
            Practice_CopyBytes(gPracticeState.preset.inventory, (void*)TMC_SAVE.inventory, 34);
            gPracticeState.inventoryDirty = 1;
            PracticeRuntime_Queue(PENDING_APPLY_PRESET);
            break;
        case 10: SceneSkip_Start(); break;
        default: break;
    }
}

static void action_player(u32 row, u16 pressed, s32 direction) {
    if (row == 0 && (pressed & KEY_A)) Player_FullHealth();
    if (row == 1 && direction) Player_AdjustMaxHealth(direction);
    if (row == 2 && direction) Player_AdjustRupees(direction * ((pressed & (KEY_L | KEY_R)) ? 100 : 10));
}

static void edit_group(u32 first, u32 last, u32 row, u16 pressed, s32 direction) {
    u32 selected = Inventory_GroupSelection(first, last);
    u32 count = last - first + 1;
    if ((pressed & KEY_A) || direction) {
        selected = cycle_value(selected, count, direction == 0 ? 1 : direction);
        if (first == 1 && selected == 5) selected = direction < 0 ? 4 : 6;
        Inventory_SetWeaponGroup(first, last, selected);
    }
    (void)row;
}

static void action_weapons(u32 row, u16 pressed, s32 direction) {
    u32 item;
    if (row == 0) edit_group(1, 6, row, pressed, direction);
    else if (row == 1) edit_group(13, 14, row, pressed, direction);
    else if (row == 2) edit_group(7, 8, row, pressed, direction);
    else if (row == 3) edit_group(9, 10, row, pressed, direction);
    else if (row == 4) edit_group(11, 12, row, pressed, direction);
    else {
        static const u8 items[] = { 15, 17, 18, 19, 20, 21, 23 };
        item = items[row - 5];
        if ((pressed & KEY_A) || direction) {
            Inventory_Set(item, Inventory_Get(item) ? 0 : 1);
            if (item == 15) Inventory_Set(16, 0);
        }
    }
}

static u32 bottle_content_index(u8 value) {
    u32 index;
    for (index = 0; index < ARRAY_COUNT(sBottleValues); index++) {
        if (sBottleValues[index] == value) return index;
    }
    return 0;
}

static void action_bottles(u32 row, u16 pressed, s32 direction) {
    u32 owned;
    u32 index;
    u32 slot;
    if (row == 4 && (pressed & KEY_A)) {
        for (slot = 0; slot < 4; slot++) {
            Inventory_Set(28 + slot, 0);
            TMC_SAVE.stats.bottles[slot] = 0;
        }
        return;
    }
    if (row >= 4) return;
    owned = Inventory_Get(28 + row);
    if (pressed & KEY_A) {
        Inventory_Set(28 + row, owned ? 0 : 1);
        TMC_SAVE.stats.bottles[row] = owned ? 0 : 32;
    } else if (direction && owned) {
        index = bottle_content_index(TMC_SAVE.stats.bottles[row]);
        index = cycle_value(index, ARRAY_COUNT(sBottleValues) - 1, direction);
        TMC_SAVE.stats.bottles[row] = sBottleValues[index];
    }
}

static void action_upgrades(u32 row, s32 direction) {
    static const u8 ammoMaximum[4] = { 10, 30, 50, 99 };
    if (!direction) return;
    switch (row) {
        case 0:
            TMC_SAVE.stats.bombCount = (u8)cycle_value(TMC_SAVE.stats.bombCount,
                ammoMaximum[TMC_SAVE.stats.bombBagType & 3], direction);
            break;
        case 1:
            TMC_SAVE.stats.arrowCount = (u8)cycle_value(TMC_SAVE.stats.arrowCount,
                (const u8[4]){30, 50, 70, 99}[TMC_SAVE.stats.quiverType & 3], direction);
            break;
        case 2: TMC_SAVE.stats.bombBagType = (u8)cycle_value(TMC_SAVE.stats.bombBagType, 3, direction); break;
        case 3: TMC_SAVE.stats.quiverType = (u8)cycle_value(TMC_SAVE.stats.quiverType, 3, direction); break;
        case 4: TMC_SAVE.stats.walletType = (u8)cycle_value(TMC_SAVE.stats.walletType, 3, direction); break;
        case 5:
            if (direction > 0) TMC_SAVE.stats.shells = TMC_SAVE.stats.shells >= 999 ? 0 : TMC_SAVE.stats.shells + 1;
            else TMC_SAVE.stats.shells = TMC_SAVE.stats.shells == 0 ? 999 : TMC_SAVE.stats.shells - 1;
            break;
        case 6: TMC_SAVE.stats.heartPieces = (u8)cycle_value(TMC_SAVE.stats.heartPieces, 3, direction); break;
        default: break;
    }
}

static void toggle_item_range(u32 first, u32 last) {
    u32 item;
    u32 value = Inventory_Get(first) ? 0 : 1;
    for (item = first; item <= last; item++) Inventory_Set(item, value);
}

static void action_quest(u32 row, u16 pressed) {
    static const u8 singleItems[] = { 52, 53, 54, 55, 56, 60, 61, 62, 70, 71 };
    u32 item;
    if ((pressed & KEY_A) == 0) return;
    if (row <= 4) item = singleItems[row];
    else if (row == 5) { toggle_item_range(57, 59); return; }
    else if (row <= 8) item = singleItems[row - 1];
    else if (row == 9) { gPracticeState.page = PAGE_ELEMENTS; return; }
    else if (row == 10) { toggle_item_range(68, 69); return; }
    else item = singleItems[row - 3];
    Inventory_Set(item, Inventory_Get(item) ? 0 : 1);
}

static void action_elements(u32 row, u16 pressed) {
    static const char* const added[] = {
        "EARTH ELEMENT ADDED", "FIRE ELEMENT ADDED",
        "WATER ELEMENT ADDED", "WIND ELEMENT ADDED",
    };
    static const char* const removed[] = {
        "EARTH ELEMENT REMOVED", "FIRE ELEMENT REMOVED",
        "WATER ELEMENT REMOVED", "WIND ELEMENT REMOVED",
    };
    u32 item;
    u32 owned;
    if ((pressed & KEY_A) == 0 || row >= 4) return;
    item = 64 + row;
    owned = Inventory_Get(item);
    Inventory_Set(item, owned ? 0 : 1);
    PracticeRuntime_SetStatus(owned ? removed[row] : added[row]);
}

static void process_action(u16 pressed, u16 repeat) {
    u32 row = gPracticeState.cursor[gPracticeState.page];
    s32 direction = horizontal_direction((u16)(pressed | repeat));
    if (((pressed&(KEY_A|KEY_L|KEY_R)) || direction) &&
        !PracticeModal_CanAct(gPracticeState.page,row)) {
        PracticeRuntime_SetStatus("USE FROM ACTIVE GAMEPLAY"); return;
    }
    switch (gPracticeState.page) {
        case PAGE_ROOT:
            if (pressed & KEY_A) {
                static const u8 rootPages[] = {
                    PAGE_PRACTICE, PAGE_PLAYER, PAGE_MOVEMENT, PAGE_INVENTORY, PAGE_WARP,
                    PAGE_FLAGS, PAGE_DEBUG, PAGE_CHEATS, PAGE_SETTINGS, PAGE_BETA, PAGE_ACTORS,
                };
                gPracticeState.page = rootPages[row];
            }
            break;
        case PAGE_ACTORS: case PAGE_ACTOR_LIST: case PAGE_ACTOR_DETAIL:
        case PAGE_ACTOR_SPAWN: case PAGE_ACTOR_DELETE:
            Actors_Action(row, pressed, direction); break;
        case PAGE_PRACTICE: action_practice(row, pressed); break;
        case PAGE_PLAYER: action_player(row, pressed, direction); break;
        case PAGE_INVENTORY:
            if (pressed & KEY_A) {
                static const u8 inventoryPages[] = {
                    PAGE_WEAPONS, PAGE_BOTTLES, PAGE_UPGRADES, PAGE_QUEST, PAGE_CONFIRM_ALL, PAGE_CONFIRM_DELETE,
                };
                gPracticeState.page = inventoryPages[row];
                if (row == 4) { gPracticeState.cursor[PAGE_CONFIRM_ALL] = 1; gPracticeState.collectionAction=0; }
                if (row == 5) gPracticeState.cursor[PAGE_CONFIRM_DELETE] = 0;
            }
            break;
        case PAGE_WEAPONS: action_weapons(row, pressed, direction); break;
        case PAGE_BOTTLES: action_bottles(row, pressed, direction); break;
        case PAGE_UPGRADES: action_upgrades(row, direction); break;
        case PAGE_QUEST: action_quest(row, pressed); break;
        case PAGE_ELEMENTS: action_elements(row, pressed); break;
        case PAGE_CONFIRM_ALL:
            if (pressed & KEY_A) {
                if (row == 0) {
                    if (gPracticeState.collectionAction && (gPracticeState.pausedMessageValid || PSTATE8(0x8B))) {
                        PracticeRuntime_SetStatus("COLLECTION: END DIALOG FIRST"); return;
                    }
                    if (gPracticeState.collectionAction) Inventory_GiveCollection(gPracticeState.collectionAction);
                    else Inventory_GiveAll();
                }
                gPracticeState.page = gPracticeState.collectionAction ? PAGE_CHEATS : PAGE_INVENTORY;
                gPracticeState.collectionAction=0;
            }
            break;
        case PAGE_CONFIRM_DELETE:
            if (pressed & KEY_A) {
                if (row == 1) Inventory_DeleteAll();
                gPracticeState.page = PAGE_INVENTORY;
            }
            break;
        case PAGE_WARP:
            if (row==7 && (pressed&KEY_A)) SceneWarp_Return();
            if ((row==5 || row==6) && (pressed&KEY_A)) {
                if (gPracticeState.sceneConfirm==row+1) {
                    gPracticeState.sceneConfirm=0;
                    PracticeRuntime_Queue(row==5?PENDING_STORY_INTRO:PENDING_STORY_ENDING);
                } else {
                    gPracticeState.sceneConfirm=row+1;
                    PracticeRuntime_SetStatus("A AGAIN: START STORY REPLAY");
                }
            }
            if (row==3 && (pressed&KEY_A)) Warp_ToggleFavorite();
            if (row==4 && (pressed&KEY_A)) gPracticeState.page=PAGE_FAVORITES;
            if (row == 0 && direction) Warp_AdjustArea(direction);
            if (row == 1 && direction) Warp_AdjustRoom(direction);
            if (row == 2 && (pressed & KEY_A)) {
                if (Warp_IsValid(gPracticeState.selectedArea, gPracticeState.selectedRoom))
                    PracticeRuntime_Queue(PENDING_WARP);
                else PracticeRuntime_SetStatus("INVALID AREA / ROOM");
            }
            break;
        case PAGE_FAVORITES:
            if(pressed&KEY_A) {
                u8* favorite=gPracticeState.favorites[row];
                if(Warp_IsValid(favorite[0],favorite[1])) {
                    gPracticeState.selectedArea=favorite[0]; gPracticeState.selectedRoom=favorite[1];
                    gPracticeState.page=PAGE_WARP; gPracticeState.cursor[PAGE_WARP]=2;
                } else PracticeRuntime_SetStatus("EMPTY SLOT - ADD IN WARP");
            }
            break;
        case PAGE_BINDINGS:
            if(pressed&KEY_A) {
                static const u16 bits[]={KEY_A,KEY_B,KEY_SELECT,KEY_START,KEY_L,KEY_R};
                if(row==0) {
                    gPracticeState.bindingTarget^=1;
                    gPracticeState.bindingDraft=gPracticeState.bindingTarget?gPracticeState.confirmHotkey:gPracticeState.menuHotkey;
                } else if(row<=6) gPracticeState.bindingDraft^=bits[row-1];
                else if(row==8) gPracticeState.bindingDraft=gPracticeState.bindingTarget?(KEY_L|KEY_R|KEY_A):PRACTICE_HOTKEY;
                else {
                    u16 menu=gPracticeState.bindingTarget?gPracticeState.menuHotkey:gPracticeState.bindingDraft;
                    u16 confirm=gPracticeState.bindingTarget?gPracticeState.bindingDraft:gPracticeState.confirmHotkey;
                    if(Settings_ValidKeys(menu,confirm)) {
                        gPracticeState.menuHotkey=menu; gPracticeState.confirmHotkey=confirm;
                        gPracticeState.page=PAGE_SETTINGS;
                        PracticeRuntime_SetStatus("KEYS APPLIED - SAVE SETTINGS");
                    } else PracticeRuntime_SetStatus("INVALID OR CONFLICTING KEYS");
                }
            }
            break;
        case PAGE_MOVEMENT:
            if (row == 0 && ((pressed & KEY_A) || direction)) {
                gPracticeState.noClip ^= 1;
                gPracticeState.roomExplore = gPracticeState.noClip;
            }
            if (row == 1 && ((pressed & KEY_A) || direction))
                gPracticeState.speedMode = (u8)cycle_value(gPracticeState.speedMode,2,direction ? direction : 1);
            if (row == 2 && (pressed & KEY_A)) gPracticeState.lockDirection ^= 1;
            if (row == 3 && (pressed & KEY_A)) gPracticeState.cameraMode = gPracticeState.cameraMode == 1 ? 0 : 1;
            if (row == 4 && (pressed & KEY_A)) {
                gPracticeState.cameraMode = 2; PracticeRuntime_CloseMenu();
            }
            if (row == 5 && (pressed & KEY_A)) gPracticeState.page = PAGE_NUDGE;
            if (row == 6 && (pressed & KEY_A)) PracticeRuntime_Queue(PENDING_GROUND_RESET);
            break;
        case PAGE_NUDGE:
            if (pressed & KEY_A) {
                if (row < 4) { gPracticeState.nudgeDirection = row; PracticeRuntime_Queue(PENDING_NUDGE); }
                else gPracticeState.nudgeStep ^= 1;
            }
            break;
        case PAGE_CHEATS:
            if (pressed & KEY_A) {
                if (row == 0) gPracticeState.invincibility ^= 1;
                if (row == 1) gPracticeState.freezeEnemies ^= 1;
                if (row == 2) PracticeRuntime_Queue(PENDING_BREAK_FREE);
                if (row == 3) PracticeRuntime_Queue(PENDING_OCARINA_GLITCH);
                if (row == 12) gPracticeState.resourceCheats ^= CHEAT_MINIGAME_TIME;
                if (row >= 4 && row <= 7) gPracticeState.resourceCheats ^= 1u << (row - 4);
                if (row==8 || row==9) {
                    gPracticeState.collectionAction=row-7;
                    gPracticeState.cursor[PAGE_CONFIRM_ALL]=1;
                    gPracticeState.page=PAGE_CONFIRM_ALL;
                }
                if (row==10) {
                    gPracticeState.cursor[PAGE_CONFIRM_COMPLETE]=0;
                    gPracticeState.page=PAGE_CONFIRM_COMPLETE;
                }
                if (row==11) {
                    if(Completion_CanUndo()) {
                        gPracticeState.cursor[PAGE_CONFIRM_UNDO]=0; gPracticeState.page=PAGE_CONFIRM_UNDO;
                    } else PracticeRuntime_SetStatus("NO VALID 100% BACKUP");
                }
            }
            break;
        case PAGE_CONFIRM_COMPLETE:
        case PAGE_CONFIRM_UNDO:
            if (pressed&KEY_A) {
                if (row==0) gPracticeState.page=PAGE_CHEATS;
                else if ((TMC_INPUT.held&gPracticeState.confirmHotkey)==gPracticeState.confirmHotkey)
                    PracticeRuntime_Queue(gPracticeState.page==PAGE_CONFIRM_UNDO?PENDING_UNDO_COMPLETE:PENDING_COMPLETE_GAME);
                else PracticeRuntime_SetStatus("HOLD SHOWN KEYS THEN PRESS A");
            }
            break;
        case PAGE_BETA:
            if (pressed&KEY_A) {
                if (row==0) {
                    if (!gPracticeState.pausedMessageValid && !PSTATE8(0x8B)) {
                        if (!gPracticeState.betaReturn.valid) capture_position(&gPracticeState.betaReturn);
                        PracticeRuntime_Queue(PENDING_BETA_ENTER);
                    } else PracticeRuntime_SetStatus("FINISH CURRENT SCENE FIRST");
                }
                if (row==1 && gPracticeState.betaReturn.valid) PracticeRuntime_Queue(PENDING_BETA_RETURN);
                if (row==2) PracticeRuntime_SetStatus("UNUSED SWORD IS NOT PLAYABLE");
            }
            break;
        case PAGE_FLAGS:
            if (pressed & KEY_A) {
                if (row<2) gPracticeState.page=row==0 ? PAGE_KNOWN_FLAGS : PAGE_RAW_FLAGS;
                else { gPracticeState.flagGroup=row-2; gPracticeState.page=PAGE_ENEMIES; gPracticeState.cursor[PAGE_ENEMIES]=0; }
            }
            break;
        case PAGE_ENEMIES:
            if (row<Flags_ListCount() && ((pressed&KEY_A)||direction)) Flags_ListEdit(row,direction);
            break;
        case PAGE_KNOWN_FLAGS:
            if (row == 0 && direction) gPracticeState.knownFlag = cycle_value(gPracticeState.knownFlag,8,direction);
            if (row == 1 && (pressed & KEY_A)) Flags_KnownWrite(1);
            if (row == 2 && (pressed & KEY_A)) Flags_KnownWrite(0);
            if (row == 3 && (pressed & KEY_A)) PracticeRuntime_Queue(PENDING_RELOAD_ROOM);
            break;
        case PAGE_RAW_FLAGS:
            if (row == 0 && direction) {
                gPracticeState.flagBank = (u8)cycle_value(gPracticeState.flagBank, 6, direction);
                if (gPracticeState.flagBank >= 3 && gPracticeState.flagBank <= 5 && gPracticeState.flagIndex > 7)
                    gPracticeState.flagIndex = 7;
                gPracticeState.flagOperation = 0;
            } else if (row == 1 && direction) {
                s32 value = gPracticeState.flagIndex + direction * ((pressed & (KEY_L | KEY_R)) ? 16 : 1);
                u32 maximum = gPracticeState.flagBank == 2 ? 0x19F :
                              (gPracticeState.flagBank >= 3 ? 7 : 0xFFF);
                if (value < 0) value = maximum;
                if (value > (s32)maximum) value = 0;
                gPracticeState.flagIndex = (u16)value;
                gPracticeState.flagOperation = 0;
            } else if (row == 2 && (pressed & KEY_A)) Flags_Write(1);
            else if (row == 3 && (pressed & KEY_A)) Flags_Write(0);
            break;
        case PAGE_DEBUG:
            if (row == 0 && (pressed & KEY_A)) gPracticeState.debugHud ^= 1;
            break;
        case PAGE_SETTINGS:
            if (row == 0 && (pressed & KEY_A)) {
                gPracticeState.noClip = 0;
                gPracticeState.roomExplore = 0;
                gPracticeState.invincibility = 0;
                gPracticeState.speedMode = 0;
                gPracticeState.freezeEnemies = 0;
                gPracticeState.resourceCheats = 0;
                gPracticeState.lockDirection = 0;
                gPracticeState.cameraMode = 0;
                PracticeRuntime_SetStatus("PRACTICE TOGGLES RESET");
            } else if (row == 3 && (pressed & KEY_A)) PracticeRuntime_CloseMenu();
            else if (row == 4 && ((pressed & KEY_A) || direction)) {
                gPracticeState.menuTheme=cycle_value(gPracticeState.menuTheme,3,direction ? direction : 1);
                PracticeRender_Theme();
            }
            else if (row==5 && (pressed&KEY_A)) Settings_Save();
            else if ((row==2 || row==6) && (pressed&KEY_A)) {
                gPracticeState.bindingTarget=row==6;
                gPracticeState.bindingDraft=row==6?gPracticeState.confirmHotkey:gPracticeState.menuHotkey;
                gPracticeState.cursor[PAGE_BINDINGS]=0; gPracticeState.page=PAGE_BINDINGS;
            }
            break;
        default: break;
    }
}

static void draw_root(void) {
    u32 row;
    PracticeRender_Frame("THE MINISH CAP", "PRACTICE MENU");
    for (row = 0; row < ARRAY_COUNT(sRootItems); row++) draw_item(row, sRootItems[row], 0, 0);
}

static void draw_practice(void) {
    PracticeRender_Frame("PRACTICE", 0);
    draw_item(0, "SAVE POSITION", gPracticeState.savedPosition.valid ? "READY" : "EMPTY", 0);
    draw_item(1, "LOAD POSITION", 0, 0);
    draw_item(2, "RELOAD ROOM", 0, 0);
    draw_item(3, "RELOAD AREA", 0, 0);
    draw_toggle(4, "PRACTICE TIMER", gPracticeState.timerRunning);
    draw_item(5, "RESET TIMER", 0, 0);
    draw_item(6, "CAPTURE PRESET 0", gPracticeState.preset.valid ? "READY" : "EMPTY", 0);
    draw_item(7, "APPLY PRESET 0", 0, 0);
    draw_number(8, "FRAME COUNTER", gPracticeState.frameCounter, 8);
    draw_timer(9, "TIMER", gPracticeState.timerFrames);
    draw_item(10,"SKIP CUTSCENE","FAST",3);
    PracticeRender_Text(3,14,"NATIVE EVENTS / B TO STOP",0);
    PracticeRender_Text(3,15,"STOPS AT CHOICES AND WARPS",0);
}

static void draw_player(void) {
    PracticeRender_Frame("PLAYER", 0);
    draw_item(0, "FULL HEALTH", 0, 0);
    draw_number(1, "MAX HEARTS", TMC_SAVE.stats.maxHealth >> 3, 2);
    draw_number(2, "RUPEES", TMC_SAVE.stats.rupees, 3);
}

static void draw_inventory(void) {
    static const char* const items[] = { "WEAPONS / ITEMS", "BOTTLES", "AMMO / UPGRADES", "QUEST / MAJOR", "GIVE ALL ITEMS", "DELETE ALL ITEMS" };
    u32 row;
    PracticeRender_Frame("INVENTORY", "RUNTIME RAM");
    for (row = 0; row < ARRAY_COUNT(items); row++) draw_item(row, items[row], 0, row >= 4 ? 3 : 0);
}

static void draw_weapons(void) {
    static const char* const labels[] = { "SWORD", "SHIELD", "BOMB TYPE", "BOW TYPE", "BOOMERANG", "LANTERN", "GUST JAR", "PACCI CANE", "MOLE MITTS", "ROCS CAPE", "PEGASUS BOOTS", "OCARINA" };
    static const u8 items[] = { 15, 17, 18, 19, 20, 21, 23 };
    u32 row;
    u32 selected;
    PracticeRender_Frame("WEAPONS / ITEMS", 0);
    selected = Inventory_GroupSelection(1, 6); draw_item(0, labels[0], sSwordNames[selected], 0);
    selected = Inventory_GroupSelection(13, 14); draw_item(1, labels[1], sShieldNames[selected], 0);
    selected = Inventory_GroupSelection(7, 8); draw_item(2, labels[2], sBombNames[selected], 0);
    selected = Inventory_GroupSelection(9, 10); draw_item(3, labels[3], sBowNames[selected], 0);
    selected = Inventory_GroupSelection(11, 12); draw_item(4, labels[4], sBoomNames[selected], 0);
    for (row = 5; row < 12; row++) draw_item(row, labels[row], Inventory_Get(items[row - 5]) ? "OWNED" : "NONE", 0);
}

static void draw_bottles(void) {
    u32 row;
    u32 index;
    PracticeRender_Frame("BOTTLES", "SAFE CONTENTS");
    for (row = 0; row < 4; row++) {
        draw_cursor(row);
        PracticeRender_Text(3, 3 + row, "BOTTLE", 0);
        PracticeRender_U32(10, 3 + row, row + 1, 1, 0);
        if (Inventory_Get(28 + row)) {
            index = bottle_content_index(TMC_SAVE.stats.bottles[row]);
            PracticeRender_Text(20, 3 + row, sBottleNames[index], 0);
        } else PracticeRender_Text(20, 3 + row, "NONE", 0);
    }
    draw_item(4, "REMOVE ALL BOTTLES", 0, 3);
}

static void draw_upgrades(void) {
    PracticeRender_Frame("AMMO / UPGRADES", 0);
    draw_number(0, "BOMBS", TMC_SAVE.stats.bombCount, 2);
    draw_number(1, "ARROWS", TMC_SAVE.stats.arrowCount, 2);
    draw_number(2, "BOMB BAG LEVEL", TMC_SAVE.stats.bombBagType + 1, 1);
    draw_number(3, "QUIVER LEVEL", TMC_SAVE.stats.quiverType + 1, 1);
    draw_number(4, "WALLET LEVEL", TMC_SAVE.stats.walletType + 1, 1);
    draw_number(5, "SHELLS", TMC_SAVE.stats.shells, 3);
    draw_number(6, "HEART PIECES", TMC_SAVE.stats.heartPieces, 1);
}

static void draw_quest(void) {
    static const char* const labels[] = { "QUEST SWORD", "BROKEN SWORD", "DOG FOOD", "LON LON KEY", "MUSHROOM", "LIBRARY BOOKS", "GRAVEYARD KEY", "TINGLE TROPHY", "CARLOV MEDAL", "ELEMENTS", "GRIP / BRACELETS", "FLIPPERS", "MAP" };
    static const u8 items[] = { 52,53,54,55,56,57,60,61,62,64,68,70,71 };
    u32 row;
    PracticeRender_Frame("QUEST / MAJOR", "NO STORY FLAGS");
    for (row = 0; row < 13; row++) {
        if (row == 9) draw_item(row, labels[row], "SELECT", 2);
        else draw_item(row, labels[row], Inventory_Get(items[row]) ? "YES" : "NO", 0);
    }
}

static void draw_elements(void) {
    u32 row;
    PracticeRender_Frame("ELEMENTS", "PICK SEPARATE");
    for (row = 0; row < 4; row++) {
        draw_item(row, sElementNames[row], Inventory_Get(64 + row) ? "OWNED" : "MISSING", 0);
    }
    PracticeRender_Text(3, 10, "ITEM + ELEMENT CLEAR FLAG", 2);
    PracticeRender_Text(3, 11, "OTHER STORY FLAGS KEPT", 0);
    PracticeRender_Text(3, 12, "NO BOSS / PUZZLE RESET", 0);
}

static void draw_warp(void) {
    const WarpRoomInfo* info = Warp_RoomInfo(gPracticeState.selectedArea,gPracticeState.selectedRoom);
    const TmcRoomHeader* header = Warp_GetHeader(gPracticeState.selectedArea,gPracticeState.selectedRoom);
    PracticeRender_Frame("WORLD / WARP",0);
    draw_item(0,"AREA",0,0);
    PracticeRender_Text(8,3,Warp_AreaName(gPracticeState.selectedArea),2);
    draw_item(1,"ROOM",0,0);
    PracticeRender_Text(8,4,info ? info->name : "UNNAMED ROOM",2);
    draw_item(2,"WARP TO DESTINATION",0,3);
    draw_item(3,Warp_FavoriteIndex()>=0?"REMOVE FAVORITE":"ADD FAVORITE",0,2);
    draw_item(4,"FAVORITE ROOMS","OPEN",2);
    draw_item(5,"STORY INTRO",0,2);
    draw_item(6,"ENDING / CREDITS",0,2);
    draw_item(7,"RETURN FROM REPLAY",0,gPracticeState.sceneReplay?2:0);
    if(gPracticeState.cursor[PAGE_WARP]>=5) {
        PracticeRender_Text(3,12,"REPLAY - NOT A NEW SAVE",2);
        PracticeRender_Text(3,13,"STORY PROGRESS IS RESTORED",0);
        PracticeRender_Text(3,14,"RETURNS TO YOUR ROOM",0);
        PracticeRender_Text(3,15,gPracticeState.sceneReplay?"A: RETURN TO ORIGINAL ROOM":
            (gPracticeState.sceneConfirm?"A AGAIN: CONFIRM  B:CANCEL":"A: SELECT STORY REPLAY"),3);
        return;
    }
    PracticeRender_Text(3,11,"FULL ROOM NAME",0);
    if (info) {
        char first[27]; u32 i;
        for (i=0;i<26 && info->name[i];i++) first[i]=info->name[i];
        first[i]=0; PracticeRender_Text(3,12,first,2);
        if (i==26) PracticeRender_Text(3,13,info->name+26,2);
    }
    PracticeRender_Text(3,14,"HEX",0);
    PracticeRender_Hex(6,14,gPracticeState.selectedArea,2,0);
    PracticeRender_Text(8,14,"/",0);
    PracticeRender_Hex(9,14,gPracticeState.selectedRoom,2,0);
    PracticeRender_Text(13,14,"SIZE",0);
    if (header) { PracticeRender_U32(18,14,header->width,4,0); PracticeRender_U32(23,14,header->height,4,0); }
    PracticeRender_Text(3,15,info && info->layer && header && info->x<header->width && info->y<header->height ? "SPAWN: NATIVE ENTRY" : "SPAWN: UNVERIFIED CENTER",3);
}

static void draw_flags(void) {
    const char* last = gPracticeState.flagOperation == 1 ? "SET TRUE" :
                       (gPracticeState.flagOperation == 2 ? "CLEARED FALSE" : "NONE");
    PracticeRender_Frame("RAW FLAGS", "UNKNOWN EFFECT");
    draw_item(0, "SCOPE", sFlagBanks[gPracticeState.flagBank], 3);
    draw_item(1, "BIT INDEX", 0, 3);
    PracticeRender_Hex(21, 4, gPracticeState.flagIndex, 3, 3);
    draw_item(2, "SET BIT", 0, 3);
    draw_item(3, "CLEAR BIT", 0, 3);
    PracticeRender_Text(3,7,"AREA",0); PracticeRender_U32(8,7,gPracticeState.menuArea,3,0);
    PracticeRender_Text(13,7,"ROOM",0); PracticeRender_U32(18,7,gPracticeState.menuRoom,2,0);
    PracticeRender_Text(22,7,"DGN",0); PracticeRender_U32(26,7,gPracticeState.menuDungeonIndex & 0xF,2,0);
    PracticeRender_Text(3, 8, "VALUE NOW", 0);
    PracticeRender_Text(20, 8, Flags_Read() ? "SET / TRUE" : "CLEAR / FALSE", 3);
    PracticeRender_Text(3, 10, "SCOPE MEANS", 0);
    PracticeRender_Text(3, 11, sFlagHelp[gPracticeState.flagBank], 2);
    PracticeRender_Text(3, 13, "LAST ACTION", 0);
    PracticeRender_Text(20, 13, last, gPracticeState.flagOperation ? 2 : 0);
    if (gPracticeState.flagOperation) {
        PracticeRender_Text(3,14,sFlagBanks[gPracticeState.flagChangedBank],0);
        PracticeRender_Text(16,14,"ID",0);
        PracticeRender_Hex(19,14,gPracticeState.flagChangedIndex,3,0);
        PracticeRender_Text(3,15,"READBACK",0);
        PracticeRender_U32(12,15,gPracticeState.flagBefore,1,0);
        PracticeRender_Text(14,15,"->",0);
        PracticeRender_U32(17,15,gPracticeState.flagAfter,1,2);
    }
}

static void draw_debug(void) {
    PracticeRender_Frame("DEBUG", "OPEN SNAPSHOT");
    draw_toggle(0, "GAMEPLAY HUD", gPracticeState.debugHud);
    draw_item(1, "LINK X", 0, 0); PracticeRender_S32(19, 4, gPracticeState.menuX >> 16, 5, 0);
    draw_item(2, "LINK Y", 0, 0); PracticeRender_S32(19, 5, gPracticeState.menuY >> 16, 5, 0);
    draw_item(3, "LINK Z", 0, 0); PracticeRender_S32(19, 6, gPracticeState.menuZ >> 16, 4, 0);
    draw_number(4, "DIRECTION", gPracticeState.menuDirection, 2);
    draw_number(5, "MOVE SPEED", gPracticeState.menuSpeed, 4);
    draw_number(6, "AREA ID", gPracticeState.menuArea, 3);
    draw_number(7, "ROOM ID", gPracticeState.menuRoom, 2);
    draw_number(8, "PLAYER ACTION", gPracticeState.menuAction, 2);
    draw_number(9, "FRAME STATE", gPracticeState.menuFrameState, 2);
    draw_number(10, "FLOOR TYPE", gPracticeState.menuFloorType, 2);
    draw_number(11, "ENTITY COUNT", gPracticeState.menuEntityCount, 2);
    PracticeRender_Text(17, 15, "KEYS", 0); PracticeRender_Hex(22, 15, gPracticeState.menuHeldInput, 3, 0);
}

void PracticeMenu_Draw(void) {
    switch (gPracticeState.page) {
        case PAGE_ACTORS: case PAGE_ACTOR_LIST: case PAGE_ACTOR_DETAIL:
        case PAGE_ACTOR_SPAWN: case PAGE_ACTOR_DELETE: Actors_Draw(); break;
        case PAGE_ROOT: draw_root(); break;
        case PAGE_PRACTICE: draw_practice(); break;
        case PAGE_PLAYER: draw_player(); break;
        case PAGE_INVENTORY: draw_inventory(); break;
        case PAGE_WEAPONS: draw_weapons(); break;
        case PAGE_BOTTLES: draw_bottles(); break;
        case PAGE_UPGRADES: draw_upgrades(); break;
        case PAGE_QUEST: draw_quest(); break;
        case PAGE_ELEMENTS: draw_elements(); break;
        case PAGE_CONFIRM_ALL:
            PracticeRender_Frame(gPracticeState.collectionAction==1 ? "ALL FRAGMENTS" :
                (gPracticeState.collectionAction==2 ? "ALL FIGURINES" : "GIVE ALL ITEMS"),0);
            draw_item(0, "YES - APPLY", 0, 3);
            draw_item(1, "CANCEL", 0, 0);
            if (gPracticeState.collectionAction==1) {
                PracticeRender_Text(3,8,"17 TYPES X99 + STONE BAG",2);
                PracticeRender_Text(3,10,"DOES NOT COMPLETE FUSIONS",0);
            } else if (gPracticeState.collectionAction==2) {
                PracticeRender_Text(3,8,"136 FIGURES + CARLOV MEDAL",2);
                PracticeRender_Text(3,10,"SETS COLLECTION COMPLETE",3);
                PracticeRender_Text(3,11,"KEEPS STORY / BOSS FLAGS",0);
            } else PracticeRender_Text(3,8,"NO STORY FLAGS",2);
            PracticeRender_Text(3,14,"NO AUTOMATIC SAVE",3);
            break;
        case PAGE_CONFIRM_DELETE:
            PracticeRender_Frame("DELETE ALL ITEMS", 0);
            draw_item(0, "NO - KEEP ITEMS", 0, 2);
            draw_item(1, "YES - DELETE ITEMS", 0, 3);
            PracticeRender_Text(3,7,"CLEARS OWNERSHIP / SKILLS",0);
            PracticeRender_Text(3,8,"EQUIPMENT / BOTTLES / AMMO",0);
            PracticeRender_Text(3,10,"KEEPS STORY / BOSS FLAGS",2);
            PracticeRender_Text(3,11,"HEARTS / MONEY / CAPACITY",2);
            PracticeRender_Text(3,13,"NO AUTOMATIC SAVE",3);
            break;
        case PAGE_WARP: draw_warp(); break;
        case PAGE_FAVORITES: {
            u32 i; PracticeRender_Frame("FAVORITE ROOMS",0);
            for(i=0;i<8;i++) {
                const WarpRoomInfo* info=Warp_RoomInfo(gPracticeState.favorites[i][0],gPracticeState.favorites[i][1]);
                char name[20]; u32 j;
                const char* source=info?info->name:(gPracticeState.favorites[i][0]==0xFF?"EMPTY":"UNNAMED ROOM");
                for(j=0;j<19 && source[j];j++) name[j]=source[j];
                name[j]=0;
                draw_cursor(i);
                PracticeRender_Hex(3,3+i,gPracticeState.favorites[i][0],2,0);
                PracticeRender_Text(5,3+i,"/",0);
                PracticeRender_Hex(6,3+i,gPracticeState.favorites[i][1],2,0);
                PracticeRender_Text(9,3+i,name,0);
            }
            { u8* f=gPracticeState.favorites[gPracticeState.cursor[PAGE_FAVORITES]%8];
              PracticeRender_Text(3,12,Warp_AreaName(f[0]),2);
              PracticeRender_Hex(3,13,f[0],2,0); PracticeRender_Hex(6,13,f[1],2,0); }
            PracticeRender_Text(3,14,"A: PREVIEW THEN A: WARP",2);
            PracticeRender_Text(3,15,"PERSIST: SAVE MENU SETTINGS",0);
            break;
        }
        case PAGE_BINDINGS: {
            u32 i; char keys[24];
            static const char* const labels[]={"A","B","SELECT","START","L","R"};
            static const u16 bits[]={KEY_A,KEY_B,KEY_SELECT,KEY_START,KEY_L,KEY_R};
            PracticeRender_Frame("BUTTON COMBINATIONS",0);
            draw_item(0,"EDIT",0,2);
            PracticeRender_Text(16,3,gPracticeState.bindingTarget?"100% / UNDO":"OPEN/CLOSE",2);
            for(i=0;i<6;i++) draw_toggle(i+1,labels[i],gPracticeState.bindingDraft&bits[i]);
            draw_item(7,"APPLY COMBINATION",0,2); draw_item(8,"DRAFT: DEFAULT KEYS",0,0);
            Settings_KeyText(gPracticeState.bindingDraft,keys); PracticeRender_Text(3,13,keys,2);
            PracticeRender_Text(3,14,gPracticeState.bindingTarget?"A + MODIFIER / NO B":"AT LEAST TWO BUTTONS",0);
            PracticeRender_Text(3,15,"B: CANCEL UNAPPLIED DRAFT",0);
            break;
        }
        case PAGE_MOVEMENT:
            PracticeRender_Frame("MOVEMENT", 0);
            draw_toggle(0, "NO CLIP", gPracticeState.noClip);
            draw_item(1,"MOVEMENT SPEED",(const char*[]){"NORMAL","1.5X","2X"}[gPracticeState.speedMode],0);
            draw_toggle(2,"LOCK DIRECTION",gPracticeState.lockDirection);
            draw_toggle(3,"CAMERA LOCK",gPracticeState.cameraMode == 1);
            draw_item(4,"FREE CAMERA","START",0);
            draw_item(5,"POSITION NUDGE","OPEN",0);
            draw_item(6,"GROUND / Z RESET",0,0);
            PracticeRender_Text(3,12,"NO CLIP INCLUDES ROOMS",2);
            PracticeRender_Text(3,13,"FREE CAM: DPAD R:FAST B:END",0);
            PracticeRender_Text(3,14,"CAMERA: LOADED MAP ONLY",0);
            PracticeRender_Text(3,15,"OUTSIDE ROOM MAY BE EMPTY",0);
            break;
        case PAGE_NUDGE:
            PracticeRender_Frame("POSITION NUDGE",0);
            draw_item(0,"LEFT",0,0); draw_item(1,"RIGHT",0,0);
            draw_item(2,"UP",0,0); draw_item(3,"DOWN",0,0);
            draw_item(4,"STEP",gPracticeState.nudgeStep ? "8 PIXELS" : "1 PIXEL",0);
            PracticeRender_Text(3,10,"A: ONE STEP AND RESUME",2);
            PracticeRender_Text(3,12,"NO CLIP OFF: SAFE FLOOR",0);
            PracticeRender_Text(3,13,"CHECKED ALONG WHOLE STEP",0);
            break;
        case PAGE_CHEATS:
            PracticeRender_Frame("CHEATS",0);
            draw_toggle(0,"INVINCIBILITY",gPracticeState.invincibility);
            draw_toggle(1,"ENEMY FREEZE",gPracticeState.freezeEnemies);
            draw_item(2,"BREAK FREE",0,0);
            draw_item(3,"OCARINA GLITCH",(PSTATE32(0x30)&0x10000000u)?"END":"START",3);
            draw_toggle(4,"INFINITE HEARTS",gPracticeState.resourceCheats & CHEAT_HEARTS);
            draw_toggle(5,"INFINITE BOMBS",gPracticeState.resourceCheats & CHEAT_BOMBS);
            draw_toggle(6,"INFINITE ARROWS",gPracticeState.resourceCheats & CHEAT_ARROWS);
            draw_toggle(7,"INFINITE RUPEES",gPracticeState.resourceCheats & CHEAT_RUPEES);
            draw_item(8,"ALL FRAGMENTS","GIVE",3);
            draw_item(9,"ALL FIGURINES","GIVE",3);
            draw_item(10,"COMPLETE GAME 100%",0,3);
            draw_item(11,"UNDO 100%",Completion_CanUndo()?"READY":"--",3);
            draw_toggle(12,"INFINITE TIME",gPracticeState.resourceCheats & CHEAT_MINIGAME_TIME);
            {
                static const char* const help1[11] = {
                    "BLOCKS NORMAL CONTACT HITS", "FREEZE: COMBAT STAYS ACTIVE",
                    "RELEASE CONTROL LOCK ONCE", "REAL STAIR-GLITCH STATE",
                    "FULL HEARTS - HITS STILL ON", "REFILLS OWNED BOMBS ONLY",
                    "REFILLS OWNED BOW ONLY", "REFILLS CURRENT WALLET",
                    "17 TYPES X99 - NO FUSIONS", "136 FIGURES + MEDAL", "STORY / HEARTS / COLLECTION"
                };
                static const char* const help2[11] = {
                    "HEARTS: KNOCKBACK REMAINS", "BOSSES / SPECIALS: NATIVE",
                    "SCENES MAY LOCK AGAIN", "ROLL / TALK CAN SOFTLOCK",
                    "NO REVIVE / NO SCENE SKIP", "USES CURRENT BAG CAPACITY",
                    "USES CURRENT QUIVER SIZE", "NO WALLET UPGRADE GRANTED",
                    "CONFIRM FIRST - MANUAL SAVE", "COLLECTION FLAGS INCLUDED", "WARNING: CHANGES PROGRESS"
                };
                u32 row = gPracticeState.cursor[PAGE_CHEATS] % 11;
                const char* help=gPracticeState.cursor[PAGE_CHEATS]==12?"LIMITS / EYES / BUFFS":
                    gPracticeState.cursor[PAGE_CHEATS]==11?"RAM BACKUP / LOST ON RELOAD":
                    row==1?"INCLUDES BOSS ENEMY ACTORS":row==2?"CLOSE TEXT + RELEASE CONTROL":help1[row];
                PracticeRender_Text(3,16,help,row == 3 ? 3 : 0);
                (void)help2;
            }
            break;
        case PAGE_CONFIRM_COMPLETE:
            PracticeRender_Frame("100% COMPLETION",0);
            draw_item(0,"CANCEL - KEEP MY PROGRESS",0,2);
            draw_item(1,"APPLY FULL COMPLETION",0,3);
            PracticeRender_Text(3,7,"STORY CLEAR / ALL EQUIPMENT",0);
            PracticeRender_Text(3,8,"20 HEARTS / ALL SKILLS",0);
            PracticeRender_Text(3,9,"100 FUSIONS / 136 FIGURES",0);
            PracticeRender_Text(3,11,"RELOADS THE CURRENT AREA",3);
            PracticeRender_Text(3,12,"NAME AND SAVE SLOT KEPT",0);
            PracticeRender_Text(3,13,"PERMANENT WHEN YOU SAVE",3);
            { char keys[24]; Settings_KeyText(gPracticeState.confirmHotkey,keys);
              PracticeRender_Text(3,14,"HOLD MODIFIERS / PRESS A",2); PracticeRender_Text(3,15,keys,2); }
            break;
        case PAGE_CONFIRM_UNDO:
            PracticeRender_Frame("UNDO 100% COMPLETION",0);
            draw_item(0,"CANCEL - KEEP CURRENT GAME",0,2);
            draw_item(1,"RESTORE PRE-100% BACKUP",0,3);
            PracticeRender_Text(3,7,"REVERTS ALL PROGRESS SINCE",3);
            PracticeRender_Text(3,8,"THE FIRST 100% APPLICATION",3);
            PracticeRender_Text(3,10,"RETURNS TO ORIGINAL ROOM",0);
            PracticeRender_Text(3,11,"RESOURCE CHEATS TURN OFF",0);
            PracticeRender_Text(3,12,"RAM ONLY / LOST ON RELOAD",0);
            PracticeRender_Text(3,13,"DISK: SAVE AGAIN AFTER UNDO",3);
            { char keys[24]; Settings_KeyText(gPracticeState.confirmHotkey,keys);
              PracticeRender_Text(3,14,"HOLD MODIFIERS / PRESS A",2); PracticeRender_Text(3,15,keys,2); }
            break;
        case PAGE_BETA:
            PracticeRender_Frame("BETA / UNUSED","VERIFIED USA ROM LEFTOVERS");
            draw_item(0,"UNUSED TREE ROOM","24/1F",2);
            draw_item(1,"RETURN TO GAME",gPracticeState.betaReturn.valid?"READY":"--",0);
            draw_item(2,"UNUSED SWORD ID 05","INFO",3);
            PracticeRender_Text(3,8,"ROOM: UNUSED HEART PICKUP",0);
            PracticeRender_Text(3,9,"RETURN POINT HELD IN RAM",0);
            PracticeRender_Text(3,11,"SWORD 05: NOT FUNCTIONAL",3);
            PracticeRender_Text(3,12,"NATIVE SWORD CODE REJECTS IT",0);
            PracticeRender_Text(3,14,"NO EXTERNAL BETA ROM NEEDED",0);
            PracticeRender_Text(3,15,"ONLY VERIFIED REMNANTS HERE",0);
            break;
        case PAGE_FLAGS:
            PracticeRender_Frame("FLAGS",0);
            draw_item(0,"KNOWN FLAGS","OPEN",2);
            draw_item(1,"RAW FLAGS","CAREFUL",3);
            draw_item(2,"GENERAL FLAGS","OPEN",2);
            draw_item(3,"DUNGEON FLAGS","OPEN",2);
            draw_item(4,"WIND PORTALS","OPEN",2);
            draw_item(5,"EDIT HISTORY","OPEN",2);
            PracticeRender_Text(3,10,"RAW: BITS, EFFECT UNKNOWN",3);
            PracticeRender_Text(3,12,"EDITING A FLAG DOES NOT",0);
            PracticeRender_Text(3,13,"REPLAY ITS GAME EVENT",0);
            break;
        case PAGE_ENEMIES: {
            u32 row, count=Flags_ListCount();
            PracticeRender_Frame((const char*[]){"GENERAL FLAGS","DUNGEON FLAGS","WIND PORTALS","EDIT HISTORY"}[gPracticeState.flagGroup%4],0);
            for (row=0;row<count;row++) {
                draw_cursor(row);
                PracticeRender_Text(3,3+row,Flags_ListName(row),0);
                if (gPracticeState.flagGroup==1 && row==0) PracticeRender_U32(26,3+row,Flags_ListRead(row),2,2);
                else PracticeRender_Text(27,3+row,Flags_ListRead(row)?"X":"-",Flags_ListRead(row)?2:0);
            }
            if (gPracticeState.flagGroup==3) {
                static const char* const scopes[]={"GLOBAL","LOCAL","ROOM","KEYBIT","ITEM","WARP","CLEAR","PORTAL","KEYS"};
                PracticeRender_Text(3,3,"SCOPE  ID   OLD>NEW DGN",2);
                for (row=0;row<gPracticeState.flagLogCount;row++) {
                    u32 bank=gPracticeState.flagLog[row].bank;
                    PracticeRender_Text(3,5+row,scopes[bank%9],0);
                    PracticeRender_Hex(10,5+row,gPracticeState.flagLog[row].index,3,0);
                    PracticeRender_U32(15,5+row,gPracticeState.flagLog[row].before,2,0);
                    PracticeRender_Text(17,5+row,">",0);
                    PracticeRender_U32(18,5+row,gPracticeState.flagLog[row].after,2,2);
                    if (bank==3 || bank==4 || bank==5 || bank==8) PracticeRender_Hex(23,5+row,gPracticeState.flagLog[row].dungeon,2,0);
                    else PracticeRender_Text(23,5+row,"--",0);
                }
                PracticeRender_Text(3,14,"LAST 8 MENU EDITS / RAM",0);
                PracticeRender_Text(3,15,"ID/DGN:HEX VALUES:DECIMAL",0);
            } else {
                if (gPracticeState.flagGroup==1) {
                    PracticeRender_Text(3,9,"CURRENT DUNGEON INDEX",0);
                    PracticeRender_Hex(26,9,gPracticeState.menuDungeonIndex,2,2);
                    PracticeRender_Text(3,11,"KEYS: LEFT -1 / RIGHT +1",0);
                }
                PracticeRender_Text(3,13,"A: TOGGLE   X:SET -:CLEAR",2);
                PracticeRender_Text(3,14,"EDITS DO NOT REPLAY EVENTS",3);
                PracticeRender_Text(3,15,"CHANGES LISTED IN HISTORY",0);
            }
            break;
        }
        case PAGE_RAW_FLAGS: draw_flags(); break;
        case PAGE_KNOWN_FLAGS:
            PracticeRender_Frame("KNOWN FLAGS",0);
            draw_item(0,"SELECT FLAG",0,0);
            draw_item(1,"MARK DEFEATED",0,3);
            draw_item(2,"CLEAR DEFEATED",0,3);
            draw_item(3,"RELOAD CURRENT ROOM",0,0);
            PracticeRender_Text(3,8,Flags_KnownName(),2);
            PracticeRender_Text(3,9,"GLOBAL",0);
            PracticeRender_Hex(10,9,0x31+gPracticeState.knownFlag%9,2,0);
            PracticeRender_Text(15,9,Flags_KnownRead()?"DEFEATED":"NOT SET",2);
            PracticeRender_Text(3,11,"NATIVE CHECK: ENEMY SPAWN",0);
            PracticeRender_Text(3,12,"SET NATURALLY: ENEMY DEATH",0);
            PracticeRender_Text(3,13,"CLEAR NEEDS ROOM RELOAD",0);
            PracticeRender_Text(3,14,"FUSION REQUIREMENTS REMAIN",0);
            if (!gPracticeState.flagOperation) PracticeRender_Text(3,15,"NO FLAG EDIT SELECTED",0);
            else if (gPracticeState.flagChangedBank != 0) PracticeRender_Text(3,15,"LAST EDIT WAS A RAW FLAG",0);
            else {
                PracticeRender_Text(3,15,"LAST ID",0);
                PracticeRender_Hex(11,15,gPracticeState.flagChangedIndex,3,0);
                PracticeRender_U32(16,15,gPracticeState.flagBefore,1,0);
                PracticeRender_Text(18,15,"->",0);
                PracticeRender_U32(21,15,gPracticeState.flagAfter,1,2);
            }
            break;
        case PAGE_DEBUG: draw_debug(); break;
        case PAGE_SETTINGS:
            PracticeRender_Frame("SETTINGS", 0);
            draw_item(0, "RESET PRACTICE TOGGLES", 0, 0);
            draw_item(1, "SAVE POLICY", "MANUAL", 2);
            draw_item(2, "MENU HOTKEY", "EDIT", 2);
            draw_item(3, "CLOSE MENU", 0, 0);
            draw_item(4,"MENU THEME",(const char*[]){"CLASSIC","DARK","FOREST","PLUM"}[gPracticeState.menuTheme%4],2);
            draw_item(5,"SAVE MENU SETTINGS",0,2);
            draw_item(6,"100% HOTKEY","EDIT",2);
            { char keys[24]; Settings_KeyText(gPracticeState.menuHotkey,keys);
              PracticeRender_Text(3,11,"OPEN/CLOSE:",0); PracticeRender_Text(3,12,keys,2); }
            PracticeRender_Text(3,13,"SAVES SETUP + FAVORITES",0);
            PracticeRender_Text(3,14,"DOES NOT SAVE GAME PROGRESS",0);
            break;
        default: gPracticeState.page = PAGE_ROOT; draw_root(); break;
    }
    PracticeRender_Commit();
}

void PracticeMenu_Update(void) {
    u16 pressed = TMC_INPUT.pressed;
    u16 repeat = TMC_INPUT.repeat;
    u32 count;
    u32 opening = 0;
    u32 currentPage;
    u8* cursor;
    if (gPracticeState.page >= MAX_MENU_PAGES)
        gPracticeState.page = PAGE_ROOT;
    if (!gPracticeState.menuOpen) {
        gPracticeState.sceneConfirm=0;
        gPracticeState.menuOpen = 1;
        PracticeRender_Init();
        PracticeRuntime_SetStatus(gPracticeState.modalMode ? "PAUSED VIEW - HOTKEY RETURNS" : "RUNTIME ONLY - NO AUTO SAVE");
        opening = 1;
    }
    if (gPracticeState.statusTimer != 0) gPracticeState.statusTimer--;
    if ((TMC_INPUT.held & gPracticeState.menuHotkey) == gPracticeState.menuHotkey && (pressed & gPracticeState.menuHotkey) != 0) {
        PracticeRuntime_CloseMenu();
        return;
    }
    if (pressed & KEY_B) {
        gPracticeState.sceneConfirm=0;
        go_back();
        if (!gPracticeState.menuOpen) return;
    }
    currentPage = gPracticeState.page;
    if (currentPage >= MAX_MENU_PAGES) currentPage = gPracticeState.page = PAGE_ROOT;
    count = page_count(currentPage);
    cursor = &gPracticeState.cursor[currentPage];
    if (*cursor >= count) *cursor = 0;
    if (count != 0) {
        if(repeat&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT)) gPracticeState.sceneConfirm=0;
        if ((repeat & KEY_UP) != 0) *cursor = *cursor == 0 ? (u8)(count - 1) : (u8)(*cursor - 1);
        if ((repeat & KEY_DOWN) != 0) *cursor = (u32)(*cursor + 1) >= count ? 0 : (u8)(*cursor + 1);
    }
    process_action(pressed, repeat);
    if (gPracticeState.menuOpen) {
        PracticeMenu_Draw();
        if (opening != 0 && !gPracticeState.modalMode) CALL_VOID_U32(TMC_SET_FADE_INVERTED)(0x20);
    }
}
