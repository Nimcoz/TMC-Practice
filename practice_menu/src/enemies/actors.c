#include "practice.h"
#include "actor_names.h"

#define E32(e,o) (*(volatile u32*)((e)+(o)))
#define E16(e,o) (*(volatile u16*)((e)+(o)))
#define PM gPracticeState
#define ENTITY_BASE 0x030015A0u
#define MANAGER_BASE 0x02033290u

typedef struct { u8 kind, id, type; const char* name; } Spawn;
/* Quick presets remain available alongside the separate native raw spawner. */
static const Spawn spawns[] = {
    {3,0,0,"OCTOROK"}, {3,8,0,"KEESE"}, {3,0x2C,0,"ROPE"},
    {6,0,0x5F,"HEART PICKUP"}, {6,0,0x54,"GREEN RUPEE"},
    {3,1,0,"CHUCHU"}, {3,2,0,"LEEVER"}, {3,0x0B,0,"SPINY CHUCHU"},
    {3,0x12,0,"PUFFSTOOL"}, {3,0x1E,0,"SPIKED BEETLE"}, {3,0x26,0,"TEKTITE"},
    {3,0x30,0,"KEATON"}, {3,0x31,0,"CROW"}, {3,0x32,0,"MULLDOZER"},
    {3,0x34,0,"WISP"}, {3,0x22,0,"BOB-OMB"}, {3,5,0,"DARKNUT"},
    {6,0,0x55,"BLUE RUPEE / 5"}, {6,0,0x56,"RED RUPEE / 20"},
    {6,0,0x57,"BIG RUPEE / 50"}, {6,0,0x58,"BIG RUPEE / 100"},
    {6,0,0x5D,"BOMBS / 5"}, {6,0,0x5E,"ARROWS / 5"},
    {6,0,0x61,"SHELL / 1"}, {6,0,0x6C,"BOMBS / 10"}, {6,0,0x6E,"ARROWS / 10"},
};
const u32 gPracticeSpawnCount=ARRAY_COUNT(spawns);
static const char* const filters[]={"ALL","ENEMIES","NPCS","OBJECTS","PROJECTILES","PLAYER / ITEMS","MANAGERS"};
static const u8 kinds[]={0,3,7,6,4,8,9};

