#include "practice.h"

enum {
    PAL_NORMAL = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_DANGER = 3,
};

static const u8 sDigits[10][7] = {
    { 14, 17, 19, 21, 25, 17, 14 }, { 4, 12, 4, 4, 4, 4, 14 },
    { 14, 17, 1, 2, 4, 8, 31 },     { 30, 1, 1, 14, 1, 1, 30 },
    { 2, 6, 10, 18, 31, 2, 2 },     { 31, 16, 16, 30, 1, 1, 30 },
    { 6, 8, 16, 30, 17, 17, 14 },   { 31, 1, 2, 4, 8, 8, 8 },
    { 14, 17, 17, 14, 17, 17, 14 }, { 14, 17, 17, 15, 1, 2, 12 },
};

static const u8 sLetters[26][7] = {
    { 14,17,17,31,17,17,17 }, { 30,17,17,30,17,17,30 },
    { 14,17,16,16,16,17,14 }, { 30,17,17,17,17,17,30 },
    { 31,16,16,30,16,16,31 }, { 31,16,16,30,16,16,16 },
    { 14,17,16,23,17,17,15 }, { 17,17,17,31,17,17,17 },
    { 14,4,4,4,4,4,14 },      { 7,2,2,2,2,18,12 },
    { 17,18,20,24,20,18,17 }, { 16,16,16,16,16,16,31 },
    { 17,27,21,21,17,17,17 }, { 17,25,21,19,17,17,17 },
    { 14,17,17,17,17,17,14 }, { 30,17,17,30,16,16,16 },
    { 14,17,17,17,21,18,13 }, { 30,17,17,30,20,18,17 },
    { 15,16,16,14,1,1,30 },   { 31,4,4,4,4,4,4 },
    { 17,17,17,17,17,17,14 }, { 17,17,17,17,17,10,4 },
    { 17,17,17,21,21,21,10 }, { 17,17,10,4,10,17,17 },
    { 17,17,10,4,4,4,4 },     { 31,1,2,4,8,16,31 },
};

static u8 punctuation_row(char character, u32 row) {
    static const u8 question[7] = { 14, 17, 1, 2, 4, 0, 4 };
    static const u8 slash[7] = { 1, 1, 2, 4, 8, 16, 16 };
    static const u8 greater[7] = { 16, 8, 4, 2, 4, 8, 16 };
    static const u8 less[7] = { 1, 2, 4, 8, 4, 2, 1 };
    static const u8 plus[7] = { 0, 4, 4, 31, 4, 4, 0 };
    static const u8 bracketL[7] = { 14, 8, 8, 8, 8, 8, 14 };
    static const u8 bracketR[7] = { 14, 2, 2, 2, 2, 2, 14 };
    switch (character) {
        case '-': return row == 3 ? 31 : 0;
        case '_': return row == 6 ? 31 : 0;
        case '|': return 4;
        case '+': return plus[row];
        case '>': return greater[row];
        case '<': return less[row];
        case '/': return slash[row];
        case '?': return question[row];
        case '[': return bracketL[row];
        case ']': return bracketR[row];
        case ':': return row == 2 || row == 5 ? 4 : 0;
        case '.': return row == 6 ? 4 : 0;
        case ',': return row == 5 ? 4 : (row == 6 ? 8 : 0);
        case '!': return row < 5 || row == 6 ? 4 : 0;
        case '=': return row == 2 || row == 4 ? 31 : 0;
        case '*': return row == 2 ? 21 : (row == 3 ? 14 : 0);
        default: return 0;
    }
}

static u8 glyph_row(char character, u32 row) {
    if (character >= '0' && character <= '9') {
        return sDigits[(u32)(character - '0')][row];
    }
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - ('a' - 'A'));
    }
    if (character >= 'A' && character <= 'Z') {
        return sLetters[(u32)(character - 'A')][row];
    }
    return punctuation_row(character, row);
}

