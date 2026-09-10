# Controls, feature scope and safety

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

For installation and the complete user guide in your language, choose a link above.
The Practice menu itself remains in English.

## Controls

- Default open/close hotkey: **L + R + Select**.
- D-pad: select rows and change editable values. A activates; B returns/cancels.
- Destructive operations: use the confirmation shown on screen. Default is
  holding **L + R** and pressing **A** again on the confirmation row.
- Raw numeric fields are hexadecimal. Left/right change by one; L/R change
  applicable byte fields by `0x10`. Category selectors use their listed choices.
- Saved custom bindings override the defaults. Menu settings persist separately
  from native progress; never assume unsaved gameplay is automatically saved.

## Story replays

WORLD / WARP contains STORY INTRO, ENDING / CREDITS and RETURN FROM REPLAY.
Select the requested scene with A, then confirm with a second A. Start from
active gameplay. Replays use native scripts and transitions, not empty-room warps.
They back up all 1,204 native save bytes in RAM and restore them when returning.
Returning reloads the origin room; transient room actors are not an emulator savestate.
Other mutations and cheats/timing are temporarily suspended. Ending replays
return before the native save prompt; normal endings retain their original save path.

## Menu access

Supported native inventories, dialogue/cutscenes, title/intro and ending screens
can be inspected. Destructive world actions need active gameplay and may be
disabled in a paused native view. The menu waits for fades/resource transfers
and will not suspend an actual EEPROM write. A rejected hotkey during a
transition is not a promise that every initialization frame can be interrupted.

## Actor tools

ACTORS / OBJECTS has a named RAW NATIVE SPAWNER, QUICK SPAWN PRESETS and ROOM ACTOR LIST.
The raw mode exposes native kind/ID/type/type2/timer/subtimer/flags/parent/layer.
There is no curated room/floor/variant allowlist; missing native handlers and
full native pools are still rejected. 546 native slots and 118 ground-item
types are labeled; 25 unidentified entries remain UNKNOWN.

Individual actors can be frozen, removed and spatially edited. LINK TO ACTOR
and ACTOR TO LINK teleport within the current room without an artificial warp.
Managers have no shared XYZ layout, so generic spatial editing is unavailable
for them, although freezing/removal remains available. A running native AI may
move an edited actor again; freeze it first for fixed placement.

Spawning a second Link, invalid bosses or actors lacking their room/script/parent
dependencies can crash or block the game. Removing a boss does not grant victory
or solve the room. Keep a backup and reload without saving if necessary.

## Enemy Freeze, Break Free and Infinite Time

Enemy Freeze includes boss enemies and visible non-colliding body parts.
Projectiles of other categories may need individual actor freeze. Break Free
closes active text through the native close state and releases player control;
it does not undo already-executed story scripts or guarantee a later script
will not take control again.

Infinite Time covers Anju's chicken countdown, the Dark Hyrule Castle countdown,
timed eye-switch activation and already-active charm/luck-potion durations.
It is not a global freeze of every timer, animation or cutscene. Turn it off to
allow native countdown evaluation/rewards and timed effects to finish.

## Save safety

Collections, flags and the confirmed 100% action can materially change progress.
Use a same-region save and an independent backup. No cross-region save converter
is released. Do not load old-build emulator savestates or stack external cheats.
The native EU language selection and JP game text are preserved; Practice is English.