static volatile u8* slot(u32 n) {
    if(n<80) return (volatile u8*)(ADDR_G_PLAYER_ENTITY+n*0x88);
    if(n<112) return (volatile u8*)(MANAGER_BASE+(n-80)*0x40);
    return 0;
}
static u32 live(volatile u8* e) {
    return e && (s32)E32(e,0)>0 && E32(e,4) && !(e[0x10]&0x10);
}
static s32 mark_index(volatile u8* e) {
    u32 d=(uptr)e-ENTITY_BASE;
    if(d<72*0x88 && d%0x88==0) return d/0x88;
    d=(uptr)e-ADDR_G_PLAYER_ENTITY;
    if(d<8*0x88 && d%0x88==0) return 72+d/0x88;
    d=(uptr)e-MANAGER_BASE;
    return d<32*0x40 && d%0x40==0 ? (s32)(80+d/0x40) : -1;
}
static volatile u8* selected(void) {
    u32 n;
    for(n=0;n<112;n++) {
        volatile u8* e=slot(n);
        if((uptr)e==PM.actorSelected && live(e) && E32(e,8)==PM.actorIdentity) return e;
    }
    return 0;
}
static u32 matches(volatile u8* e) {
    u32 f=PM.actorFilter;
    return live(e) && (f==0 || (f<7 && (e[8]==kinds[f] || (f==5 && e[8]==1))));
}
static volatile u8* nth(u32 index) {
    u32 n;
    for(n=0;n<112;n++) if(matches(slot(n))) { if(!index--) return slot(n); }
    return 0;
}
u32 Actors_Count(void) {
    u32 n,count=0;
    for(n=0;n<112;n++) if(matches(slot(n))) count++;
    return count;
}
const char* Actors_NativeName(u32 kind,u32 id) {
    if(kind>=10 || id>=actorNameCounts[kind]) return "NO NATIVE HANDLER";
    return actorNames[kind][id];
}
const char* Actors_GroundItemName(u32 type) {
    return type<ARRAY_COUNT(groundItemNames)?groundItemNames[type]:"UNKNOWN / RAW ITEM TYPE";
}
static const char* name(volatile u8* e) {
    return Actors_NativeName(e[8],e[9]);
}
static u32 can_delete(volatile u8* e) {
    return live(e) && mark_index(e)>=0;
}
u32 Actors_Frozen(void* entity) {
    s32 i;
    if(PM.magic!=PRACTICE_MAGIC || !PM.actorContext) return 0;
    i=mark_index(entity);
    return i>=0 && (PM.actorMarks[i]&1) && live(entity);
}
static void capture_pose(volatile u8* e) {
    s32 i=mark_index(e); u32 a;
    if(i>=0 && i<80) for(a=0;a<3;a++) PM.actorPose[i][a]=E32(e,0x2C+4*a);
}
static void restore_pose(volatile u8* e) {
    s32 i=mark_index(e); u32 a;
    if(i>=0 && i<80) for(a=0;a<3;a++) E32(e,0x2C+4*a)=PM.actorPose[i][a];
}
void Actors_RestorePoses(void) {
    u32 n,a;
    if(!PM.actorContext) return;
    for(n=0;n<80;n++) if(PM.actorMarks[n]&1) {
        volatile u8* e=n<72?slot(n+8):slot(n-72);
        if(live(e)) for(a=0;a<3;a++) E32(e,0x2C+4*a)=PM.actorPose[n][a];
    }
}
void Actors_UpdateDispatch(void* entity) {
    volatile u8* e=entity;
    static const u32 update[10]={0x0805E781,0x08016F29,0x0805E781,0x080011C5,
        0x08016AE5,0x0805E781,0x080174A5,0x08017531,0x08017339,0x08017509};
    if(Actors_Frozen(entity)) {
        if(e[8]!=9) { restore_pose(e); CALL_VOID1(0x0800404Du)(entity); }
        return;
    }
    if(e[8]<10) Minigame_ActorDispatch(entity,(void(*)(void*))update[e[8]]);
}
void Actors_Context(void) {
    /* Native subtasks detach the gameplay lists and temporarily clear RoomControls.
     * The actors stay allocated. Only inspect the restored gameplay context. */
    if(TMC_MAIN.state==GAMETASK_MAIN && TMC_MAIN.substate!=GAMEMAIN_UPDATE) return;
    if(TMC_MAIN.state!=GAMETASK_MAIN || TMC_TRANSITION.transitioningOut ||
       TMC_ROOM.scrollAction!=1 || PM.actorArea!=TMC_ROOM.area || PM.actorRoom!=TMC_ROOM.room) {
        PM.actorSelected=0;
        PM.actorContext=0;
        Practice_ClearBytes(PM.actorMarks,sizeof(PM.actorMarks));
    }
    PM.actorArea=TMC_ROOM.area; PM.actorRoom=TMC_ROOM.room;
}
/* Exact displaced native instructions; LDR/BX preserve CMP flags for BEQ.
 * All graphics, palette, script, heap and linked-list cleanup stays native. */
__attribute__((naked,noinline)) static void native_delete(void* e __attribute__((unused))) {
    __asm__ volatile("push {r4,r5,lr}\n add r4,r0,#0\n ldr r0,[r4,#4]\n cmp r0,#0\n ldr r3,=0x0805E7C5\n bx r3\n");
}
void Actors_DeleteDispatch(void* entity) {
    s32 i=mark_index(entity);
    if(PM.magic==PRACTICE_MAGIC) {
        if(i>=0) { if((PM.actorMarks[i]&1) && PM.actorContext) PM.actorContext--; PM.actorMarks[i]=0; }
        if(PM.actorSelected==(uptr)entity) PM.actorSelected=0;
    }
    native_delete(entity);
}
__attribute__((naked,noinline)) static void native_delete_manager(void* e __attribute__((unused))) {
    __asm__ volatile("push {r4,lr}\n add r4,r0,#0\n ldr r0,[r4,#4]\n cmp r0,#0\n ldr r3,=0x0805E909\n bx r3\n");
}
void Actors_DeleteManagerDispatch(void* entity) {
    s32 i=mark_index(entity);
    if(PM.magic==PRACTICE_MAGIC) {
        if(i>=0) { if((PM.actorMarks[i]&1) && PM.actorContext) PM.actorContext--; PM.actorMarks[i]=0; }
        if(PM.actorSelected==(uptr)entity) PM.actorSelected=0;
    }
    native_delete_manager(entity);
}

