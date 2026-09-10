import hashlib
import re
import struct
from pathlib import Path

SEEDS = (0x7AA9648F, 0x7FAE6994, 0xC0EFAAD5, 0x42712C57)
DELTA = 0x9E3779B9
MASK = 0xFFFFFFFF


def dec(a, v):
    s0, s1, s2, s3 = SEEDS
    rs = (32 * DELTA) & MASK
    for _ in range(32):
        v = (v - (((((a << 4) & MASK) + s2) ^ ((a + rs) & MASK)) ^ (((a >> 5) + s3) & MASK))) & MASK
        a = (a - (((((v << 4) & MASK) + s0) ^ ((v + rs) & MASK)) ^ (((v >> 5) + s1) & MASK))) & MASK
        rs = (rs - DELTA) & MASK
    return a, v


def enc(a, v):
    s0, s1, s2, s3 = SEEDS
    rs = 0
    for _ in range(32):
        rs = (rs + DELTA) & MASK
        a = (a + (((((v << 4) & MASK) + s0) ^ ((v + rs) & MASK)) ^ (((v >> 5) + s1) & MASK))) & MASK
        v = (v + (((((a << 4) & MASK) + s2) ^ ((a + rs) & MASK)) ^ (((a >> 5) + s3) & MASK))) & MASK
    return a, v


def parse_codes(path):
    result = []
    for line in Path(path).read_text().splitlines():
        match = re.fullmatch(r"([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})", line.strip())
        if match:
            result.append(tuple(int(value, 16) for value in match.groups()))
    return result


def rom_halfword_patch(rom_address, value):
    assert 0x08000000 <= rom_address < 0x0A000000
    assert rom_address % 2 == 0 and 0 <= value <= 0xFFFF
    halfword_index = (rom_address - 0x08000000) >> 1
    return [(0, 0x18000000 | halfword_index), (value, 0)]


HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
BASELINE = ROOT / "work" / "input" / "TMC_CODEX_LOW_USAGE" / "01_KNOWN_GOOD_RECOVERY_FIX3_ARv3.txt"
SEED = HERE / "FIX5_camera_target_fix_payload.bin"
ROM = Path(r"C:\Users\user\Downloads\Legend of Zelda, The - The Minish Cap (USA).gba")
PAYLOAD_OUT = HERE / "FINAL_verified_respawn_guard_payload.bin"
OUTPUT = HERE / "TMC_USA_GLOBAL_EXPLORATION_FINAL_LINKBOY_ARv3.txt"

# This is the already validated FIX-3-derived payload with the fixed-camera
# predicate correction.  The unused disproven static-writer wrapper is removed
# below without recompiling or altering any executed instruction.
seed = bytearray(SEED.read_bytes())
assert len(seed) == 856
assert hashlib.sha256(seed).hexdigest().upper() == "5CF4E88F69B166062EBC46E0B409956DE704AFA301511A67E867CFB7F882C706"

# Layout of the seed:
#   000..30B executed FIX-3-derived payload
#   30C..31D disproven/unneeded static-writer wrapper
#   31E..323 two register-call trampolines + NOP
#   324..353 literals used by executed code
#   354..357 wrapper-only literal 080044AF
# Delete the wrapper and its literal, keep the trampolines, and move the live
# literal pool 0x10 bytes earlier.  Every affected Thumb LDR-literal is fixed
# mechanically and checked against its old/new effective address.
OLD_POOL_START = 0x324
OLD_POOL_END = 0x354
POOL_SHIFT = 0x10
prefix = bytearray(seed[:0x30C])
literal_fixups = 0
for offset in range(0, len(prefix), 2):
    instruction = struct.unpack_from("<H", prefix, offset)[0]
    if instruction & 0xF800 != 0x4800:
        continue
    pc_base = (offset + 4) & ~3
    old_target = pc_base + (instruction & 0xFF) * 4
    if OLD_POOL_START <= old_target < OLD_POOL_END:
        new_target = old_target - POOL_SHIFT
        assert (new_target - pc_base) % 4 == 0
        new_imm = (new_target - pc_base) // 4
        assert 0 <= new_imm <= 0xFF
        struct.pack_into("<H", prefix, offset, (instruction & 0xFF00) | new_imm)
        literal_fixups += 1

# The register-call trampolines move from 31E/320 to 30C/30E.  Retarget every
# local Thumb-1 BL which reached either trampoline; all other BLs are unchanged.
branch_fixups = 0
old_trampolines = {0x31E: 0x30C, 0x320: 0x30E}
for offset in range(0, len(prefix) - 2, 2):
    high, low = struct.unpack_from("<HH", prefix, offset)
    if high & 0xF800 != 0xF000 or low & 0xF800 != 0xF800:
        continue
    upper = high & 0x7FF
    if upper & 0x400:
        upper -= 0x800
    target = offset + 4 + (upper << 12) + ((low & 0x7FF) << 1)
    if target not in old_trampolines:
        continue
    new_target = old_trampolines[target]
    displacement = new_target - (offset + 4)
    assert displacement % 2 == 0 and -(1 << 22) <= displacement < (1 << 22)
    new_high = 0xF000 | ((displacement >> 12) & 0x7FF)
    new_low = 0xF800 | ((displacement >> 1) & 0x7FF)
    struct.pack_into("<HH", prefix, offset, new_high, new_low)
    branch_fixups += 1

