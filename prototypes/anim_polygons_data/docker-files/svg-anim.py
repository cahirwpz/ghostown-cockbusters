#!/usr/bin/env python3

import os
import sys
import argparse
import xml.etree.ElementTree as ET

BORDER = 10
FPS = 25
XMLNS = {"svg": "http://www.w3.org/2000/svg"}


def append_group(to, group_id):
    group = ET.SubElement(to, "g")
    group.attrib["id"] = group_id
    return group


def append_animate_element(to, anim_id, total_frames, frame_number):
    frame_time = 1/FPS
    anim_duration = frame_time * total_frames
    anim = ET.SubElement(to, "animate")
    anim.attrib["id"] = anim_id
    anim.attrib["begin"] = f"{-(anim_duration - frame_number * frame_time)}"
    anim.attrib["attributeName"] = "display"
    anim.attrib["values"] = "inline;none;none"
    anim.attrib["keyTimes"] = f"0;{1/total_frames};1"
    anim.attrib["repeatCount"] = "indefinite"
    anim.attrib["dur"] = f"{anim_duration}"


def create_svg_root(width, height):
    svg_root = ET.XML("<svg></svg>")
    svg_root.attrib["xmlns"] = XMLNS["svg"]
    svg_root.attrib["version"] = "1.1"
    svg_root.attrib["width"] = f"{width}"
    svg_root.attrib["height"] = f"{height}"
    return svg_root


def append_path(to, verts, path_id, fill):
    path = ET.SubElement(to, "path")
    d = f"M{verts[0][0]},{verts[0][1]} "
    for vert in verts[1:]:
        d += f"L{vert[0]},{vert[1]} "
    path.attrib["d"] = d + "Z"
    path.attrib["id"] = path_id
    path.attrib["fill"] = fill


def get_coords(code):
    xy = code.split(",")
    x = int(xy[0]) - BORDER
    y = int(xy[1]) - BORDER
    return (x, y)


def append_frame(to, frame_svg, total_frames, frame_number):
    frame_root = ET.parse(frame_svg).getroot()
    frame = append_group(to, f"frame_{frame_number}")
    polys = append_group(frame, f"polygons_frame_{frame_number}")
    append_animate_element(frame, f"anim_frame_{frame_number}",
                           total_frames, frame_number)

    count = -1
    verts = []
    for code in frame_root[0].attrib["d"].split():
        if code[0] == "Z":
            if len(verts) >= 3:
                count += 1
                if count > 0:
                    append_path(polys, verts,
                                f"path_{frame_number}_{count}", "#FF0000")
            verts = []
        else:
            verts.append(get_coords(code[1:]))


def get_dimensions(svg_file):
    svg_root = ET.parse(svg_file).getroot()
    width = int(svg_root.attrib["width"]) - 2 * BORDER
    height = int(svg_root.attrib["height"]) - 2 * BORDER
    return width, height


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Process rendered animation into a single "
                    ".svg file for anim-polygons effect.")
    parser.add_argument(
        "anim_dir",
        help="Directory containing blender animation output.")
    parser.add_argument(
        "-o", "--output",
        metavar="OUT",
        help="Output svg file name.",
        default="dancing.svg")
    args = parser.parse_args()

    if not os.path.isdir(args.anim_dir):
        sys.exit("anim_dir must be a valid directory!")

    ET.register_namespace("", XMLNS["svg"])

    frames_files = sorted(
            [os.path.join(args.anim_dir, name)
             for name in os.listdir(args.anim_dir)
             if name.endswith(".svg")])
    total_frames = len(frames_files)
    width, height = get_dimensions(frames_files[0])
    svg_root = create_svg_root(width, height)
    animation = append_group(svg_root, "Animation")

    for frame_number, frame_svg in enumerate(frames_files):
        append_frame(animation, frame_svg, total_frames, frame_number)

    ET.ElementTree(svg_root).write(args.output)