/* ItemOnGround_SetFlagAndDelete, verified USA entry 08081404. Replay its
 * conditional branch with absolute continuations; native flags/deletion remain
 * native. Only an actually collected Practice-created element gains its bit. */
__attribute__((naked,noinline)) static void native_item_cleanup(void* e __attribute__((unused)),u32 collected __attribute__((unused))) {
    __asm__ volatile("push {lr}\n cmp r1,#0\n beq 1f\n add r1,r0,#0\n ldr r3,=0x0808140D\n bx r3\n 1: ldr r3,=0x08081419\n bx r3\n");
}
void Actors_ItemPickupDispatch(void* entity,u32 collected) {
    volatile u8* e=entity;
    s32 i=mark_index(e);
    if(collected && PM.magic==PRACTICE_MAGIC && i>=0 && (PM.actorMarks[i]&2) && e[8]==6 && e[9]==0)
        Inventory_ElementFlag(e[10],1);
    native_item_cleanup(entity,collected);
}

static void spawn_raw(void) {
    /* Native dispatch-table extents, NOT a curated entity/variant allowlist.
     * All type/type2 bytes are passed through. Context-dependent combinations
     * can crash in Nintendo code: this is the explicitly confirmed expert path. */
    static const u16 counts[10]={0,1,0,103,37,0,194,128,25,58};
    static const u8 lists[10]={8,1,8,4,5,8,6,7,2,6};
    u32 k=PM.actorRawKind,n; volatile u8* e=0; volatile u8* parent=0;
    if(k>=10 || PM.actorRawId>=counts[k] || (k==9 && !PM.actorRawId)) {
        PracticeRuntime_SetStatus("NO NATIVE HANDLER FOR ID"); return;
    }
    if(PM.actorRawParent==1) parent=(volatile u8*)ADDR_G_PLAYER_ENTITY;
    if(PM.actorRawParent==2) { parent=selected(); if(!parent) {
        PracticeRuntime_SetStatus("SELECTED PARENT IS GONE"); return;
    } }
    for(n=k==9?80:8;n<(k==9?112:80);n++) if(!E32(slot(n),0)) { e=slot(n); break; }
    if(!e) { PracticeRuntime_SetStatus("NATIVE ENTITY POOL FULL"); return; }
    Practice_ClearBytes((void*)e,k==9?0x40:0x88);
    e[8]=k; e[9]=PM.actorRawId; e[10]=PM.actorRawType; e[11]=PM.actorRawType2;
    e[14]=PM.actorRawTimer; e[15]=PM.actorRawSubtimer; e[16]=PM.actorRawFlags;
    E32(e,k==9?0x14:0x50)=(uptr)parent;
    if(k!=9) {
        E32(e,0x2C)=PM.menuX+(32u<<16); E32(e,0x30)=PM.menuY; E32(e,0x34)=PM.menuZ;
        e[0x38]=PM.actorRawLayer?PM.actorRawLayer:PM.menuCollisionLayer;
        if(k==8) e[16]|=0x80;
    }
    ((void(*)(void*,u32))0x0805EA2Du)((void*)e,lists[k]);
    PM.actorMarks[mark_index(e)]=2;
    PM.actorSelected=(uptr)e; PM.actorIdentity=E32(e,8); PM.actorExpert=1;
    PracticeRuntime_SetStatus("RAW SPAWN - NATIVE INIT NEXT");
}