void PracticeRender_BuildFontAt(u32 baseTile) {
    volatile u32* tiles = (volatile u32*)ADDR_VRAM_CHARBLOCK3 + baseTile * 8;
    u32 character;
    u32 row;
    u32 column;
    u32 pixels;
    u8 bits;
    for (character = 32; character < 96; character++) {
        for (row = 0; row < 8; row++) {
            pixels = 0;
            bits = row < 7 ? glyph_row((char)character, row) : 0;
            for (column = 0; column < 5; column++) {
                if ((bits & (1u << (4 - column))) != 0) {
                    pixels |= 1u << ((column + 1) * 4);
                }
            }
            tiles[(character - 32) * 8 + row] = pixels;
        }
    }
}

static void palette_set(u32 index, u16 color) {
    ((volatile u16*)ADDR_G_PALETTE_BUFFER)[index] = color;
    ((volatile u16*)ADDR_BG_PALETTE)[index] = color;
}

void PracticeRender_HudFont(void) {
    /* Native UI: C000-CFFF; dialog gfx: D040-DD3F; maps: E000 onward.
     * 22 compact glyphs fit exactly in the unused DD40-DFFF tail. */
    static const char alphabet[] = " 0123456789TARXYZFP+-:";
    volatile u32* tiles = (volatile u32*)0x0600DD40u;
    volatile u16* palette = (volatile u16*)ADDR_G_PALETTE_BUFFER + 240;
    u32 color = 1, dark = 1, light = 0, shade = 94, i, row, column;
    u32 signature;
    for (i = 1; i < 16; i++) {
        u32 c = palette[i];
        u32 value = (c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31);
        if (value > light) { light = value; color = i; }
        if (value < shade) { shade = value; dark = i; }
    }
    signature = color | (dark << 4);
    /* Never rebuild visible VRAM every frame: it tears during scanout.
     * Native UI/dialog reloads do not own this tail; a sentinel catches clears.
     * These static cache values are initialized via PracticeState, not .data. */
    if (gPracticeState.hudFontReady == signature &&
        tiles[8] != 0 && tiles[11 * 8] != 0) return;
    gPracticeState.hudFontReady = (u8)signature;
    for (i = 0; i < sizeof(alphabet) - 1; i++) {
        for (row = 0; row < 8; row++) {
            u32 bits = row < 7 ? glyph_row(alphabet[i], row) : 0;
            u32 pixels = 0;
            for (column = 0; column < 7; column++) {
                u32 mask = 1u << (6 - column);
                u32 line = bits << 1;
                u32 outline = line | (line << 1) | (line >> 1);
                if (row > 0) outline |= glyph_row(alphabet[i], row - 1) << 1;
                if (row < 6) outline |= glyph_row(alphabet[i], row + 1) << 1;
                if (line & mask) pixels |= color << (column * 4);
                else if (outline & mask) pixels |= dark << (column * 4);
            }
            tiles[i * 8 + row] = pixels;
        }
    }
}

void PracticeRender_Theme(void) {
    /* Only the menu-owned four palettes; gameplay and HUD are restored normally. */
    static const u16 themes[4][5] = {
        {0x30C3,0x7BDE,0x0BDF,0x338A,0x215F},
        {0x0841,0x7FFF,0x03FF,0x3FE0,0x3DFF},
        {0x10A1,0x7FFF,0x1BFF,0x3FEA,0x3DFF},
        {0x1C46,0x7FFF,0x5FFF,0x3FEA,0x3DFF},
    };
    u32 i;
    const u16* colors = themes[gPracticeState.menuTheme % 4];
    for (i=0;i<4;i++) { palette_set(i*16,colors[0]); palette_set(i*16+1,colors[i+1]); }
}

