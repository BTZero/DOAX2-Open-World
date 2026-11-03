"""Audit one completed five-minute exploration capture; visuals need review."""
import argparse
import math
from pathlib import Path
import re


def check_capture(text):
    if any(message in text for message in (
        "exploration: rejected", "exploration: terrain ownership changed",
        "movement: rejected", "idle actor: ownership changed")):
        raise ValueError("Exploration rejection or ownership loss in capture")
    if "radius=20 duration=300" not in text:
        raise ValueError("No five-minute expanded terrain admission")
    ground = re.findall(
        r"exploration: ground update=\d+ blocked=(true|false) native_y=([-\d.]+) projected_y=([-\d.]+)", text)
    accepted = [(float(a), float(b)) for blocked, a, b in ground if blocked == "false"]
    if len(accepted) < 30 or any(not math.isfinite(a+b) or abs(a-b) > .1201 for a, b in accepted):
        raise ValueError("Need repeated native/projected ground agreement")
    steps = re.findall(
        r"movement: step update=\d+ traveling=(true|false) xyz=\(([^)]+)\).*?radius=([-\d.]+)", text)
    radii = [float(radius) for _, _, radius in steps]
    if not radii or max(radii) <= 1.25 or max(radii) > 20.001:
        raise ValueError("Need travel beyond the old radius and within the new radius")
    previous = None
    for traveling, xyz, _ in steps:
        position = tuple(float(v) for v in xyz.split(","))
        if traveling == "false" and previous is not None:
            if any(abs(a-b) > .001 for a, b in zip(position, previous)):
                raise ValueError("Owned pose drifted across consecutive neutral samples")
        previous = position if traveling == "false" else None
    releases = re.findall(r"pictorial hold: release .*?elapsed=([\d.]+)s eligible=(true|false).*?duration=300.0s", text)
    if not releases or not any(300 <= float(seconds) < 302 and eligible == "true" for seconds, eligible in releases):
        raise ValueError("Need release at the five-minute deadline with ownership retained")
    pose = text.find("movement: pose restored")
    actor = text.find("idle actor: restored", pose)
    bank = text.find("idle bank: released", actor)
    selector = re.search(r"pictorial: .*?phase=1 ", text[bank:]) if bank >= 0 else None
    if min(pose, actor, bank) < 0 or selector is None:
        raise ValueError("Need pose restoration, native restoration, bank release and selector return")
    cameras = re.findall(r"exploration: camera .*?radius=([\d.]+) fraction=([\d.]+) yaw=([-\d.]+) pitch=([-\d.]+)", text)
    if not cameras or any(not (1.5 <= float(r) <= 6 and 0 <= float(f) <= 1 and .049 <= float(p) <= 1.001)
                          for r, f, _, p in cameras):
        raise ValueError("Camera samples missing or outside limits")
    return len(accepted), max(radii), sum(blocked == "true" for blocked, _, _ in ground)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        count, radius, blocked = check_capture(args.log.read_text(encoding="utf-8", errors="replace"))
    except ValueError as error:
        parser.exit(1, f"FAIL: {error}\n")
    print(f"PASS: {count} native ground samples; max radius {radius:.4f}; {blocked} blocked samples; five-minute release and cleanup observed.")
    print("Visual foot contact, props missing from terrain, camera clearance, disconnect and save integrity require separate checks.")