void Actors_Apply(u32 action) {
    volatile u8* e;
    if(TMC_TRANSITION.transitioningOut || TMC_ROOM.area!=PM.menuArea || TMC_ROOM.room!=PM.menuRoom) {
        PracticeRuntime_SetStatus("ACTORS: SCENE CHANGED"); return;
    }
    if(action==PENDING_LINK_TO_ACTOR || action==PENDING_ACTOR_TO_LINK) {
        volatile u8 *from,*to,*link=(volatile u8*)ADDR_G_PLAYER_ENTITY;
        u32 a;
        e=selected();
        if(!e || !live(link)) { PracticeRuntime_SetStatus("ACTOR OR LINK IS GONE"); return; }
        if(e[8]==9) { PracticeRuntime_SetStatus("MANAGER HAS NO GENERIC XYZ"); return; }
        from=action==PENDING_LINK_TO_ACTOR?e:link;
        to=action==PENDING_LINK_TO_ACTOR?link:e;
        /* Same-room exact XYZ/layer copy. No warp, action reset, script abort,
         * thawing, or floor clamp. Native attachments may move again on update. */
        for(a=0;a<3;a++) E32(to,0x2C+4*a)=E32(from,0x2C+4*a);
        to[0x38]=from[0x38]; capture_pose(to);
        PracticeRuntime_SetStatus(action==PENDING_LINK_TO_ACTOR?"LINK TELEPORTED TO ACTOR":"ACTOR TELEPORTED TO LINK");
        return;
    }
    if(action==PENDING_ACTOR_DELETE) {
        e=selected();
        if(!can_delete(e)) { PracticeRuntime_SetStatus("ACTOR PROTECTED OR GONE"); return; }
        PM.actorExpert=1;
        CALL_VOID1(e[8]==9?0x0805E901u:0x0805E7BDu)((void*)e);
        PracticeRuntime_SetStatus("ACTOR REMOVED - NOT A KILL"); return;
    }
    if(action==PENDING_ACTOR_SPAWN && PM.actorRaw) { spawn_raw(); return; }
    if(action==PENDING_ACTOR_SPAWN && PM.actorSpawn<ARRAY_COUNT(spawns)) {
        u32 n,free=0; s32 x,y; const Spawn* s=&spawns[PM.actorSpawn];
        static const s8 delta[4][2]={{0,-32},{32,0},{0,32},{-32,0}};
        if(!Movement_IsControlled() || (PSTATE32(0x30)&0x80)) {
            PracticeRuntime_SetStatus("SPAWN: NORMAL GROUND ONLY"); return;
        }
        for(n=8;n<80;n++) if(!E32(slot(n),0)) free++;
        /* Keep engine reserve and prohibit native allocation's eviction path. */
        if(free<8 || *(volatile u8*)0x03003DBCu>=64) {
            PracticeRuntime_SetStatus("SPAWN: ENTITY RESERVE FULL"); return;
        }
        x=(s32)PLAYER32(0x2C)>>16; y=(s32)PLAYER32(0x30)>>16;
        n=(PLAYER8(0x14)>>1)&3; x+=delta[n][0]; y+=delta[n][1];
        if(!Movement_IsSafeGround(x,y)) { PracticeRuntime_SetStatus("SPAWN: NEED CLEAR FLOOR"); return; }
        for(n=8;n<80;n++) if(live(slot(n))) {
            s32 dx=(s16)E16(slot(n),0x2E)-x, dy=(s16)E16(slot(n),0x32)-y;
            if(dx>-16 && dx<16 && dy>-16 && dy<16) { PracticeRuntime_SetStatus("SPAWN: POSITION OCCUPIED"); return; }
        }
        if(s->kind==3) e=((void*(*)(u32,u32))0x0804AA61u)(s->id,s->type);
        else {
            e=((void*(*)(void))0x0805E679u)();
            if(e) { e[8]=6; e[9]=0; e[10]=s->type;
                ((void(*)(void*,u32))0x0805EA2Du)((void*)e,6); }
        }
        if(!e) { PracticeRuntime_SetStatus("SPAWN FAILED"); return; }
        E32(e,0x2C)=(u32)x<<16; E32(e,0x30)=(u32)y<<16;
        e[0x38]=PLAYER8(0x38);
        PM.actorMarks[mark_index(e)]=2;
        PM.actorSelected=(uptr)e; PM.actorIdentity=E32(e,8);
        PracticeRuntime_SetStatus("SPAWNED 32PX AHEAD");
    }
}

