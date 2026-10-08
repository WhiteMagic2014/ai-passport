#!/usr/bin/env python3
"""Merge ESP32 build artifacts into a single flashable binary.

Reads flasher_args.json from the build directory, places each component
at its correct offset with 0xFF padding, and writes a merged .bin file
that can be flashed from 0x0 with esptool or the FoloToy web flasher.

Usage:
    python3 merge_bin.py [--build-dir build] [--output merged.bin]
"""

import argparse
import json
import sys
from pathlib import Path


def merge(flasher_args: dict, build_dir: Path, output: Path) -> None:
    flash_files = flasher_args.get("flash_files", {})
    if not flash_files:
        print("Error: no flash_files found in flasher_args.json", file=sys.stderr)
        sys.exit(1)

    # Parse offsets and sort by address
    parts = []
    for offset_str, rel_path in flash_files.items():
        offset = int(offset_str, 0)
        abs_path = build_dir / rel_path
        if not abs_path.exists():
            print(f"Error: {abs_path} not found", file=sys.stderr)
            sys.exit(1)
        parts.append((offset, abs_path, rel_path))
    parts.sort(key=lambda p: p[0])

    # Determine total size (last file offset + its size, aligned to 4KB)
    last_offset, last_path, _ = parts[-1]
    last_size = last_path.stat().st_size
    total_size = last_offset + last_size
    # Align to 4KB boundary
    total_size = (total_size + 0xFFF) & ~0xFFF

    # Build merged binary
    buf = bytearray(b"\xFF" * total_size)
    for offset, abs_path, rel_path in parts:
        data = abs_path.read_bytes()
        buf[offset:offset + len(data)] = data
        print(f"  0x{offset:06X}  {rel_path}  ({len(data):,} bytes)")

    output.write_bytes(buf)
    print(f"\nMerged: {output} ({len(buf):,} bytes)")


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description="Merge ESP32 build artifacts into a single binary")
    parser.add_argument("--build-dir", type=Path, default=script_dir / "build",
                        help="Build output directory (default: ./build)")
    parser.add_argument("--output", type=Path, default=None,
                        help="Output file path (default: <build-dir>/<project>_merged.bin)")
    args = parser.parse_args()

    flasher_json = args.build_dir / "flasher_args.json"
    if not flasher_json.exists():
        print(f"Error: {flasher_json} not found. Run 'idf.py build' first.", file=sys.stderr)
        return 1

    flasher_args = json.loads(flasher_json.read_text())

    # Derive output name from the app binary
    app_file = flasher_args.get("app", {}).get("file", "app.bin")
    app_stem = Path(app_file).stem
    if args.output is None:
        args.output = args.build_dir / f"{app_stem}_merged.bin"

    print(f"Build dir: {args.build_dir}")
    print(f"Flash mode: {flasher_args.get('flash_settings', {}).get('flash_mode', 'dio')}")
    print(f"Components:")
    merge(flasher_args, args.build_dir, args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
