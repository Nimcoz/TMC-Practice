# TMC Practice v1.0.1

A No-Clip update for **The Legend of Zelda: The Minish Cap**.

## Changes

- **Hardware freeze fix:** relocates the unchanged No-Clip payload away from the
  ROM area that returned incorrect data on the tested EZ-Flash Omega Definitive
  Edition. The isolated EU fix was confirmed on a GBA SP AGS-101.
- **Lava walking:** No-Clip allows walking over lava floors without burning or
  bouncing. Turning it off restores the original lava reaction.
- Updated USA, Europe and Japan patches, source, tests and six language guides.

## Downloads

Choose the BPS patch for your original ROM's region.

| ROM region | Download |
| --- | --- |
| USA | [USA BPS](https://github.com/Nimcoz/TMC-Practice/releases/download/v1.0.1/TMC-Practice-v1.0.1-USA.bps) |
| Europe | [Europe BPS](https://github.com/Nimcoz/TMC-Practice/releases/download/v1.0.1/TMC-Practice-v1.0.1-Europe.bps) |
| Japan | [Japan BPS](https://github.com/Nimcoz/TMC-Practice/releases/download/v1.0.1/TMC-Practice-v1.0.1-Japan.bps) |

[Source code](https://github.com/Nimcoz/TMC-Practice/releases/download/v1.0.1/TMC-Practice-v1.0.1-source.zip) — includes build tools and tests.

## Getting started

1. Back up your normal in-game save.
2. Apply one matching BPS patch to an **unmodified original ROM**, not an older Practice or diagnostic ROM. Do not bypass a source-checksum mismatch.
3. Open the patched ROM in your emulator with a normal save from the same region.
4. Press **L + R + Select** to open the menu. Saved custom bindings take precedence.

Do not stack patches or load emulator savestates from older builds.
The Practice menu is in **English**; the original game's language is preserved.

## Highlights

- Practice timer and on-screen debug information.
- No-Clip through walls, cliffs, pits and lava floors; free camera and movement tools.
- Room warps, favorites and intro/ending replays.
- Inventory, elements, flags and collection controls.
- Actor spawning, freeze, editing and two-way teleportation.
- Resource cheats, Enemy Freeze, Break Free and Infinite Time.
- Persistent settings, themes and custom button combinations.

## Verification

17 focused mGBA suites across all three regions, independent ROM reconstruction,
byte-identical public-source rebuilds and checked BPS round trips. The combined
lava build has not been separately hardware-confirmed; physical confirmation
applies to the isolated EU freeze fix. Lava walking is not general invincibility
against enemies or every flame/projectile.

## Guides

[English](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_EN.md) · [Deutsch](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_DE.md) · [Français](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_FR.md) · [Español](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_ES.md) · [Italiano](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_IT.md) · [日本語](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/README_JA.md)

[ROM compatibility](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/ROM_COMPATIBILITY.md) · [Feature details and safety](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/USER_GUIDE.md) · [Test results](https://github.com/Nimcoz/TMC-Practice/blob/main/docs/VERIFICATION.md)

## Support

Keep a backup before editing progress or spawning actors: invalid combinations can crash or block the game.
[Report a bug](https://github.com/Nimcoz/TMC-Practice/issues) with your region, emulator/version and steps to reproduce it. Do not upload ROMs, private saves or credentials.

[Contributions by prior agreement](https://github.com/Nimcoz/TMC-Practice/blob/main/CONTRIBUTING.md) · [Credits](https://github.com/Nimcoz/TMC-Practice/blob/main/CREDITS.md) · [Rights and third-party notices](https://github.com/Nimcoz/TMC-Practice/blob/main/THIRD_PARTY_NOTICES.md)

Unofficial fan project, not affiliated with or endorsed by Nintendo. No ROMs are distributed.
