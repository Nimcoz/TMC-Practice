"""Independent byte-level verification for the USA Practice Menu release."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

CLEAN_SHA1 = "B4BD50E4131B027C334547B4524E2DBBD4227130"
SAVE_SHA256 = "80909955A162A18D85B6AEC8BCC19EC0F198EE5EA0183B3EA03A3B8E877451F8"
EXPLORATION_SHA256 = "A284A537A5A174769E355572B287A6A1172C025F5C50EB52569A9933DEC12361"
MANIFEST_SHA256 = "971DE38A538C8E43B4DDE6DF6255A21D5B474B8A742409518C0E26DC7705B096"
ROM_BASE = 0x08000000
ROM_SIZE = 0x02000000
MODULE_OFFSET = 0x01020000
EXPLORATION_OFFSET = 0x01010000


def sha1(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest().upper()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def parse_symbols(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}
    for line in path.read_text(encoding="ascii").splitlines():
        fields = line.split()
        if len(fields) >= 3:
            try:
                result[fields[-1]] = int(fields[0], 16)
            except ValueError:
                pass
    return result


def write16(rom: bytearray, address: int, value: int) -> None:
    struct.pack_into("<H", rom, address - ROM_BASE, value & 0xFFFF)


def write32(rom: bytearray, address: int, value: int) -> None:
    struct.pack_into("<I", rom, address - ROM_BASE, value & 0xFFFFFFFF)


def write_thumb_bl(rom: bytearray, source: int, target: int) -> None:
    displacement = target - (source + 4)
    write16(rom, source, 0xF000 | ((displacement >> 12) & 0x7FF))
    write16(rom, source + 2, 0xF800 | ((displacement >> 1) & 0x7FF))


def write_tail_stub(rom: bytearray, address: int, target: int) -> None:
    write16(rom, address, 0x4B00)
    write16(rom, address + 2, 0x4718)
    write32(rom, address + 4, target | 1)


def expected_rom(clean: bytes, module: bytes, exploration: bytes, symbols: dict[str, int], autotest: bool) -> bytes:
    rom = bytearray(clean)
    rom.extend(b"\xFF" * (ROM_SIZE - len(rom)))
    rom[MODULE_OFFSET : MODULE_OFFSET + len(module)] = module
    rom[EXPLORATION_OFFSET : EXPLORATION_OFFSET + len(exploration)] = exploration
    write_tail_stub(rom,0x08109A30,symbols["PracticeFrameTailDispatch"])
    write_thumb_bl(rom,0x08055F58,0x08109A30)
    write16(rom,0x08055F5C,0x46C0);write16(rom,0x08055F5E,0x46C0)
    write32(rom,0x08100CBC,symbols["PracticeScreenTaskWrapper"]|1)
    write32(rom,0x08100CCC,symbols["PracticeScreenTaskWrapper"]|1)
    write_tail_stub(rom, 0x0805E7BC, symbols["Actors_DeleteDispatch"])
    write_tail_stub(rom, 0x0805E900, symbols["Actors_DeleteManagerDispatch"])
    for kind in (1,3,4,6,7,8,9):
        write32(rom, 0x080B2248 + kind * 4, symbols["Actors_UpdateDispatch"] | 1)
    write_tail_stub(rom, 0x080A1270, symbols["MinigameTimerDispatch"])
    write32(rom, 0x080FCB18, symbols["MinigameDarknutDispatch"] | 1)
    write_tail_stub(rom, 0x08081404, symbols["Actors_ItemPickupDispatch"])
    write_tail_stub(rom, 0x0810D504, symbols["MovementDispatch"])
    write_tail_stub(rom, 0x0810D50C, symbols["ExplorationDispatch"])
    write_thumb_bl(rom, 0x08079E42, 0x0810D504)
    write_thumb_bl(rom, 0x08079E48, 0x0810D50C)
    write_tail_stub(rom, 0x0805F9E8, symbols["RespawnGuardDispatch"])
    write_thumb_bl(rom, 0x08070B98, 0x0805F9E8)
    write32(rom, 0x08100CD0, symbols["PracticeDebugTask"] | 1)
    write_tail_stub(rom, 0x08074BF8, symbols["ConveyorDispatch"])
    write_tail_stub(rom, 0x08074200, symbols["PitDispatch"])
    write32(rom, 0x0811C150, symbols["SurfaceMinishFrontDispatch"] | 1)
    write32(rom, 0x0811C184, symbols["Surface21Dispatch"] | 1)
    write16(rom, 0x08077D90, 0xE018)
    write16(rom, 0x08077DA2, 0xD10F)
    write_tail_stub(rom, 0x080526A0, symbols["Cheats_ModHealth"])
    write_tail_stub(rom, 0x0805F9F0, symbols["EnemyBehaviorDispatch"])
    write_thumb_bl(rom, 0x080011FC, 0x0805F9F0)
    write32(rom, 0x0811E76C, symbols["ScrollFollowDispatch"] | 1)
    write_tail_stub(rom, 0x0805F9F8, symbols["FigurineAvailableDispatch"])
    write_thumb_bl(rom, 0x080880F4, 0x0805F9F8)
    for address, opcode in ((0x080A468C,0x2288),(0x080A48E8,0x2188),
                            (0x080A4958,0x2288),(0x080A49AA,0x2288),(0x080A4BF8,0x2188)):
        write16(rom,address,opcode)
    if autotest:
        for address in (0x08100CBC, 0x08100CC0, 0x08100CC4):
            write32(rom, address, symbols["AutoTestTaskWrapper"] | 1)
    else:
        write32(rom, 0x08100CC4, symbols["GameTaskWrapper"] | 1)
    write32(rom, 0x080A753C, symbols["gPracticeSubtasks"])
    rom[0xBD] = (-sum(rom[0xA0:0xBD]) - 0x19) & 0xFF
    return bytes(rom)


def verify_archive(archive: Path) -> dict[str, object]:
    manifest = archive / "ARCHIVE_MANIFEST_SHA256.txt"
    if sha256(manifest.read_bytes()) != MANIFEST_SHA256:
        raise ValueError("immutable Exploration manifest hash changed")
    checked = 0
    for line in manifest.read_text(encoding="utf-8").splitlines():
        expected, relative = line.split("  ", 1)
        actual = sha256((archive / Path(relative)).read_bytes())
        if actual != expected:
            raise ValueError(f"immutable Exploration file changed: {relative}")
        checked += 1
    return {"manifest_sha256": MANIFEST_SHA256, "files_checked": checked}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--clean", type=Path, required=True)
    parser.add_argument("--normal", type=Path, required=True)
    parser.add_argument("--autotest", type=Path, required=True)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--exploration", type=Path, required=True)
    parser.add_argument("--save", type=Path)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()

    clean = args.clean.read_bytes()
    normal = args.normal.read_bytes()
    autotest = args.autotest.read_bytes()
    module = args.module.read_bytes()
    exploration = args.exploration.read_bytes()
    symbols = parse_symbols(args.symbols)
    if len(clean) != 0x01000000 or sha1(clean) != CLEAN_SHA1:
        raise ValueError("clean ROM is not the verified USA ROM")
    if len(normal) != ROM_SIZE or len(autotest) != ROM_SIZE:
        raise ValueError("release ROMs must be exactly 32 MiB")
    if sha256(exploration) != EXPLORATION_SHA256:
        raise ValueError("sealed Exploration payload changed")
    if args.save is not None and sha256(args.save.read_bytes()) != SAVE_SHA256:
        raise ValueError("test save changed")
    if normal != expected_rom(clean, module, exploration, symbols, False):
        raise ValueError("normal ROM contains bytes outside the deterministic build definition")
    if autotest != expected_rom(clean, module, exploration, symbols, True):
        raise ValueError("autotest ROM contains bytes outside the deterministic build definition")

    differences = [index for index, (a, b) in enumerate(zip(normal, autotest)) if a != b]
    allowed = set(range(0x100CBC, 0x100CC8))
    if not differences or any(index not in allowed for index in differences):
        raise ValueError("normal/autotest differences escape the three task-table words")
    if struct.unpack_from("<I", normal, 0x100CBC)[0] != symbols["PracticeScreenTaskWrapper"] | 1 or \
       struct.unpack_from("<I", normal, 0x100CC0)[0] != 0x08050451:
        raise ValueError("normal ROM Title wrapper/File Select handler mismatch")
    if struct.unpack_from("<I",normal,0x100CCC)[0] != symbols["PracticeScreenTaskWrapper"] | 1:
        raise ValueError("Staffroll wrapper mismatch")
    if not (0x0203B000 <= symbols["__modal_start__"] < symbols["__modal_end__"] <= 0x0203D000):
        raise ValueError("modal memory range mismatch")
    # USA's last native EWRAM allocation is the complete 82-track audio pool.
    tracks = [struct.unpack_from("<IIB", clean, 0xA11C3C + i * 12)[1:] for i in range(32)]
    cursor = 0x02036BC0
    for start, count in tracks:
        if start != cursor or not count:
            raise ValueError("native audio allocation table changed")
        cursor += count * 0x50
    if cursor != 0x02038560 or sum(count for _, count in tracks) != 82:
        raise ValueError("native audio pool bound changed")
    if struct.unpack_from("<III", clean, 0x55FE8) != (0x080B2CD8, 0x080B2CD8, cursor):
        raise ValueError("native end-of-EWRAM overlay is no longer empty")
    if not (cursor < symbols["__modal_start__"] and symbols["gPracticeState"] == 0x0203D000 and symbols["__bss_end__"] <= 0x0203F000):
        raise ValueError("native/modal/Practice allocations overlap")
    if struct.unpack_from("<I", normal, 0x100CC4)[0] != symbols["GameTaskWrapper"] | 1:
        raise ValueError("normal ROM Game handler is not GameTaskWrapper")

    report = {
        "result": "PASS",
        "clean_sha1": CLEAN_SHA1,
        "normal_bytes": len(normal),
        "normal_sha1": sha1(normal),
        "normal_sha256": sha256(normal),
        "autotest_sha256": sha256(autotest),
        "module_bytes": len(module),
        "module_sha256": sha256(module),
        "exploration_sha256": EXPLORATION_SHA256,
        "save_sha256": SAVE_SHA256 if args.save is not None else "NOT_CHECKED",
        "normal_autotest_diff_count": len(differences),
        "normal_autotest_diff_offsets": [f"0x{index:08X}" for index in differences],
        "title_handler": f"0x{symbols['PracticeScreenTaskWrapper'] | 1:08X}",
        "staffroll_handler": f"0x{symbols['PracticeScreenTaskWrapper'] | 1:08X}",
        "modal_scratch_start": f"0x{symbols['__modal_start__']:08X}",
        "modal_scratch_end": f"0x{symbols['__modal_end__']:08X}",
        "native_audio_tracks_verified": 82,
        "native_ewram_allocations_end": "0x02038560",
        "native_end_overlay_bytes": 0,
        "practice_bss_end": f"0x{symbols['__bss_end__']:08X}",
        "file_select_handler": "0x08050451",
        "game_handler": f"0x{symbols['GameTaskWrapper'] | 1:08X}",
        "archive": verify_archive(args.archive),
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