void PracticeRender_Init(void) {
    PracticeRender_BuildFontAt(0);
    PracticeRender_Theme();
    *(volatile u32*)ADDR_G_USED_PALETTES = 0x0000000Fu;
    TMC_SCREEN.displayControl = 0x0100;
    TMC_SCREEN.displayControlMask = 0xFFFF;
    TMC_SCREEN.bg0.control = 0x1F0C;
    TMC_SCREEN.bg0.xOffset = 0;
    TMC_SCREEN.bg0.yOffset = 0;
    TMC_SCREEN.bg0.subTileMap = (void*)ADDR_G_BG0_BUFFER;
    TMC_SCREEN.bg0.updated = 1;
}

void PracticeRender_Clear(void) {
    volatile u16* map = (volatile u16*)ADDR_G_BG0_BUFFER;
    u32 index;
    for (index = 0; index < 0x400; index++) {
        map[index] = 0;
    }
}

void PracticeRender_Text(u32 x, u32 y, const char* text, u32 palette) {
    volatile u16* map = (volatile u16*)ADDR_G_BG0_BUFFER;
    u32 position = y * 32 + x;
    u8 character;
    while (*text != '\0' && x < 30) {
        character = (u8)*text++;
        if (character >= 'a' && character <= 'z') character -= 32;
        if (character < 32 || character >= 96) character = '?';
        map[position++] = (u16)((character - 32) | ((palette & 0xF) << 12));
        x++;
    }
}

void PracticeRender_U32(u32 x, u32 y, u32 value, u32 digits, u32 palette) {
    char buffer[11];
    u32 index = digits;
    if (digits > 10) digits = 10;
    buffer[digits] = '\0';
    while (index != 0) {
        index--;
        buffer[index] = (char)('0' + value % 10);
        value /= 10;
    }
    PracticeRender_Text(x, y, buffer, palette);
}

void PracticeRender_S32(u32 x, u32 y, s32 value, u32 digits, u32 palette) {
    if (value < 0) {
        PracticeRender_Text(x, y, "-", palette);
        PracticeRender_U32(x + 1, y, (u32)(-value), digits, palette);
    } else {
        PracticeRender_Text(x, y, "+", palette);
        PracticeRender_U32(x + 1, y, (u32)value, digits, palette);
    }
}

void PracticeRender_Hex(u32 x, u32 y, u32 value, u32 digits, u32 palette) {
    char buffer[9];
    u32 index;
    u32 nibble;
    if (digits > 8) digits = 8;
    buffer[digits] = '\0';
    for (index = 0; index < digits; index++) {
        nibble = (value >> ((digits - index - 1) * 4)) & 0xF;
        buffer[index] = (char)(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
    PracticeRender_Text(x, y, buffer, palette);
}

void PracticeRender_Frame(const char* title, const char* subtitle) {
    u32 x;
    u32 y;
    PracticeRender_Clear();
    PracticeRender_Text(0, 0, "+", PAL_GOLD);
    PracticeRender_Text(29, 0, "+", PAL_GOLD);
    PracticeRender_Text(0, 18, "+", PAL_GOLD);
    PracticeRender_Text(29, 18, "+", PAL_GOLD);
    for (x = 1; x < 29; x++) {
        PracticeRender_Text(x, 0, "-", PAL_GOLD);
        PracticeRender_Text(x, 2, "-", PAL_GOLD);
        PracticeRender_Text(x, 18, "-", PAL_GOLD);
    }
    for (y = 1; y < 18; y++) {
        PracticeRender_Text(0, y, "|", PAL_GOLD);
        PracticeRender_Text(29, y, "|", PAL_GOLD);
    }
    PracticeRender_Text(2, 1, title, PAL_GOLD);
    if (subtitle != 0) PracticeRender_Text(17, 1, subtitle, PAL_NORMAL);
    PracticeRender_Text(2, 17, gPracticeState.status, gPracticeState.statusTimer ? PAL_GOOD : PAL_NORMAL);
    PracticeRender_Text(1, 19, "A:OK B:BACK HOTKEY:CLOSE", PAL_NORMAL);
}

void PracticeRender_Commit(void) {
    TMC_SCREEN.bg0.updated = 1;
}
