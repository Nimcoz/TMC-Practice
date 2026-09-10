#include "practice.h"

#define ROOM16(off) (*(volatile u16*)(ADDR_G_ROOM_CONTROLS + (off)))
#define CAMERA (*(volatile uptr*)(ADDR_G_ROOM_CONTROLS + 0x30))

u32 Movement_IsControlled(void) {
    return (PLAYER8(0x0C) == 1 || PLAYER8(0x0C) == 9) &&
        PSTATE8(0x8B) == 0 && PSTATE8(0x0C) == 0 && PSTATE8(2) == 0 &&
        PSTATE8(0x1C) == 0 && PSTATE8(0x26) == 0 && PSTATE8(0x1E) == 0 &&
        PLAYER8(0x42) == 0 && PLAYER32(0x34) == 0 &&
        (PSTATE32(0x30) & ~0x00C00088u) == 0 &&
        PSTATE8(0xA8) <= 1 && !TMC_TRANSITION.transitioningOut &&
        !*(volatile u8*)ADDR_G_FADE_CONTROL &&
        !(*(volatile u8*)ADDR_G_MESSAGE & 0x7F);
}

/* Conservative native collision + surface test. A clear collision cell alone
 * is not sufficient: pits, water, stairs and triggers have separate act tiles.
 * Check Link's full footprint; refuse all non-ordinary / non-clone surfaces.
 */
u32 Movement_IsSafeGround(s32 x, s32 y) {
    static const s8 offsets[5][2] = {{-5,-9},{5,-9},{-5,0},{5,0},{0,-4}};
    u32 i, collision, act, surface;
    if (PLAYER8(0x38) < 1 || PLAYER8(0x38) > 2) return 0;
    for (i = 0; i < 5; i++) {
        s32 px = x + offsets[i][0], py = y + offsets[i][1];
        if (px < TMC_ROOM.originX || py < TMC_ROOM.originY ||
            px >= TMC_ROOM.originX + ROOM16(0x1E) ||
            py >= TMC_ROOM.originY + ROOM16(0x20)) return 0;
        collision = ((u32(*)(s32,s32,u32))0x080002D5u)(px,py,PLAYER8(0x38));
        if (collision != 0) return 0;
        act = ((u32(*)(s32,s32,u32))0x080002BDu)(px,py,PLAYER8(0x38));
        surface = ((u32(*)(u32,const void*))0x08007DD7u)(act,(void*)0x08007CACu);
        if (surface != 0 && surface != 0x15) return 0;
    }
    return 1;
}

static s32 clamp_camera(s32 p, s32 origin, s32 extent, s32 half);

void Movement_ReleaseCamera(void) {
    if (gPracticeState.cameraActive && CAMERA == (uptr)gPracticeState.cameraEntity) {
        CAMERA = gPracticeState.cameraOwner;
        if (gPracticeState.cameraMode==2 && TMC_ROOM.scrollAction==1 &&
            TMC_ROOM.area==gPracticeState.cameraArea && TMC_ROOM.room==gPracticeState.cameraRoom) {
            /* Native Scroll1 assumes its input already lies inside room bounds.
             * Return to a legal viewport, including exactly screen-sized houses. */
            ROOM16(0x0A)=clamp_camera(PLAYER16(0x2E),TMC_ROOM.originX,ROOM16(0x1E),120)-120;
            ROOM16(0x0C)=clamp_camera(PLAYER16(0x32),TMC_ROOM.originY,ROOM16(0x20),80)-80;
            *(volatile u8*)0x02000070u=1; /* native gUpdateVisibleTiles */
        }
    }
    gPracticeState.cameraActive = 0;
}

static s32 clamp_camera(s32 p, s32 origin, s32 extent, s32 half) {
    s32 lo = origin + half, hi = origin + extent - half;
    if (hi < lo) hi = lo;
    if (p < lo) p = lo;
    if (p > hi) p = hi;
    return p;
}

