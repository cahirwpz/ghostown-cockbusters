#!/usr/bin/python3

import math
import sys
import os.path
from contextlib import redirect_stdout


def gen_double_helix(radius=1.0, pitch=0.5, turns=2, points_per_turn=100):
    points = int(points_per_turn * turns)

    start = -math.pi * turns
    end = math.pi * turns
    vertices = []

    for i in range(points):
        theta = (end - start) * i / points + start

        x = radius * math.cos(theta)
        y = pitch * theta / (2.0 * math.pi)
        z = radius * math.sin(theta)
        vertices.append((x, y, z))

        x = radius * math.cos(theta + math.pi)
        y = pitch * theta / (2.0 * math.pi)
        z = radius * math.sin(theta + math.pi)
        vertices.append((x, y, z))

    return vertices


def print_obj(name, vertices):
    print("# Double Helix OBJ file")
    print(f"mtllib {name}.mtl")
    print(f"o {name}")
    for x, y, z in vertices:
        x = round(x, 3)
        y = round(y, 3)
        z = round(z, 3)
        print(f"v {x} {y} {z}")
    print("usemtl default")
    print("g lines")
    for i in range(len(vertices) // 2):
        p0 = i * 2 + 1
        p1 = i * 2 + 2
        print(f"l {p0} {p1}")


def print_mtl(name):
    print("# Double Helix MTL file")
    print(f"newmtl default")


if __name__ == "__main__":
    path = sys.argv[1]
    name = os.path.basename(path)

    dna = gen_double_helix(
            radius=1.5, pitch=10.0, turns=1.0, points_per_turn=16)

    with redirect_stdout(open(f"{path}.obj", "w")):
        print_obj(name, dna)

    with redirect_stdout(open(f"{path}.mtl", "w")):
        print_mtl(name)