void Actors_Action(u32 row,u16 pressed,s32 direction) {
    volatile u8* e; s32 i;
    if(PM.page==PAGE_ACTORS) {
        if(pressed&KEY_A) {
            if(row==0) PM.page=PAGE_ACTOR_LIST;
            if(row==1 || row==3) {
                PM.actorRaw=row==1; PM.page=PAGE_ACTOR_SPAWN; PM.cursor[PAGE_ACTOR_SPAWN]=0;
                if(!PM.actorRawKind) PM.actorRawKind=3;
            }
            if(row==2) { PM.actorContext=0; for(i=0;i<112;i++) PM.actorMarks[i]&=~1; PracticeRuntime_SetStatus("INDIVIDUAL FREEZES CLEARED"); }
        }
    } else if(PM.page==PAGE_ACTOR_LIST) {
        if(direction) {
            PM.actorFilter=(PM.actorFilter+(direction>0?1:6))%7;
            PM.cursor[PAGE_ACTOR_LIST]=0;
        } else if(pressed&KEY_A) {
            e=nth(row); if(e) { PM.actorSelected=(uptr)e; PM.actorIdentity=E32(e,8);
                PM.page=PAGE_ACTOR_DETAIL; PM.cursor[PAGE_ACTOR_DETAIL]=0; }
        }
    } else if(PM.page==PAGE_ACTOR_DETAIL) {
        e=selected();
        if(!e) return;
        if(row==0 && (pressed&KEY_A)) {
            i=mark_index(e);
            if(i>=0 && (PM.actorMarks[i]&1)) { PM.actorMarks[i]&=~1; if(PM.actorContext) PM.actorContext--; }
            else if(i>=0) { capture_pose(e); PM.actorMarks[i]|=1; PM.actorContext++; }
        } else if(row==1 && (pressed&KEY_A) && can_delete(e)) {
            PM.actorConfirmSpawn=0; PM.page=PAGE_ACTOR_DELETE; PM.cursor[PAGE_ACTOR_DELETE]=0;
        } else if(row>=2 && row<=5 && direction) {
            if(e[8]==9) { PracticeRuntime_SetStatus("MANAGER HAS NO GENERIC XYZ"); return; }
            if(row<=4) E32(e,0x2C+(row-2)*4)+=(u32)(direction*(PM.actorMoveStep?16:1))*0x10000u;
            else e[0x38]=(e[0x38]+(direction>0?1:3))%4;
            capture_pose(e);
        } else if(row==6 && (direction || (pressed&KEY_A))) PM.actorMoveStep^=1;
        else if((row==7 || row==8) && (pressed&KEY_A)) {
            if(e[8]==9) { PracticeRuntime_SetStatus("MANAGER HAS NO GENERIC XYZ"); return; }
            PracticeRuntime_Queue(row==7?PENDING_LINK_TO_ACTOR:PENDING_ACTOR_TO_LINK);
        }
    } else if(PM.page==PAGE_ACTOR_SPAWN) {
        if(PM.actorRaw) {
            u8* fields[]={&PM.actorRawKind,&PM.actorRawId,&PM.actorRawType,&PM.actorRawType2,
                &PM.actorRawTimer,&PM.actorRawSubtimer,&PM.actorRawFlags,&PM.actorRawParent,&PM.actorRawLayer};
            s32 step=direction;
            if(pressed&KEY_L) step=-16; else if(pressed&KEY_R) step=16;
            if(row<9 && step) {
                if(row==0) { static const u8 rk[]={1,3,4,6,7,8,9};
                    for(i=0;i<7 && rk[i]!=PM.actorRawKind;i++);
                    PM.actorRawKind=rk[(i+(step>0?1:6))%7]; PM.actorRawId=0;
                } else if(row==7) PM.actorRawParent=(PM.actorRawParent+(step>0?1:2))%3;
                else if(row==8) PM.actorRawLayer=(PM.actorRawLayer+(step>0?1:3))%4;
                else *fields[row]+=step;
            }
            if(row==9 && (pressed&KEY_A)) { PM.actorConfirmSpawn=1;
                PM.page=PAGE_ACTOR_DELETE; PM.cursor[PAGE_ACTOR_DELETE]=0; }
            return;
        }
        if(row==0) {
            s32 step=direction;
            if(pressed&KEY_L) step=-5;
            else if(pressed&KEY_R) step=5;
            else if(!step && (pressed&KEY_A)) step=1;
            if(step) PM.actorSpawn=(PM.actorSpawn+ARRAY_COUNT(spawns)+step)%ARRAY_COUNT(spawns);
        }
        if(row==1 && (pressed&KEY_A)) PracticeRuntime_Queue(PENDING_ACTOR_SPAWN);
    } else if(PM.page==PAGE_ACTOR_DELETE && (pressed&KEY_A)) {
        if(row==0) PM.page=PM.actorConfirmSpawn?PAGE_ACTOR_SPAWN:PAGE_ACTOR_DETAIL;
        else if((TMC_INPUT.held&PM.confirmHotkey)==PM.confirmHotkey)
            PracticeRuntime_Queue(PM.actorConfirmSpawn?PENDING_ACTOR_SPAWN:PENDING_ACTOR_DELETE);
        else PracticeRuntime_SetStatus("HOLD CONFIRM HOTKEY + NEW A");
    }
}