void Movement_BeforeGame(void) {
    u16 keys = TMC_INPUT.held;
    u32 safe = Movement_IsControlled();
    if (gPracticeState.lockDirection && safe &&
        !(keys & (KEY_A | KEY_B | KEY_R | KEY_L))) {
        if (!gPracticeState.facingApplied) {
            gPracticeState.facing = PLAYER8(0x14);
            gPracticeState.facingApplied = 1;
        }
        /* Reserved bit, removed after this native update; no item bits owned. */
        PSTATE8(0x0B) |= 0x40;
        PLAYER8(0x14) = gPracticeState.facing;
    } else gPracticeState.facingApplied = 0;

    if (gPracticeState.cameraActive &&
        (TMC_ROOM.area != gPracticeState.cameraArea || TMC_ROOM.room != gPracticeState.cameraRoom ||
         !safe || TMC_ROOM.scrollAction != 1 ||
         (CAMERA != (uptr)gPracticeState.cameraEntity && CAMERA != gPracticeState.cameraOwner))) {
        Movement_ReleaseCamera();
        gPracticeState.cameraMode = 0;
    }
    if (!gPracticeState.cameraMode) { Movement_ReleaseCamera(); return; }
    if (!gPracticeState.cameraActive) {
        if (!safe || TMC_ROOM.scrollAction != 1 || CAMERA != ADDR_G_PLAYER_ENTITY) {
            gPracticeState.cameraMode = 0;
            PracticeRuntime_SetStatus("CAMERA: CONTROLLED GROUND ONLY");
            return;
        }
        Practice_ClearBytes(gPracticeState.cameraEntity,sizeof(gPracticeState.cameraEntity));
        gPracticeState.cameraEntity[0x2C/4] = (ROOM16(0x0A) + 120) << 16;
        gPracticeState.cameraEntity[0x30/4] = (ROOM16(0x0C) + 80) << 16;
        gPracticeState.cameraOwner = CAMERA;
        gPracticeState.cameraArea = TMC_ROOM.area;
        gPracticeState.cameraRoom = TMC_ROOM.room;
        gPracticeState.cameraActive = 1;
    }
    if (gPracticeState.cameraMode == 2) {
        s32 x = (s32)gPracticeState.cameraEntity[11] >> 16;
        s32 y = (s32)gPracticeState.cameraEntity[12] >> 16;
        s32 step = keys & KEY_R ? 8 : 2;
        if (keys & KEY_LEFT) x -= step;
        if (keys & KEY_RIGHT) x += step;
        if (keys & KEY_UP) y -= step;
        if (keys & KEY_DOWN) y += step;
        /* Loaded native map is 64x64 metatiles, not an adjacency/warp request.
         * Keep the entire viewport within that allocation, including small rooms. */
        gPracticeState.cameraEntity[11] = (u32)clamp_camera(x,TMC_ROOM.originX,1024,120) << 16;
        gPracticeState.cameraEntity[12] = (u32)clamp_camera(y,TMC_ROOM.originY,1024,80) << 16;
        CAMERA = (uptr)gPracticeState.cameraEntity;
        /* Suppress native input and door transitions while viewing the map.
         * B / Start exit; hotkey survives to open the menu safely. */
        if (TMC_INPUT.pressed & (KEY_B | KEY_START)) {
            Movement_ReleaseCamera(); gPracticeState.cameraMode = 0;
        }
        if ((keys & PRACTICE_HOTKEY) != PRACTICE_HOTKEY) {
            TMC_INPUT.held = TMC_INPUT.pressed = TMC_INPUT.repeat = 0;
        }
    }
}

void Movement_AfterGame(void) {
    if (gPracticeState.facingApplied) PSTATE8(0x0B) &= ~0x40;
    if (gPracticeState.cameraActive &&
        (!Movement_IsControlled() || TMC_TRANSITION.transitioningOut || TMC_ROOM.scrollAction != 1)) {
        Movement_ReleaseCamera(); gPracticeState.cameraMode = 0;
    }
}

void ScrollFollowDispatch(void* controls) {
    uptr target = CAMERA;
    u16 width = ROOM16(0x1E), height = ROOM16(0x20);
    if (gPracticeState.cameraActive && gPracticeState.cameraMode == 1 &&
        target == gPracticeState.cameraOwner) CAMERA = (uptr)gPracticeState.cameraEntity;
    if (gPracticeState.cameraActive && gPracticeState.cameraMode == 2 && target == (uptr)gPracticeState.cameraEntity) {
        ROOM16(0x1E) = 1024; ROOM16(0x20) = 1024;
    }
    CALL_VOID1(0x0807FC7Du)(controls);
    ROOM16(0x1E) = width; ROOM16(0x20) = height;
    if (gPracticeState.cameraMode == 1 && CAMERA == (uptr)gPracticeState.cameraEntity)
        CAMERA = target;
}

