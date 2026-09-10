# TMC Practice — English guide

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

TMC Practice adds a practice and exploration menu to **The Legend of Zelda: The Minish Cap**.
Version **v1.0.0** has separate BPS patches for USA, Europe and Japan.
This guide is translated; **the Practice menu itself remains in English**.
The original EU language selection and Japanese game text are preserved.
English menu labels are kept below so you can find them in the game.

## 1. Download and installation

1. Open [TMC Practice v1.0.0](https://github.com/Nimcoz/TMC-Practice/releases/tag/v1.0.0).
2. Download exactly one patch for your **original ROM's region**, not this guide's language:
   - USA: `TMC-Practice-v1.0.0-USA.bps`.
   - Europe (English, French, German, Spanish, Italian): `TMC-Practice-v1.0.0-Europe.bps`.
   - Japan: `TMC-Practice-v1.0.0-Japan.bps`.
3. Make a separate backup of your normal in-game save.
4. Apply the BPS patch to your **unmodified original ROM** with a BPS-compatible patcher.
5. Open the resulting `.gba` in your emulator and use a normal save from the same region.
6. Press **L + R + Select** to open or close the menu. A previously saved custom shortcut takes precedence.

The [ROM compatibility list](ROM_COMPATIBILITY.md) gives the exact supported fingerprints.
Do not ignore a source-checksum error. Do not stack patches, external AR codes or
other Practice/No-Clip modifications, or load emulator savestates from older builds.
No ROMs, saves or savestates are included. There is no released cross-region save converter.

## 2. Controls and settings

- **D-pad:** select a row; left/right change an editable value.
- **A:** activate/confirm. **B:** back/cancel.
- **Destructive actions:** follow the on-screen confirmation; normally hold **L + R** and press **A** again on the confirmation row.
- **Raw numbers are hexadecimal.** Left/right change by one; L/R change applicable byte fields by `0x10`.
- **SETTINGS:** themes, button bindings and persistent menu settings. Saving menu settings does **not** save unsaved game progress.

## 3. Finding features

| Menu | Purpose |
| --- | --- |
| PRACTICE | Practice timer and controls; on-screen timer display |
| PLAYER / MOVEMENT | Link, movement, No-Clip and camera |
| INVENTORY | Items, equipment, bottles and individual elements |
| WORLD / WARP | Rooms, favorites, story intro and ending replays |
| FLAGS | Inspect and edit game-state flags |
| DEBUG | On-screen debug information |
| CHEATS | Resources, Enemy Freeze, Infinite Time and other cheats |
| SETTINGS | Appearance, controls and saved menu preferences |
| ACTORS / OBJECTS | Actor list, raw spawner, freeze, removal and teleport tools |

Collection edits, flags and the confirmed **100%** action can change story progress.
Use a backup before experimenting. Unused/Beta placeholders are not reconstructed Beta content.

## 4. Opening the menu outside normal gameplay

The menu supports native inventory screens, dialogue/cutscenes, title/intro and
ending screens. Destructive world actions require active gameplay and may be disabled
in these views. During fades, resource transfers or actual EEPROM save writes,
the menu waits; not every transition frame can be interrupted.

## 5. Replaying the intro or ending

Open **WORLD / WARP → STORY INTRO** or **ENDING / CREDITS** from normal gameplay.
Press **A**, then **A** again to confirm. Do not start a replay from an active
conversation, cutscene or native inventory screen.

**RETURN FROM REPLAY** ends the replay and reloads the original room. All 1,204
native save bytes are backed up in RAM and restored on return. Temporary room actors
are not restored like an emulator savestate. Other mutations, cheats and timing
are temporarily suspended. Ending replays return before the normal save prompt;
normal story endings keep their original save behavior. Resetting the emulator
loses the temporary RAM return point.

## 6. Actor tools

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** exposes native categories, names and raw
kind/ID/type/type2/timer/subtimer/flags/parent/layer values. **QUICK SPAWN PRESETS**
do not restrict raw mode. There is no curated room/floor/variant allowlist;
missing native handlers and full native pools are still rejected. There are labels
for 546 native slots and 118 ground-item types; 25 entries intentionally remain **UNKNOWN**.

In **ROOM ACTOR LIST**, select an actor to freeze, remove or reposition it.
**LINK TO ACTOR** teleports Link to it; **ACTOR TO LINK** brings it to Link, within
the current room without a room warp. Managers with no common XYZ layout show
**NO GENERIC XYZ**: generic movement is unavailable, but freeze/remove still work.
Freeze an actor first if its native AI keeps moving it back.

A second Link, invalid bosses/types or missing room/script/parent dependencies
can crash or block the game. Removing a boss does **not** count as defeating it.
If necessary, reload without saving.

## 7. Enemy Freeze, Break Free and Infinite Time

- **Enemy Freeze:** includes bosses and visible non-colliding body parts. Projectiles in other actor categories may require individual freeze.
- **Break Free:** closes active text through the native close state and releases player control. It does not undo executed story scripts; later scripts may take control again.
- **Infinite Time:** covers Anju's chicken countdown, the Dark Hyrule Castle countdown, timed eye-switch activation and already-active charm/luck-potion durations. It does not freeze every timer, animation or cutscene. Turn it off to allow countdown evaluation/rewards and timed effects to finish.

## 8. Testing, problems and contributions

The release passed **166 automated mGBA suites: 53 USA + 113 EU/JP**. The project
tester also confirmed menu opening and No-Clip in their emulator. A complete
Android acceptance test for every region was not documented; real GBA hardware
was not tested. See [verification](VERIFICATION.md).

Report bugs in [Issues](https://github.com/Nimcoz/TMC-Practice/issues) with region,
patch version, emulator/version, reproduction steps and a screenshot if useful.
Do not upload ROMs, private saves or credentials. Contributions to the official
project require prior agreement with Nimcoz; bug reports do not. See
[contribution policy](../CONTRIBUTING.md) and [credits and rights](../THIRD_PARTY_NOTICES.md).
