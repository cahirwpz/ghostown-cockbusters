#!/usr/bin/env python3 -B

from uvmap import NonUniformUVMap, Ball, Lens, Copy
import sys

if __name__ == "__main__":
    uvmap = NonUniformUVMap(64, 64, utexsize=64, vtexsize=32)
    lens = Lens(0.2, 0.3)
    uvmap.generate(lens, (-1, 1, -1, 1))
    # uvmap.generate(Copy, (0, 1, 0, 1))
    uvmap.save(sys.argv[1], uscale=64, vscale=32)