payload = prefix + seed[0x31E:0x324] + bytes.fromhex("C0 46") + seed[OLD_POOL_START:OLD_POOL_END]
assert len(payload) == 836
assert len(payload) % 4 == 0
assert literal_fixups >= 20
assert branch_fixups >= 4
assert payload[0x64:0x66] == bytes.fromhex("10 B5")
assert payload[0x8C:0x8E] == bytes.fromhex("F0 B5")
assert payload[0x30C:0x312] == bytes.fromhex("18 47 30 47 C0 46")
assert payload[0x314:0x318] == bytes.fromhex("60 11 00 03")
assert bytes.fromhex("AF 44 00 08") not in payload
PAYLOAD_OUT.write_bytes(payload)

baseline_enc = parse_codes(BASELINE)
baseline_raw = [dec(*code) for code in baseline_enc]
assert all(enc(*raw) == encrypted for raw, encrypted in zip(baseline_raw, baseline_enc))
baseline_rom_raw = baseline_raw[-32:]

# Proven normal-action caller: LR=08070B9D when sub_08008AC6 is entered.
# Replace only its BL at 08070B98 with two Thumb NOPs.  The second caller and
# RespawnPlayer itself remain untouched for every special state.
rom = ROM.read_bytes()
assert hashlib.sha1(rom).hexdigest().upper() == "B4BD50E4131B027C334547B4524E2DBBD4227130"
assert rom[0x70B98:0x70B9C] == bytes.fromhex("97 F7 95 FF")
callers = []
for offset in range(0, len(rom) - 3, 2):
    high, low = struct.unpack_from("<HH", rom, offset)
    if high & 0xF800 != 0xF000 or low & 0xF800 != 0xF800:
        continue
    upper = high & 0x7FF
    if upper & 0x400:
        upper -= 0x800
    target = 0x08000000 + offset + 4 + (upper << 12) + ((low & 0x7FF) << 1)
    if target == 0x08008AC6:
        callers.append(0x08000000 + offset)
assert callers == [0x08070B98, 0x08073EE8]
respawn_guard_raw = []
respawn_guard_raw += rom_halfword_patch(0x08070B98, 0x46C0)
respawn_guard_raw += rom_halfword_patch(0x08070B9A, 0x46C0)

# Exact remaining castle/cliff counter-path proven in mGBA: conveyer_push at
# 08074BF8 sets field_0xa/mobility/PL_CONVEYOR_PUSHED and then its call at
# 08074C38 reaches the coordinate writers 0806F6D8/0806F700.  Returning at
# the helper entry prevents both the state lockout and the backward write.
# Its four callers are already gated to ordinary non-Minish surface actions.
assert rom[0x74BF8:0x74BFA] == bytes.fromhex("10 B5")
surface_counter_raw = []
surface_counter_raw += rom_halfword_patch(0x08074BF8, 0x4770)

# Explicit no-clip pit/void support: SurfaceAction_Pit is the ordinary grounded
# dispatcher that queues PLAYER_FALL on sky void and normal chasm tiles.  Its
# own sub_080741C4 guard already excludes airborne/special movement.  Returning
# at the entry therefore suppresses only the grounded pit fall action.
assert rom[0x74200:0x74202] == bytes.fromhex("10 B5")
pit_walk_raw = []
pit_walk_raw += rom_halfword_patch(0x08074200, 0x4770)

ram_raw = [
    (0x0423F000 + offset, struct.unpack_from("<I", payload, offset)[0])
    for offset in range(0, len(payload), 4)
]
raw = ram_raw + baseline_rom_raw + respawn_guard_raw + surface_counter_raw + pit_walk_raw
encrypted = [enc(*code) for code in raw]
assert [dec(*code) for code in encrypted] == raw
assert len(ram_raw) == 209
assert len(baseline_rom_raw) == 32
assert len(respawn_guard_raw) == 4
assert len(surface_counter_raw) == 2
assert len(pit_walk_raw) == 2
assert len(encrypted) == 249

OUTPUT.write_text("\n".join(f"{a:08X} {v:08X}" for a, v in encrypted) + "\n")
print(f"payload_bytes={len(payload)}")
print(f"literal_fixups={literal_fixups}")
print(f"branch_fixups={branch_fixups}")
print(f"ram_lines={len(ram_raw)}")
print(f"rom_patch_lines={len(baseline_rom_raw) + len(respawn_guard_raw) + len(surface_counter_raw) + len(pit_walk_raw)}")
print(f"total_arv3_lines={len(encrypted)}")
print("crypto_roundtrip=PASS")
print("normal_respawn_call_08070B98=NOP_NOP")
print("surface_counter_helper_08074BF8=BX_LR")
print("pit_surface_action_08074200=BX_LR")
print(OUTPUT)
