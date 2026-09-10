"""Build the local 32 MiB USA practice ROM from the verified clean ROM."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

CLEAN_SHA1 = "B4BD50E4131B027C334547B4524E2DBBD4227130"
EXPLORATION_SHA256 = "A284A537A5A174769E355572B287A6A1172C025F5C50EB52569A9933DEC12361"
ROM_BASE = 0x08000000
ROM_SIZE = 0x02000000
MODULE_OFFSET = 0x01020000
EXPLORATION_OFFSET = 0x01010000


def parse_symbols(path: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for line in path.read_text(encoding="ascii").splitlines():
        fields = line.split()
        if len(fields) >= 3:
            try:
                symbols[fields[-1]] = int(fields[0], 16)
            except ValueError:
                pass
    return symbols


def require_symbol(symbols: dict[str, int], name: str) -> int:
    if name not in symbols:
        raise ValueError(f"missing linked symbol: {name}")
    return symbols[name]


def write16(rom: bytearray, address: int, value: int) -> None:
    struct.pack_into("<H", rom, address - ROM_BASE, value & 0xFFFF)


def write32(rom: bytearray, address: int, value: int) -> None:
    struct.pack_into("<I", rom, address - ROM_BASE, value & 0xFFFFFFFF)


def write_thumb_bl(rom: bytearray, source: int, target: int) -> None:
    displacement = target - (source + 4)
    if displacement & 1 or not -(1 << 22) <= displacement < (1 << 22):
        raise ValueError(f"Thumb BL out of range: {source:08X} -> {target:08X}")
    write16(rom, source, 0xF000 | ((displacement >> 12) & 0x7FF))
    write16(rom, source + 2, 0xF800 | ((displacement >> 1) & 0x7FF))


def write_tail_stub(rom: bytearray, address: int, target: int) -> None:
    if address & 3:
        raise ValueError("tail stub must be word aligned")
    write16(rom, address, 0x4B00)  # ldr r3, [pc, #0]
    write16(rom, address + 2, 0x4718)  # bx r3
    write32(rom, address + 4, target | 1)


def expect(rom: bytearray, address: int, expected_hex: str) -> None:
    expected = bytes.fromhex(expected_hex)
    actual = bytes(rom[address - ROM_BASE : address - ROM_BASE + len(expected)])
    if actual != expected:
        raise ValueError(f"ROM mismatch at {address:08X}: {actual.hex()} != {expected.hex()}")


def fix_header_checksum(rom: bytearray) -> None:
    rom[0xBD] = (-sum(rom[0xA0:0xBD]) - 0x19) & 0xFF


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--exploration", type=Path, required=True)
    parser.add_argument("--save", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--autotest", action="store_true")
    args = parser.parse_args()

    clean = args.rom.read_bytes()
    if hashlib.sha1(clean).hexdigest().upper() != CLEAN_SHA1:
        raise ValueError("input is not the verified The Minish Cap USA ROM")
    if len(clean) != 0x01000000:
        raise ValueError("verified input ROM must be exactly 16 MiB")

    module = args.module.read_bytes()
    if len(module) > 0x40000:
        raise ValueError("practice module exceeds its 256 KiB ROM region")
    exploration = args.exploration.read_bytes()
    if hashlib.sha256(exploration).hexdigest().upper() != EXPLORATION_SHA256:
        raise ValueError("exploration payload does not match immutable known-good baseline")

    symbols = parse_symbols(args.symbols)
    rom = bytearray(clean)
    rom.extend(b"\xFF" * (ROM_SIZE - len(rom)))
    rom[MODULE_OFFSET : MODULE_OFFSET + len(module)] = module
    rom[EXPLORATION_OFFSET : EXPLORATION_OFFSET + len(exploration)] = exploration
    # Main loop: preserve native MessageMain/FadeMain calls except while a modal
    # pauses a native screen. Reuse only the now-unreachable DebugTask table.
    expect(rom, 0x08055F58, "00 F0 7E FA FA F7 FA F8")
    expect(rom, 0x08109A30, "05 FA 05 08 99 FA 05 08")
    expect(rom, 0x08100CBC, "81 D3 0A 08 51 04 05 08")
    expect(rom, 0x08100CCC, "E1 35 0A 08")
    write_tail_stub(rom, 0x08109A30, require_symbol(symbols,"PracticeFrameTailDispatch"))
    write_thumb_bl(rom, 0x08055F58, 0x08109A30)
    write16(rom, 0x08055F5C, 0x46C0)
    write16(rom, 0x08055F5E, 0x46C0)
    if not (0x0203B000 <= require_symbol(symbols,"__modal_start__") <
            require_symbol(symbols,"__modal_end__") <= 0x0203D000):
        raise ValueError("modal scratch escapes separately reserved RAM")
    # Native DeleteEntity entry. Trampoline replays these four instructions,
    # including CMP flags consumed by the original BEQ at 0805E7C4.
    expect(rom, 0x0805E7BC, "30 B5 04 1C 60 68 00 28")
    expect(rom, 0x0804AA60, "70 B5 05 1C 0E 1C 13 F0 07 FE")
    write_tail_stub(rom, 0x0805E7BC, require_symbol(symbols, "Actors_DeleteDispatch"))
    expect(rom, 0x0805E900, "10 B5 04 1C 60 68 00 28")
    write_tail_stub(rom, 0x0805E900, require_symbol(symbols, "Actors_DeleteManagerDispatch"))
    # Source table copied by the native IWRAM updater during initialization.
    for kind, original in ((1,0x08016F29),(3,0x080011C5),(4,0x08016AE5),
                           (6,0x080174A5),(7,0x08017531),(8,0x08017339),(9,0x08017509)):
        address = 0x080B2248 + kind * 4
        expect(rom, address, struct.pack("<I", original).hex())
        write32(rom, address, require_symbol(symbols, "Actors_UpdateDispatch") | 1)
    expect(rom, 0x080A1270, "F0 B5 57 46 4E 46 45 46")
    write_tail_stub(rom, 0x080A1270, require_symbol(symbols, "MinigameTimerDispatch"))
    expect(rom, 0x080FCB18, "35 34 05 08")
    write32(rom, 0x080FCB18, require_symbol(symbols, "MinigameDarknutDispatch") | 1)
    expect(rom, 0x08081404, "00 B5 00 29 06 D0 01 1C")
    write_tail_stub(rom, 0x08081404, require_symbol(symbols, "Actors_ItemPickupDispatch"))

    expect(rom, 0x08079E42, "8E F7 9B FB")
    expect(rom, 0x08079E48, "00 F0 B6 FB")
    expect(rom, 0x08070B98, "97 F7 95 FF")
    expect(rom, 0x08074BF8, "10 B5 04 1C")
    expect(rom, 0x08074200, "10 B5 04 1C")
    expect(rom, 0x0811C150, "39 46 07 08")
    expect(rom, 0x0811C184, "9D 47 07 08")
    expect(rom, 0x08100CC4, "89 19 05 08")
    expect(rom, 0x08100CBC, "81 D3 0A 08")
    expect(rom, 0x08100CC0, "51 04 05 08")
    expect(rom, 0x080A753C, "30 90 12 08")
    expect(rom, 0x08077D90, "10 E0")
    expect(rom, 0x08077DA2, "07 D1")
    expect(rom, 0x080011FC, "FF F7 7A FE")
    expect(rom, 0x0805F9F0, "80 00 40 18 00 68 A1 F7")
    expect(rom, 0x0811E76C, "7D FC 07 08")
    expect(rom, 0x080526A0, "00 B5 08 4A 91 78 09 18")
    expect(rom, 0x080880F4, "00 F0 34 F8")
    expect(rom, 0x08088398, "0E 2B 00 02")
    # All five native figurine UI paths: initial index, navigation, ownership
    # range, scrollbar and name/description rows. Ownership remains independent.
    for address, original, replacement in (
        (0x080A468C, "82 22", 0x2288), (0x080A48E8, "82 21", 0x2188),
        (0x080A4958, "82 22", 0x2288), (0x080A49AA, "82 22", 0x2288),
        (0x080A4BF8, "82 21", 0x2188)):
        expect(rom, address, original)
        write16(rom, address, replacement)
    expect(rom, 0x0805F9F8, "7C FA 00 BD 30 9A 10 08")

    # sub_08077D38: PL_NO_CAP only defines alternate animations for Smith's
    # Sword, basic Shield and pickup. Other items previously passed an
    # uninitialized r6 to SetItemAnim (Four Sword => animation 0000/softlock).
    # Both default switch branches now use the native ItemDefinition.frameIndex
    # path at 08077DC4. Keep the three supported no-cap animations unchanged.
    write16(rom, 0x08077D90, 0xE018)
    write16(rom, 0x08077DA2, 0xD10F)
    write_tail_stub(rom, 0x080526A0, require_symbol(symbols, "Cheats_ModHealth"))
    # Unreachable remainder of the replaced native DebugTask, before FA04.
    write_tail_stub(rom, 0x0805F9F8, require_symbol(symbols, "FigurineAvailableDispatch"))
    write_thumb_bl(rom, 0x080880F4, 0x0805F9F8)

    movement = require_symbol(symbols, "MovementDispatch")
    exploration_dispatch = require_symbol(symbols, "ExplorationDispatch")
    respawn = require_symbol(symbols, "RespawnGuardDispatch")
    conveyor = require_symbol(symbols, "ConveyorDispatch")
    pit = require_symbol(symbols, "PitDispatch")
    minish_front = require_symbol(symbols, "SurfaceMinishFrontDispatch")
    surface21 = require_symbol(symbols, "Surface21Dispatch")
    game_wrapper = require_symbol(symbols, "GameTaskWrapper")
    debug_task = require_symbol(symbols, "PracticeDebugTask")
    subtasks = require_symbol(symbols, "gPracticeSubtasks")
    auto_wrapper = require_symbol(symbols, "AutoTestTaskWrapper")

    write_tail_stub(rom, 0x0810D504, movement)
    write_tail_stub(rom, 0x0810D50C, exploration_dispatch)
    write_thumb_bl(rom, 0x08079E42, 0x0810D504)
    write_thumb_bl(rom, 0x08079E48, 0x0810D50C)

    # The confirmed snap-back caller gets a nearby tail veneer. The veneer
    # dispatches to Nintendo when No Clip is OFF and returns immediately when ON.
    write_tail_stub(rom, 0x0805F9E8, respawn)
    write_thumb_bl(rom, 0x08070B98, 0x0805F9E8)
    write32(rom, 0x08100CD0, debug_task | 1)
    # Original DebugTask remainder is unreachable from the replaced task table.
    write_tail_stub(rom, 0x0805F9F0, require_symbol(symbols, "EnemyBehaviorDispatch"))
    write_thumb_bl(rom, 0x080011FC, 0x0805F9F0)
    write32(rom, 0x0811E76C, require_symbol(symbols, "ScrollFollowDispatch") | 1)

    # Entry veneers preserve full OFF/ON reversibility for the exact proven
    # castle/cliff conveyor and pit/sky-void paths.
    write_tail_stub(rom, 0x08074BF8, conveyor)
    write_tail_stub(rom, 0x08074200, pit)
    write32(rom, 0x0811C150, minish_front | 1)
    write32(rom, 0x0811C184, surface21 | 1)

    write32(rom, 0x08100CBC, require_symbol(symbols,"PracticeScreenTaskWrapper") | 1)
    write32(rom, 0x08100CCC, require_symbol(symbols,"PracticeScreenTaskWrapper") | 1)
    if args.autotest:
        write32(rom, 0x08100CBC, auto_wrapper | 1)
        write32(rom, 0x08100CC0, auto_wrapper | 1)
        write32(rom, 0x08100CC4, auto_wrapper | 1)
    else:
        write32(rom, 0x08100CC4, game_wrapper | 1)
    write32(rom, 0x080A753C, subtasks)
    fix_header_checksum(rom)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(rom)
    if args.save is not None:
        shutil.copyfile(args.save, args.output.with_suffix(".sav"))

    report = {
        "input_sha1": CLEAN_SHA1,
        "module_bytes": len(module),
        "module_sha256": hashlib.sha256(module).hexdigest().upper(),
        "exploration_bytes": len(exploration),
        "exploration_sha256": EXPLORATION_SHA256,
        "output_bytes": len(rom),
        "output_sha1": hashlib.sha1(rom).hexdigest().upper(),
        "output_sha256": hashlib.sha256(rom).hexdigest().upper(),
        "game_task_wrapper": f"{game_wrapper | 1:08X}",
        "subtask_table": f"{subtasks:08X}",
        "menu_subtask_index": 11,
        "hotkey": "L+R+SELECT",
        "autotest": args.autotest,
    }
    args.output.with_suffix(".build.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    fields = "page cursor pendingAction selectedArea selectedRoom noClip freezeEnemies debugHud speedMode lockDirection cameraMode cameraActive nudgeStep nudgeDirection inventoryDirty flagIndex flagBank knownFlag status menuArea menuRoom menuLocalFlagOffset flagOperation flagBefore flagAfter resourceCheats size menuTheme flagGroup collectionAction flagLogCount menuDungeonIndex sceneSkipTicks menuHotkey confirmHotkey bindingDraft bindingTarget favorites completionUndoValid completionUndoSlot completionReturn completionBackup".split()
    fields += "actorSelected actorIdentity actorMarks actorFilter actorSpawn".split()
    fields += "actorRaw actorRawKind actorRawId actorRawType actorRawType2 actorRawTimer actorRawSubtimer actorRawFlags actorRawParent actorRawLayer actorConfirmSpawn actorMoveStep actorExpert breakFreeTicks".split()
    fields += "modalMode modalSkipTail modalContext modalSaveEdits".split()
    fields += "sceneReplay sceneReturn sceneSeen sceneConfirm".split()
    offset = require_symbol(symbols, "gPracticeOffsets") - (ROM_BASE + MODULE_OFFSET)
    values = struct.unpack_from("<" + "I" * len(fields), module, offset)
    layout = dict(zip(fields, values))
    layout["actorSpawnCount"] = struct.unpack_from("<I", module, require_symbol(symbols, "gPracticeSpawnCount") - (ROM_BASE + MODULE_OFFSET))[0]
    layout["autoState"] = require_symbol(symbols, "gPracticeAutoTest")
    layout["modalBackup"] = require_symbol(symbols,"gPracticeModalBackup")
    layout["sceneBackup"] = require_symbol(symbols,"gPracticeSceneBackup")
    layout["modalBackupEnd"] = require_symbol(symbols,"__modal_end__")
    for key in ("actorNames", "actorNameCounts", "groundItemNames"):
        layout[key] = require_symbol(symbols, key)
    args.output.with_suffix(".layout.lua").write_text("return {\n" + "\n".join(f"  {k}={v}," for k,v in layout.items()) + "\n}\n", encoding="ascii")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
