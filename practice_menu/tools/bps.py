"""Small, dependency-free BPS patch creator, applier, and verifier.

The creator is intentionally deterministic.  It uses SourceRead for unchanged
ROM ranges, TargetRead for new data, and overlapping TargetCopy commands for
long fill runs in the expanded 32 MiB image.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zlib
from pathlib import Path


def encode_number(value: int) -> bytes:
    if value < 0:
        raise ValueError("BPS numbers cannot be negative")
    output = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value == 0:
            output.append(byte | 0x80)
            return bytes(output)
        output.append(byte)
        value -= 1


def decode_number(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 1
    while True:
        if offset >= len(data):
            raise ValueError("truncated BPS number")
        byte = data[offset]
        offset += 1
        value += (byte & 0x7F) * shift
        if byte & 0x80:
            return value, offset
        shift <<= 7
        value += shift


def encode_signed(value: int) -> bytes:
    return encode_number((abs(value) << 1) | (1 if value < 0 else 0))


def decode_signed(data: bytes, offset: int) -> tuple[int, int]:
    value, offset = decode_number(data, offset)
    magnitude = value >> 1
    return (-magnitude if value & 1 else magnitude), offset


def action(kind: int, length: int) -> bytes:
    if length <= 0:
        raise ValueError("BPS action length must be positive")
    return encode_number(((length - 1) << 2) | kind)


def repeated_run(data: bytes, offset: int) -> int:
    byte = data[offset]
    end = offset + 1
    while end < len(data) and data[end] == byte:
        end += 1
    return end - offset


def create_patch(source: bytes, target: bytes, metadata: bytes = b"") -> bytes:
    patch = bytearray(b"BPS1")
    patch += encode_number(len(source))
    patch += encode_number(len(target))
    patch += encode_number(len(metadata))
    patch += metadata
    position = 0
    target_relative = 0

    while position < len(target):
        if position < len(source) and target[position] == source[position]:
            end = position + 1
            limit = min(len(source), len(target))
            while end < limit and target[end] == source[end]:
                end += 1
            patch += action(0, end - position)  # SourceRead
            position = end
            continue

        run = repeated_run(target, position)
        if run >= 4:
            if position == 0 or target[position - 1] != target[position]:
                patch += action(1, 1)  # Seed one byte for an overlapping copy.
                patch.append(target[position])
                position += 1
                run -= 1
            copy_from = position - 1
            patch += action(3, run)  # TargetCopy
            patch += encode_signed(copy_from - target_relative)
            target_relative = copy_from + run
            position += run
            continue

        start = position
        position += 1
        while position < len(target):
            if position < len(source) and target[position] == source[position]:
                break
            if repeated_run(target, position) >= 4:
                break
            position += 1
        patch += action(1, position - start)  # TargetRead
        patch += target[start:position]

    patch += struct.pack("<I", zlib.crc32(source) & 0xFFFFFFFF)
    patch += struct.pack("<I", zlib.crc32(target) & 0xFFFFFFFF)
    patch += struct.pack("<I", zlib.crc32(patch) & 0xFFFFFFFF)
    return bytes(patch)


def apply_patch(source: bytes, patch: bytes) -> tuple[bytes, bytes]:
    if len(patch) < 16 or patch[:4] != b"BPS1":
        raise ValueError("not a BPS1 patch")
    stored_patch_crc = struct.unpack_from("<I", patch, len(patch) - 4)[0]
    if zlib.crc32(patch[:-4]) & 0xFFFFFFFF != stored_patch_crc:
        raise ValueError("BPS patch CRC32 mismatch")

    offset = 4
    source_size, offset = decode_number(patch, offset)
    target_size, offset = decode_number(patch, offset)
    metadata_size, offset = decode_number(patch, offset)
    if source_size != len(source):
        raise ValueError(f"source size mismatch: {len(source)} != {source_size}")
    metadata = patch[offset : offset + metadata_size]
    offset += metadata_size
    actions_end = len(patch) - 12
    output = bytearray()
    source_relative = 0
    target_relative = 0

    while offset < actions_end and len(output) < target_size:
        encoded, offset = decode_number(patch, offset)
        kind = encoded & 3
        length = (encoded >> 2) + 1
        if len(output) + length > target_size:
            raise ValueError("BPS action exceeds target size")
        if kind == 0:  # SourceRead
            start = len(output)
            end = start + length
            if end > len(source):
                raise ValueError("SourceRead exceeds source")
            output += source[start:end]
        elif kind == 1:  # TargetRead
            if offset + length > actions_end:
                raise ValueError("TargetRead exceeds patch data")
            output += patch[offset : offset + length]
            offset += length
        elif kind == 2:  # SourceCopy
            delta, offset = decode_signed(patch, offset)
            source_relative += delta
            if source_relative < 0 or source_relative + length > len(source):
                raise ValueError("SourceCopy exceeds source")
            output += source[source_relative : source_relative + length]
            source_relative += length
        else:  # TargetCopy; byte-at-a-time permits overlapping copies.
            delta, offset = decode_signed(patch, offset)
            target_relative += delta
            for _ in range(length):
                if target_relative < 0 or target_relative >= len(output):
                    raise ValueError("TargetCopy references unavailable output")
                output.append(output[target_relative])
                target_relative += 1

    if offset != actions_end or len(output) != target_size:
        raise ValueError("BPS action stream did not end at the declared target size")
    source_crc, target_crc = struct.unpack_from("<II", patch, actions_end)
    if zlib.crc32(source) & 0xFFFFFFFF != source_crc:
        raise ValueError("BPS source CRC32 mismatch")
    if zlib.crc32(output) & 0xFFFFFFFF != target_crc:
        raise ValueError("BPS target CRC32 mismatch")
    return bytes(output), metadata


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def main() -> None:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    create = subparsers.add_parser("create")
    create.add_argument("source", type=Path)
    create.add_argument("target", type=Path)
    create.add_argument("patch", type=Path)
    create.add_argument("--metadata", default="")
    apply = subparsers.add_parser("apply")
    apply.add_argument("source", type=Path)
    apply.add_argument("patch", type=Path)
    apply.add_argument("output", type=Path)
    verify = subparsers.add_parser("verify")
    verify.add_argument("source", type=Path)
    verify.add_argument("target", type=Path)
    verify.add_argument("patch", type=Path)
    args = parser.parse_args()

    source = args.source.read_bytes()
    if args.command == "create":
        target = args.target.read_bytes()
        result = create_patch(source, target, args.metadata.encode("utf-8"))
        args.patch.parent.mkdir(parents=True, exist_ok=True)
        args.patch.write_bytes(result)
        print(f"BPS bytes={len(result)} sha256={digest(result)}")
    elif args.command == "apply":
        result, metadata = apply_patch(source, args.patch.read_bytes())
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(result)
        print(f"target bytes={len(result)} sha256={digest(result)} metadata={metadata.decode('utf-8')}")
    else:
        expected = args.target.read_bytes()
        result, metadata = apply_patch(source, args.patch.read_bytes())
        if result != expected:
            raise ValueError("BPS output does not exactly match target")
        print(f"BPS VERIFY PASS target_sha256={digest(result)} metadata={metadata.decode('utf-8')}")


if __name__ == "__main__":
    main()
