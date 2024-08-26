#!/usr/bin/env python3 -B

from uvmap import UVMap, Ball, Lens, Copy
import sys


if __name__ == "__main__":
    uvmap = UVMap(64, 64, texsize=32)
    #uvmap = UVMap(16, 16, texsize=16)
    uvmap.generate(Lens, (-1, 1, -1, 1))
    #uvmap.generate(Ball, (-1.0, 1.0, -1.0, 1.0))
    uvmap.save(sys.argv[1], scale=32)
    #uvmap.save(sys.argv[1], scale=16)
