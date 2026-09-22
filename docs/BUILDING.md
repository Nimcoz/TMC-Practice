# Reproducible local builds

Use Windows, PowerShell, Python 3 and GNU Arm Embedded GCC 14.3.1/binutils.
The compiler/emulator, original ROMs and private fixtures are not distributed.
Keep the directory structure intact; `work/tmc` contains the generator definitions.

## USA

Put the GNU Arm bin directory on PATH (or install it under `practice_menu/toolchain/bin`):

```powershell
./practice_menu/build.ps1 -Rom 'C:/your-roms/Legend of Zelda, The - The Minish Cap (USA).gba'
```

The historical output filename `TMC_Practice_USA_SCENES_TEST.gba` is retained.
The release version is identified by its source revision and output fingerprint.

## Europe and Japan

Place all three original ROMs, with the filenames in ROM_COMPATIBILITY.md,
in your local ROM directory. USA is also needed to validate the regional mappings.

```powershell
./practice_menu/build_regions.ps1 -RomDirectory 'C:/your-roms' -ToolchainBin 'C:/GNU-Arm/bin'
```

The script builds both regions and independently reconstructs all target ROM bytes.
Outputs are in `practice_menu/build/regions/EU` and `JP`. Compare their SHA-256
with ROM_COMPATIBILITY.md; keep every generated ROM local.

## BPS verification

```powershell
python practice_menu/tools/bps.py verify ORIGINAL.gba BUILT.gba PATCH.bps
```

The release patches use metadata `TMC Practice v1.0.1 | REGION | Original ROM only`,
where REGION is `USA`, `Europe` or `Japan`. Generating a BPS with different metadata
can change the patch hash while producing the same ROM. Verify the ROM bytes too.

## Tests and sealed Exploration component

Lua tests and verification scripts are under `practice_menu/tests`. Re-running
the full development harness requires a Lua-enabled mGBA plus locally configured
paths/private fixtures. It is not a hosted CI job and never downloads a ROM/save.
Regional fixtures are explicitly synthetic adaptations, not a released save converter.

The 836-byte Exploration component is preserved with its assembly reference,
mechanical reconstruction script and seed dependencies under
`KNOWN_GOOD_FINAL_EXPLORATION/`. The menu builds consume its verified binary;
regional builds relocate exactly six native literal words. Historical helper
defaults reference local development paths; supply/edit your own local paths
when separately reproducing that component. Do not alter it just to rename a release.

Since v1.0.1 this unchanged component is placed at `09070000`, outside both
the first 128 KiB of ROM expansion and the menu module's reserved area.
The lava-floor hook is a separate reversible wrapper in `hooks.c`.

`noclip_lava_gameplay.lua` uses an existing native Cave of Flames lava runway.
Set `TMC_HARDWARE_HARNESS` to the absolute path of the appropriate USA/regional
`gameplay_harness.lua` and run it with a same-region save through `run_probe.ps1`.
The test moves the fixture using native room transitions; it does not replace tiles,
force lava actions, repair coordinates during play or suppress test failures.
