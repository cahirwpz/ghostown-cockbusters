#!/usr/bin/env python3

import argparse
import csv


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="")
    parser.add_argument(
        "-n", "--name", type=str, required=True)
    parser.add_argument(
        "-s", "--scale", type=float, default=1.0,
        help="")
    parser.add_argument(
        "anim_csv",
        help="")
    args = parser.parse_args()

    frames = []
    with open(args.anim_csv, newline='') as csvfile:
        reader = list(csv.reader(csvfile))
        for tx, ty, tz, rx, ry, rz, sx, sy, sz in reader[1:]:
            tx = int(float(tx) * 16 * args.scale)
            ty = int(float(ty) * 16 * args.scale)
            tz = int(float(tz) * 16 * args.scale)
            rx = int(float(rx) * 4096.0 / 360.0) & 0xfff
            ry = int(float(ry) * 4096.0 / 360.0) & 0xfff
            rz = int(float(rz) * 4096.0 / 360.0) & 0xfff
            sx = int(float(sx) * 4096)
            sy = int(float(sy) * 4096)
            sz = int(float(sz) * 4096)
            frames.append([tx, ty, tz, rx, ry, rz, sx, sy, sz])

    print(f"#define {args.name}_frames {len(frames)}")
    print("")
    print(f"static short {args.name}[{args.name}_frames][9] = {{")
    for frame in frames:
        print("  {%s}," % ", ".join(map(str, frame)))
    print("};")
    print("")
