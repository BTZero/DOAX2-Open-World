"""Check a stationary pictorial movement capture; this does not assess visuals."""
import argparse
import re
from pathlib import Path


def check_capture(text):
    samples = []
    cycles = []
    restored = False
    released = False
    returned_to_idle = False
    saw_jog = False
    for line in text.splitlines():
        if "movement binding: rejected" in line or "idle actor: ownership changed" in line:
            raise ValueError("Resource rejection or external ownership change in capture")
        match = re.search(r"movement binding: sample update=(\d+) requested=(\d+) native=(\d+) phase=([\d.]+).*xyz=\(([^)]+)\)", line)
        if match:
            _, requested, native, phase, xyz = match.groups()
            if requested != native:
                raise ValueError("Requested and native motion differ in a sample")
            position = tuple(float(v) for v in xyz.split(","))
            samples.append(position)
            saw_jog |= native == "8223"
            returned_to_idle |= saw_jog and native == "270"
        if "idle actor: native cycle" in line and "motion=8223 " in line:
            cycles.append(line)
        if "idle actor: restored" in line:
            restored = True
        if "idle bank: released" in line and restored:
            released = True
    if not samples or not returned_to_idle or len(cycles) < 2:
        raise ValueError("Need native jog samples, repeated jog cycles and return to idle")
    if any(abs(a - b) > 0.001 for position in samples for a, b in zip(position, samples[0])):
        raise ValueError("Position changed during the stationary binding trial")
    if not restored or not released:
        raise ValueError("Need native restoration followed by spare-bank release")
    return len(samples), len(cycles)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        samples, cycles = check_capture(args.log.read_text(encoding="utf-8", errors="replace"))
    except ValueError as error:
        parser.exit(1, f"FAIL: {error}\n")
    print(f"PASS: {samples} stationary native samples, {cycles} jog cycle starts; idle return, restoration and bank release observed.")
    print("Visual quality, early exit, focus loss, disconnect and save integrity require separate checks.")
