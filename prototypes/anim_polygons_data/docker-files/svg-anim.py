#!/usr/bin/env python3

import os
import sys
import argparse
import xml.etree.ElementTree as ET


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
    root = ET.XML("<svg></svg>")
    root.attrib["xmlns"] = XMLNS["svg"]
    root.attrib["version"] = "1.1"
    root.attrib["width"] = f"{width}"
    root.attrib["height"] = f"{height}"
    return root


def append_path(to, verts, path_id, fill):
    path = ET.SubElement(to, "path")
    d = f"M{verts[0][0]},{verts[0][1]} "
    for vert in verts[1:]:
        d += f"L{vert[0]},{vert[1]} "
    path.attrib["d"] = d + "Z"
    path.attrib["id"] = path_id
    path.attrib["fill"] = fill


def append_frame(to, frame_svg, total_frames, frame_number, remove_color):
    frame_root = ET.parse(frame_svg).getroot()
    frame = append_group(to, f"frame_{frame_number}")
    polys = append_group(frame, f"polygons_frame_{frame_number}")
    append_animate_element(frame, f"anim_frame_{frame_number}",
                           total_frames, frame_number)

    count = 0
    verts = []
    for stmt in frame_root:
        color = stmt.attrib.get('fill', '#000000')
        if color == remove_color:
            continue
        translate = stmt.attrib.get('transform', 'translate(0,0)')
        translate = eval(translate[9:])
        for code in stmt.attrib["d"].split():
            if code[0] == "Z":
                if len(verts) >= 3:
                    append_path(polys, verts,
                                f"path_{frame_number}_{count}", color)
                    count += 1
                verts = []
            else:
                x, y = map(int, code[1:].split(","))
                x += translate[0]
                y += translate[1]
                verts.append((x, y))


def get_dimensions(svg_file):
    svg_root = ET.parse(svg_file).getroot()
    width = int(svg_root.attrib["width"])
    height = int(svg_root.attrib["height"])
    return width, height


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Process rendered animation into a single "
                    ".svg file for anim-polygons effect.")
    parser.add_argument(
        "anim_dir",
        help="Directory containing blender animation output.")
    parser.add_argument(
        "-r", "--remove-color",
        help="Remove areas with color in #RRGGBB format",
        type=str)
    parser.add_argument(
        "-o", "--output",
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
        append_frame(animation, frame_svg, total_frames, frame_number,
                     args.remove_color)

    ET.ElementTree(svg_root).write(args.output)
