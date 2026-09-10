#include "practice.h"

/* Native EEPROM table occupies through FAF / 1FAF (including duplicated
 * status bytes). Two 64-byte records in FC0..FFF / 1FC0..1FFF do not overlap
 * any of the three save slots, their checksums, header or signature. */
typedef struct { u32 magic; u16 version, checksum; u8 theme, hud, page, count; u8 cursors[32]; u16 menuKeys, confirmKeys; u8 favorites[8][2]; } SettingsRecord;
_Static_assert(sizeof(SettingsRecord)==64,"EEPROM record size");
u32 Settings_ValidKeys(u16 menu, u16 confirm) {
    const u16 allowed=KEY_A|KEY_B|KEY_SELECT|KEY_START|KEY_L|KEY_R;
    return !(menu&~allowed) && menu && (menu&(menu-1)) &&
        !(confirm&~(allowed&~KEY_B)) && (confirm&KEY_A) && (confirm&~KEY_A) &&
        (confirm&menu)!=menu;
}
void Settings_KeyText(u16 keys, char* text) {
    static const u16 bits[]={KEY_L,KEY_R,KEY_SELECT,KEY_START,KEY_A,KEY_B};
    static const char* const names[]={"L","R","SEL","START","A","B"};
    u32 i,n=0;
    for(i=0;i<6;i++) if(keys&bits[i]) {
        const char* p=names[i]; if(n) text[n++]='+';
        while(*p) text[n++]=*p++;
    }
    if(!n) { text[n++]='-'; }
    text[n]=0;
}
static u16 checksum(const SettingsRecord* r) {
    const u8* p=(const u8*)r; u32 i; u16 crc=0xFFFF;
    for (i=8;i<64;i++) { u32 bit; crc^=(u16)p[i]<<8; for(bit=0;bit<8;bit++) crc=(crc&0x8000)?(crc<<1)^0x1021:crc<<1; }
    return crc;
}
static u32 valid(const SettingsRecord* r) {
    u32 i;
    if (!(r->magic==0x54455350u && (r->version==1 || r->version==2) && r->count>0 && r->count<=MAX_MENU_PAGES &&
        r->theme<4 && r->hud<2 && r->page<r->count && r->checksum==checksum(r))) return 0;
    if(r->version==2) {
        if(!Settings_ValidKeys(r->menuKeys,r->confirmKeys)) return 0;
        for(i=0;i<8;i++) if(!(r->favorites[i][0]==0xFF && r->favorites[i][1]==0xFF) &&
            !Warp_IsValid(r->favorites[i][0],r->favorites[i][1])) return 0;
    }
    return 1;
}
void Settings_Load(void) {
    SettingsRecord r; u32 slot,i;
    for(slot=0;slot<2;slot++) {
        if (((u32(*)(u32,void*,u32))0x0807D1D9u)(0xFC0+slot*0x1000,&r,64) && valid(&r)) {
            gPracticeState.menuTheme=r.theme; gPracticeState.debugHud=r.hud; gPracticeState.page=r.page;
            for(i=0;i<r.count;i++) gPracticeState.cursor[i]=r.cursors[i];
            if(r.version==2) {
                gPracticeState.menuHotkey=r.menuKeys; gPracticeState.confirmHotkey=r.confirmKeys;
                Practice_CopyBytes(r.favorites,gPracticeState.favorites,sizeof(r.favorites));
            }
            /* Never reopen a destructive confirmation or unfinished key draft on boot. */
            if(gPracticeState.page==PAGE_CONFIRM_COMPLETE || gPracticeState.page==PAGE_CONFIRM_UNDO)
                gPracticeState.page=PAGE_CHEATS;
            if(gPracticeState.page==PAGE_BINDINGS) gPracticeState.page=PAGE_SETTINGS;
            if(gPracticeState.page==PAGE_ACTOR_DETAIL || gPracticeState.page==PAGE_ACTOR_DELETE)
                gPracticeState.page=PAGE_ACTOR_LIST;
            return;
        }
    }
}
void Settings_Save(void) {
    SettingsRecord r, check; u32 slot,i,good=0;
    Practice_ClearBytes(&r,sizeof(r)); r.magic=0x54455350u; r.version=2;
    r.theme=gPracticeState.menuTheme; r.hud=gPracticeState.debugHud;
    r.page=gPracticeState.page; r.count=MAX_MENU_PAGES;
    for(i=0;i<MAX_MENU_PAGES;i++) r.cursors[i]=gPracticeState.cursor[i];
    r.menuKeys=gPracticeState.menuHotkey; r.confirmKeys=gPracticeState.confirmHotkey;
    Practice_CopyBytes(gPracticeState.favorites,r.favorites,sizeof(r.favorites));
    r.checksum=checksum(&r);
    /* Same DMA/audio exclusion as native HandleSaveInProgress. */
    ((void(*)(void))0x0805616Du)();
    for(slot=0;slot<2;slot++) {
        if (((u32(*)(u32,const void*,u32))0x0807D20Du)(0xFC0+slot*0x1000,&r,64) &&
            ((u32(*)(u32,void*,u32))0x0807D1D9u)(0xFC0+slot*0x1000,&check,64) && valid(&check) && check.checksum==r.checksum) good++;
    }
    ((void(*)(void))0x08056209u)();
    PracticeRuntime_SetStatus(good==2?"MENU SETTINGS SAVED":(good?"SETTINGS: ONE COPY SAVED":"SETTINGS SAVE FAILED"));
}