void Movement_ApplyNudge(void) {
    static const s8 delta[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    s32 x = PLAYER16(0x2E), y = PLAYER16(0x32);
    u32 i, count = gPracticeState.nudgeStep ? 8 : 1;
    u32 dir = gPracticeState.nudgeDirection & 3;
    if (!Movement_IsControlled()) { PracticeRuntime_SetStatus("NUDGE: CONTROLLED GROUND ONLY"); return; }
    for (i = 1; i <= count; i++) {
        if (!gPracticeState.noClip && !Movement_IsSafeGround(x + delta[dir][0]*(s32)i,y + delta[dir][1]*(s32)i)) {
            PracticeRuntime_SetStatus("NUDGE BLOCKED: SOLID / UNSAFE"); return;
        }
    }
    PLAYER32(0x2C) += (s32)delta[dir][0] * (s32)count * 65536;
    PLAYER32(0x30) += (s32)delta[dir][1] * (s32)count * 65536;
    PracticeRuntime_SetStatus("NUDGE APPLIED");
}

void Movement_GroundReset(void) {
    u32 action = PLAYER8(0x0C);
    if ((action != 1 && action != 4 && action != 6 && action != 9 && action != 24) ||
        PSTATE8(0x8B) || PSTATE8(0x0C) || TMC_TRANSITION.transitioningOut ||
        (*(volatile u8*)ADDR_G_MESSAGE & 0x7F) ||
        (PSTATE32(0x30) & ~(0x00040288u)) ||
        !Movement_IsSafeGround(PLAYER16(0x2E),PLAYER16(0x32))) {
        PracticeRuntime_SetStatus("GROUND RESET: UNSAFE / SCRIPT"); return;
    }
    CALL_VOID0(TMC_PUT_AWAY_ITEMS)();
    PLAYER32(0x20) = 0; PLAYER32(0x34) = 0;
    PSTATE8(2) = 0; PSTATE8(0x26) = 0;
    PSTATE32(0x30) &= ~0x00040200u;
    CALL_VOID0(0x080791D1u)();
    PracticeRuntime_SetStatus("GROUNDED ON VERIFIED FLOOR");
}

void Cheats_BreakFree(void) {
    if(TMC_TRANSITION.transitioningOut || PLAYER8(8)!=1 || !TMC_SAVE.stats.health) {
        PracticeRuntime_SetStatus("BREAK FREE: NO LIVE ROOM"); return;
    }
    /* USA MessageClose only writes gMessage.state=88; MsgUpdate overwrites that
     * byte before consuming it. Enter the actual native MsgClose state instead.
     * Clear token-active so its subsequent TextDispInit advances to MsgDie.
     * Window animation, graphics cleanup and closing sound remain native. */
    gPracticeState.breakFreeTicks=120;
    if(*(volatile u8*)ADDR_G_MESSAGE & 0x7F) {
        volatile u8* text=(volatile u8*)ADDR_G_TEXT_RENDER;
        text[0x20]&=~1; text[0x88]=4; text[0x89]=0; text[0x8A]=0;
        *(volatile u8*)ADDR_G_MESSAGE=8;
    }
    Cheats_BreakFreeUpdate();
}
void Cheats_BreakFreeUpdate(void) {
    if(!gPracticeState.breakFreeTicks) return;
    if(TMC_TRANSITION.transitioningOut || PLAYER8(8)!=1) { gPracticeState.breakFreeTicks=0; return; }
    gPracticeState.breakFreeTicks--;
    if(*(volatile u8*)ADDR_G_MESSAGE & 0x7F) return;
    gPracticeState.breakFreeTicks=0;
    PSTATE8(0x8B)=0;
    /* Release native priority locks, without declaring the story complete. */
    Practice_ClearBytes((void*)ADDR_G_PRIORITY_HANDLER,10);
    CALL_VOID0(0x0805E565u)();
    *(volatile u8*)0x02034490u=0;
    CALL_VOID0(TMC_DELETE_CLONES)();
    CALL_VOID0(TMC_PUT_AWAY_ITEMS)();
    PSTATE8(0x0C) = 0; PSTATE8(0x3C)=0;
    PLAYER8(0x42) = 0; PLAYER8(0x43) = 0;
    CALL_VOID0(0x080791D1u)();
    PracticeRuntime_SetStatus("TEXT CLOSED / CONTROL FREED");
}

void Cheats_OcarinaGlitch(void) {
    u32 active = PSTATE32(0x30) & 0x10000000u;
    u32 flags = PSTATE32(0x30) & ~0x10000000u;
    /* Native state measured after a real stairs + ocarina interruption:
     * idle, collision enabled, PL_USE_OCARINA remains, player/event priority 6,
     * pause disabled, no active item. Stage 2 follows a native room scroll.
     * Never maintain per-frame flags or invent additional glitch levels. */
    if ((PLAYER8(0x0C) != 1 && PLAYER8(0x0C) != 9) ||
        PSTATE8(0x8B) || PSTATE8(0x0C) || PSTATE8(2) || PSTATE8(0x1C) ||
        PSTATE8(0x26) || PSTATE8(0x1E) || PLAYER8(0x42) || PLAYER32(0x34) ||
        (flags & ~0x00C00088u) || (*(volatile u8*)ADDR_G_MESSAGE & 0x7F) ||
        TMC_TRANSITION.transitioningOut || !Movement_IsSafeGround(PLAYER16(0x2E),PLAYER16(0x32)) ||
        (!active && (!Inventory_Get(23) || *(volatile u8*)ADDR_G_PRIORITY_HANDLER))) {
        PracticeRuntime_SetStatus("OG: NEED OCARINA / SAFE IDLE"); return;
    }
    Movement_ReleaseCamera(); gPracticeState.cameraMode = 0;
    CALL_VOID0(TMC_DELETE_CLONES)();
    CALL_VOID0(TMC_PUT_AWAY_ITEMS)();
    CALL_VOID0(0x080791D1u)();
    PLAYER8(0x10) |= 0x80;
    PSTATE8(0x27) = 0;
    if (active) {
        PSTATE32(0x30) &= ~0x10000000u;
        *(volatile u8*)0x02034490u = 0;
        CALL_VOID0(0x0805E565u)();
        PracticeRuntime_SetStatus("OCARINA GLITCH ENDED");
    } else {
        PSTATE32(0x30) |= 0x10000000u;
        *(volatile u8*)0x02034490u = 1;
        CALL_VOID0(0x0805E545u)();
        PracticeRuntime_SetStatus("OG STAGE 1 - ROLL CAN LOCK");
    }
}
