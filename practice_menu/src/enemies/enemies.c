#include "practice.h"

/* Global freeze includes all enemy IDs, bosses and multipart supports. Native
 * initialization, drawing, damage and death dispatch stay active. Individual
 * actor freeze instead intercepts the outer updater, independently of kind. */

void EnemyBehaviorDispatch(void* entity, void (*behavior)(void*)) {
    volatile u8* e = (volatile u8*)entity;
    u32 wizzrobe=e[9]>=0x27 && e[9]<=0x29;
    /* Let native spawning / burrowing / teleport fade-in complete. Freezing a
     * non-collidable invisible Wizzrobe would strand its fight forever. */
    if (gPracticeState.freezeEnemies && e[0x0C] != 0 &&
        (((e[0x18] & 3) && (!wizzrobe || (e[0x10]&0x80))) || (e[0x6D]&0x11)) &&
        CALL_U32_1(0x0800279Du)(entity) == 0) return;
    behavior(entity);
}

void Enemies_BeforeGame(void) { Actors_RestorePoses(); }
void Enemies_AfterGame(void) { Actors_RestorePoses(); }