static void text(u32 y,const char* s) { PracticeRender_Text(3,y,s,0); }
static void clipped(u32 x,u32 y,const char* s,u32 width) {
    char line[27];u32 i=0;
    if(width>26) width=26;
    while(i<width && s[i]) { line[i]=s[i];i++; }
    line[i]=0;PracticeRender_Text(x,y,line,0);
}
static void full_name(u32 y,const char* s) {
    u32 len=0,split=26,i;
    while(s[len]) len++;
    if(len<=26) { text(y,s);return; }
    for(i=26;i>10;i--) if(s[i]==' ' && len-i-1<=26) { split=i;break; }
    clipped(3,y,s,split);s+=split;if(*s==' ') s++;
    clipped(3,y+1,s,26);
}
static void button(u32 row,u32 y,const char* s) {
    if(PM.cursor[PM.page]==row) PracticeRender_Text(1,y,">",1);
    text(y,s);
}
void Actors_Draw(void) {
    volatile u8* e; u32 row,first,count; char keys[24];
    PracticeRender_Frame("ACTORS / OBJECTS",0);
    switch(PM.page) {
        case PAGE_ACTORS:
            button(0,3,"ROOM ACTOR LIST");button(1,4,"RAW NATIVE SPAWNER");button(2,5,"UNFREEZE INDIVIDUALS");
            button(3,6,"QUICK SPAWN PRESETS");
            text(9,"FREEZE / REMOVE ALL ACTORS");text(11,"MOVE ANY SPATIAL ACTOR");
            text(13,"EXPERT EDITS CAN SOFTLOCK");text(15,"NO AUTOMATIC SAVE");break;
        case PAGE_ACTOR_LIST:
            count=Actors_Count();first=(PM.cursor[PM.page]/10)*10;
            for(row=0;row<10 && first+row<count;row++) {
                e=nth(first+row);
                if(PM.cursor[PM.page]==first+row) PracticeRender_Text(1,3+row,">",1);
                PracticeRender_U32(3,3+row,first+row+1,3,0);
                clipped(7,3+row,name(e),18);
                PracticeRender_Hex(26,3+row,e[9],2,0);
            }
            if(!count) text(5,"NO ACTORS IN THIS FILTER");
            text(14,"FILTER:");PracticeRender_Text(11,14,filters[PM.actorFilter<7?PM.actorFilter:0],2);
            PracticeRender_U32(25,14,count,3,0);text(15,"LEFT/RIGHT:FILTER A:DETAILS");break;
        case PAGE_ACTOR_DETAIL:
            e=selected();if(!e) { text(5,"ACTOR GONE / ROOM CHANGED");text(7,"B: RETURN TO ROOM LIST");break; }
            button(0,3,Actors_Frozen((void*)e)?"UNFREEZE SELECTED":"FREEZE SELECTED");button(1,4,"REMOVE SELECTED...");
            button(2,5,"X");button(3,6,"Y");button(4,7,"Z");button(5,8,"LAYER");
            button(6,9,PM.actorMoveStep?"MOVE STEP: 16PX":"MOVE STEP: 1PX");
            button(7,10,"LINK TO ACTOR");button(8,11,"ACTOR TO LINK");
            if(e[8]!=9) {
                PracticeRender_S32(16,5,(s16)E16(e,0x2E)-(s32)PM.menuOriginX,5,0);
                PracticeRender_S32(16,6,(s16)E16(e,0x32)-(s32)PM.menuOriginY,5,0);
                PracticeRender_S32(16,7,(s16)E16(e,0x36),5,0);PracticeRender_U32(16,8,e[0x38],1,0);
            }
            clipped(3,12,name(e),26);PracticeRender_Hex(20,13,(uptr)e,8,0);
            text(14,"ID/TYPE/SUB:");PracticeRender_Hex(16,14,e[9],2,0);
            PracticeRender_Hex(20,14,e[10],2,0);PracticeRender_Hex(24,14,e[11],2,0);
            text(15,e[8]==9?"MANAGER: NO GENERIC XYZ":"SAME ROOM / XYZ + LAYER");break;
        case PAGE_ACTOR_SPAWN:
            if(PM.actorRaw) {
                static const char* const labels[]={"KIND","ID","TYPE","TYPE 2","TIMER","SUBTIMER","FLAGS","PARENT","LAYER","SPAWN RAW..."};
                u8 values[]={PM.actorRawKind,PM.actorRawId,PM.actorRawType,PM.actorRawType2,
                    PM.actorRawTimer,PM.actorRawSubtimer,PM.actorRawFlags,PM.actorRawParent,PM.actorRawLayer};
                static const char* const parents[]={"NONE","LINK","SELECTED"};
                for(row=0;row<10;row++) { button(row,3+row,labels[row]);
                    if(row<9) PracticeRender_Hex(24,3+row,values[row],2,0); }
                { static const char* const categories[10]={"","PLAYER","","ENEMY","PROJECTILE","","OBJECT","NPC","PLAYER ITEM","MANAGER"};
                  if(PM.actorRawKind<10) PracticeRender_Text(10,3,categories[PM.actorRawKind],2); }
                PracticeRender_Text(13,10,parents[PM.actorRawParent%3],0);
                full_name(13,(PM.actorRawKind==6 && PM.actorRawId==0)?
                    Actors_GroundItemName(PM.actorRawType):Actors_NativeName(PM.actorRawKind,PM.actorRawId));
                text(15,"L/R: +/-10 HEX; +32PX EAST");break;
            }
            row=PM.actorSpawn<ARRAY_COUNT(spawns)?PM.actorSpawn:0;
            button(0,3,"CHOOSE WITH LEFT/RIGHT");text(5,spawns[row].name);
            PracticeRender_U32(21,4,row+1,2,0);PracticeRender_Text(23,4,"/",0);
            PracticeRender_U32(24,4,ARRAY_COUNT(spawns),2,0);
            text(6,spawns[row].kind==3?"ENEMY - NATIVE BEHAVIOR":"RESOURCE PICKUP");
            button(1,8,"SPAWN AT LINK + 32PX");text(10,"NORMAL SIZE / CLEAR FLOOR");
            text(12,"KEEPS 7+ ENGINE SLOTS FREE");text(14,"NO BOSSES OR RAW IDS");
            text(15,"SHOULDER L/R: SKIP 5");break;
        case PAGE_ACTOR_DELETE:
            button(0,3,"CANCEL");button(1,4,PM.actorConfirmSpawn?"CONFIRM RAW SPAWN":"CONFIRM REMOVE ACTOR");
            e=selected();full_name(6,PM.actorConfirmSpawn?
                ((PM.actorRawKind==6 && PM.actorRawId==0)?Actors_GroundItemName(PM.actorRawType):Actors_NativeName(PM.actorRawKind,PM.actorRawId)):
                e?name(e):"ACTOR NO LONGER AVAILABLE");
            Settings_KeyText(PM.confirmHotkey,keys);text(9,keys);
            text(11,"CAN CRASH / BREAK THE ROOM");text(12,"USE A BACKUP BEFORE TESTING");
            text(14,"HOLD THE ABOVE KEYS + NEW A");break;
        default:break;
    }
}
