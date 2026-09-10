# TMC Practice v1.0.0

First public-release package of the existing tested TMC Practice build,
with separate **USA, Europe and Japan BPS patches**.

## Download

- `TMC-Practice-v1.0.0-USA.bps` — original USA release.
- `TMC-Practice-v1.0.0-Europe.bps` — original European multilingual release.
- `TMC-Practice-v1.0.0-Japan.bps` — original Japanese release.
- `TMC-Practice-v1.0.0-patches.zip` — all three patches, English/German guides and checksums.
- `TMC-Practice-v1.0.0-source.zip` — source, build tools and tests; no ROM/save files.
- `SHA256SUMS.txt` — release download checksums.

Apply exactly one matching patch to a **clean original ROM**. Back up your save,
start the patched ROM normally, and press **L + R + Select**. Do not use an old
emulator savestate or apply this over another patch. No ROMs are provided.

## Features

Practice timer/debug HUD, No-Clip/exploration, inventory and element editing,
room warps, native intro/ending replays, flag/collection tools, free camera,
resource cheats, boss-inclusive freeze, Break Free, supported Infinite Time,
named raw actor spawning/editing and two-way actor teleport, persistent settings/themes.

For scenes: **WORLD / WARP → STORY INTRO / ENDING / CREDITS**, then A twice.
Use **RETURN FROM REPLAY** to return. Native game languages are retained;
Practice menu labels remain English.

## Verification

166 automated mGBA suites: 53 USA + 113 EU/JP. Fresh source rebuilds and BPS
application reproduce the exact tested ROMs. Includes native save/reload,
full intro/ending/save restoration, modal/HUD pixel comparisons, both regional
new-game starts and all five European languages. The tester additionally
confirmed menu opening and No-Clip; there is no recorded complete Android
acceptance run for each region and no physical-hardware validation.

These release patches are byte-identical to the previously tested deliveries.
No runtime feature changes were made during GitHub packaging.

## Important limitations

Arbitrary raw actors can crash or block incompatible rooms. Destructive progress
edits require confirmation but cannot replace a save backup. Twenty-five actor
labels remain UNKNOWN by agreement. Menu opening waits for native fades and
does not interrupt active save writes. Supported retail ROM hashes only.

Please report region, emulator/version and reproduction steps in Issues.
Do not upload ROMs, private saves or login information.

Unofficial fan modification. Not affiliated with or endorsed by Nintendo.
