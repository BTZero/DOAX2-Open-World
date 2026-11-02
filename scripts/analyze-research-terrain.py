"""Validate a local research terrain capture and sample triangle heights.

Outputs measurements only. Triangle projection is an offline geometry check,
not the game's collision algorithm, material policy, or walkability decision.
Captured proprietary geometry must remain in ignored out/.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
import math
from pathlib import Path
import struct


def analyze(metadata: Path, points: list[list[float]]) -> dict:
    data = json.loads(metadata.read_text(encoding="utf-8-sig"))
    if data.get("SchemaVersion") != 2:
        raise ValueError("Require schema 2 capture with verified physical alias mapping")
    if data.get("VertexStride") != 16 or data.get("TriangleStride") != 8:
        raise ValueError("Unsupported record layout")
    stem = metadata.with_suffix("")
    vertex_bytes = stem.with_name(stem.name + "-vertices.bin").read_bytes()
    triangle_bytes = stem.with_name(stem.name + "-triangles.bin").read_bytes()
    if len(vertex_bytes) != data["VertexCount"] * 16:
        raise ValueError("Vertex length mismatch")
    if len(triangle_bytes) != data["TriangleCount"] * 8:
        raise ValueError("Triangle length mismatch")
    vertices = list(struct.iter_unpack(">4f", vertex_bytes))
    triangles = list(struct.iter_unpack(">4H", triangle_bytes))
    if not vertices or not triangles:
        raise ValueError("Empty geometry")
    if not all(math.isfinite(c) for v in vertices for c in v[:3]):
        raise ValueError("Nonfinite vertex")
    if not all(i < len(vertices) for t in triangles for i in t[:3]):
        raise ValueError("Triangle index outside vertex buffer")
    samples = []
    for x, z in points:
        if not math.isfinite(x) or not math.isfinite(z):
            raise ValueError("Nonfinite sample coordinate")
        hits = []
        for triangle_id, (ia, ib, ic, flags) in enumerate(triangles):
            a, b, c = vertices[ia], vertices[ib], vertices[ic]
            denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
            if abs(denominator) < 1e-10:
                continue
            wa = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
            wb = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
            wc = 1 - wa - wb
            if min(wa, wb, wc) >= -1e-7:
                hits.append({"triangle": triangle_id, "flags": flags,
                             "height": wa * a[1] + wb * b[1] + wc * c[1]})
        samples.append({"x": x, "z": z, "hits": hits})
    return {
        "capture": metadata.name,
        "screen": data["ScreenId"], "activity_mode": data["ActivityMode"],
        "vertices": len(vertices), "triangles": len(triangles),
        "bounds_xyz": [[min(v[i] for v in vertices), max(v[i] for v in vertices)] for i in range(3)],
        "triangle_flags": dict(sorted(Counter(t[3] for t in triangles).items())),
        "samples": samples,
        "limitation": "Geometric projection only; no native material, radius, slope or walkability policy applied",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("metadata", type=Path)
    parser.add_argument("--point", nargs=2, type=float, action="append", metavar=("X", "Z"))
    args = parser.parse_args()
    points = args.point or [[-1, -5.4], [0, 0], [0, 10], [0, 20], [10, 0], [20, 20]]
    try:
        report = analyze(args.metadata, points)
    except (ValueError, KeyError, OSError) as error:
        parser.exit(1, f"Invalid terrain capture: {error}\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
