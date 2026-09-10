#include "practice.h"

static u32 infinite_time(void) {
    return gPracticeState.magic==PRACTICE_MAGIC &&
        (gPracticeState.resourceCheats & CHEAT_MINIGAME_TIME);
}

/* Actual running Cucco countdown, not object update/animation timers. Refund
 * one decrement BEFORE expiry detection; OFF and expired timers stay native. */
__attribute__((naked,noinline)) static void native_countdown(void* e __attribute__((unused))) {
    __asm__ volatile("push {r4,r5,r6,r7,lr}\n mov r7,r10\n mov r6,r9\n mov r5,r8\n ldr r3,=0x080A1279\n bx r3\n");
}
void MinigameTimerDispatch(void* entity) {
    volatile s16* timer=(volatile s16*)((u8*)entity+0x68);
    if(infinite_time() && *timer>0 && *timer<32767) ++*timer;
    native_countdown(entity);
}

/* Exact native callback from UpdateTimerCallbacks table080FCB18. Only the
 * three-minute failure deadline is paused, not Biggoron's completion wait. */
void MinigameDarknutDispatch(void* timer) {
    if(infinite_time() && *(volatile u32*)timer) return;
    CALL_VOID1(0x08053435u)(timer);
}

void Minigame_ActorDispatch(void* entity,void (*native)(void*)) {
    volatile u8* e=entity;u16 eye=0;
    if(infinite_time()) {
        if(e[8]==1) {
            /* Extend only positive beneficial effects at their last tick.
             * The normal countdown/particle cadence keeps running; freezing a
             * modulo16/64 FX timer at a fixed value could flood the actor pool.
             * Do NOT extend effectTimer: Wisp's item-lock curse must expire. */
            if(TMC_SAVE.stats.charm && TMC_SAVE.stats.charmTimer==1) TMC_SAVE.stats.charmTimer=2;
            if(TMC_SAVE.stats.picolyteType && TMC_SAVE.stats.picolyteTimer==1) TMC_SAVE.stats.picolyteTimer=2;
        } else if(e[8]==6 && e[9]==0x23 && e[12]==3 && e[14]) {
            volatile u16* timer=(volatile u16*)(e+0x70);
            if(*timer && *timer<65535) { eye=*timer;*timer=eye+1; }
        }
    }
    native(entity);
    /* Native EyeSwitch_Action3 still observes its permanent completion flag.
     * If EntityDisabled skipped the update, undo the unused refund as well. */
    if(eye && e[8]==6 && e[9]==0x23 && e[12]==3 && *(volatile u16*)(e+0x70)>eye)
        *(volatile u16*)(e+0x70)=eye;
}
