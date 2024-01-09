#!/usr/bin/env python3 -B

from uvmap import UVMap, Ball, Lens
import sys


if __name__ == "__main__":
    uvmap = UVMap(64, 64)
    uvmap.generate(Lens, (-1.0, 1.0, -1.0, 1.0))
    uvmap.save(sys.argv[1], scale=128)
