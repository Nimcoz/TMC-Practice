#include "practice.h"

#define HUD_FONT_BASE_TILE 0xEAu
#define HUD_TILE_PALETTE   0xF000u

static u32 hud_ready(void) {
    return TMC_SCREEN.bg0.subTileMap == (void*)ADDR_G_BG0_BUFFER &&
           (TMC_SCREEN.bg0.control & 0x000Cu) == 0x000Cu &&
           (TMC_SCREEN.displayControl & 0x0100u) != 0;
}

static void restore_entries(void) {
    volatile u16* map = (volatile u16*)ADDR_G_BG0_BUFFER;
    u32 index;
    for (index = 0; index < gPracticeState.hudEntryCount; index++) {
        map[gPracticeState.hudPositions[index]] = gPracticeState.hudBackup[index];
    }
    if (gPracticeState.hudEntryCount != 0) {
        TMC_SCREEN.bg0.updated = 1;
    }
    gPracticeState.hudEntryCount = 0;
}

static void ensure_font(void) {
    PracticeRender_HudFont();
}

static void put_character(u32 x, u32 y, char character) {
    volatile u16* map = (volatile u16*)ADDR_G_BG0_BUFFER;
    u32 slot;
    u32 position;
    u32 glyph = 0;
    static const char alphabet[] = " 0123456789TARXYZFP+-:";
    if (x >= 30 || y >= 20 || gPracticeState.hudEntryCount >= 64) return;
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    if (character < 32 || character >= 96) character = '?';
    while (alphabet[glyph] && alphabet[glyph] != character) glyph++;
    if (!alphabet[glyph]) glyph = 0;
    slot = gPracticeState.hudEntryCount++;
    position = ((y + (TMC_SCREEN.bg0.yOffset >> 3)) & 31) * 32 +
               ((x + (TMC_SCREEN.bg0.xOffset >> 3)) & 31);
    gPracticeState.hudPositions[slot] = (u16)position;
    gPracticeState.hudBackup[slot] = map[position];
    map[position] = (u16)(HUD_TILE_PALETTE | (HUD_FONT_BASE_TILE + glyph));
}

static void put_unsigned(u32 x, u32 y, u32 value, u32 digits) {
    char buffer[10];
    u32 index;
    if (digits > sizeof(buffer)) digits = sizeof(buffer);
    index = digits;
    while (index != 0) {
        index--;
        buffer[index] = (char)('0' + value % 10);
        value /= 10;
    }
    for (index = 0; index < digits; index++) put_character(x + index, y, buffer[index]);
}

static void put_signed(u32 x, u32 y, s32 value, u32 digits) {
    u32 magnitude;
    u32 maximum = 1;
    u32 count = digits;
    if (value < 0) {
        put_character(x, y, '-');
        magnitude = (u32)(-value);
    } else {
        put_character(x, y, '+');
        magnitude = (u32)value;
    }
    while (count-- != 0) maximum *= 10;
    if (magnitude >= maximum) magnitude %= maximum;
    put_unsigned(x + 1, y, magnitude, digits);
}

static void draw_timer(void) {
    u32 frames = gPracticeState.timerFrames;
    u32 minutes = frames / 3600;
    if (minutes > 99) minutes = 99;
    put_character(10, 0, 'T');
    put_unsigned(11, 0, minutes, 2);
    put_character(13, 0, ':');
    put_unsigned(14, 0, (frames / 60) % 60, 2);
    put_character(16, 0, ':');
    put_unsigned(17, 0, frames % 60, 2);
}

static void draw_debug(void) {
    put_character(20, 5, 'A');
    put_unsigned(21, 5, TMC_ROOM.area, 3);
    put_character(25, 5, 'R');
    put_unsigned(26, 5, TMC_ROOM.room, 2);

    put_character(20, 6, 'X');
    put_signed(21, 6, (s32)PLAYER32(0x2C) >> 16, 5);
    put_character(20, 7, 'Y');
    put_signed(21, 7, (s32)PLAYER32(0x30) >> 16, 5);
    put_character(20, 8, 'Z');
    put_signed(21, 8, (s32)PLAYER32(0x34) >> 16, 4);

    put_character(20, 9, 'F');
    put_unsigned(21, 9, PSTATE8(0x12), 2);
    put_character(25, 9, 'P');
    put_unsigned(26, 9, PLAYER8(0x0C), 2);
}

void PracticeHud_BeforeGame(void) {
    restore_entries();
}

void PracticeHud_AfterGame(void) {
    if (!hud_ready()) return;
    if (!gPracticeState.timerRunning && gPracticeState.timerFrames == 0 &&
        !gPracticeState.debugHud) return;
    ensure_font();
    if (gPracticeState.timerRunning || gPracticeState.timerFrames != 0) draw_timer();
    if (gPracticeState.debugHud) draw_debug();
    if (gPracticeState.hudEntryCount != 0) TMC_SCREEN.bg0.updated = 1;
}
