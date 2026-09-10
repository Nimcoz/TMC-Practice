#include "practice.h"
/* Read-only ABI descriptor for external tests; never enables a test driver. */
#define O(field) __builtin_offsetof(PracticeState, field)
const u32 gPracticeOffsets[] = {
    O(page), O(cursor), O(pendingAction), O(selectedArea), O(selectedRoom),
    O(noClip), O(freezeEnemies), O(debugHud), O(speedMode), O(lockDirection),
    O(cameraMode), O(cameraActive), O(nudgeStep), O(nudgeDirection),
    O(inventoryDirty), O(flagIndex), O(flagBank), O(knownFlag), O(status),
    O(menuArea), O(menuRoom), O(menuLocalFlagOffset), O(flagOperation),
    O(flagBefore), O(flagAfter), O(resourceCheats), sizeof(PracticeState),
    O(menuTheme), O(flagGroup), O(collectionAction), O(flagLogCount), O(menuDungeonIndex), O(sceneSkipTicks),
    O(menuHotkey), O(confirmHotkey), O(bindingDraft), O(bindingTarget), O(favorites),
    O(completionUndoValid), O(completionUndoSlot), O(completionReturn), O(completionBackup),
    O(actorSelected), O(actorIdentity), O(actorMarks), O(actorFilter), O(actorSpawn),
    O(actorRaw), O(actorRawKind), O(actorRawId), O(actorRawType), O(actorRawType2),
    O(actorRawTimer), O(actorRawSubtimer), O(actorRawFlags), O(actorRawParent),
    O(actorRawLayer), O(actorConfirmSpawn), O(actorMoveStep), O(actorExpert), O(breakFreeTicks),
    O(modalMode), O(modalSkipTail), O(modalContext), O(modalSaveEdits),
    O(sceneReplay), O(sceneReturn), O(sceneSeen), O(sceneConfirm)
};
