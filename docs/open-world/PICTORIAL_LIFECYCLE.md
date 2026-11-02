# Pictorial playback ownership checkpoint

The main-menu pictorial path supplies repeatable scenes without starting a
vacation. It remains an original scripted scene, not an independent actor or
free-roam session. No new movement, timer hold or completion bypass is enabled.

## Source and live-state findings

The title screen table is `0x82B83C80`. Screen 1 is the character selector
(`825A1EC8` / `825A1FB8`); screen 2 is the scene/playlist selector
(`825A2170` / `825A2270`). Screen 2 calls nested controller `826F0540` with
state base `0x83DA3D98`. The outer screen descriptor stays active during
pictorial playback, so watching screen IDs alone misses load/play/cleanup.

| Controller field | Address | Evidence |
|---|---|---|
| Phase | `0x83DB9C54` | Base +89788; source branches on phases 0..11; live playback snapshot reads 7 |
| Return phase | `0x83DB9C58` | Base +89792; phase 10 restores it; live snapshot reads 1 |
| Selected scene record | `0x83DB99CC` | Base +89140; native phase 4/5 dereferences this pointer; live snapshot points to `0x83DA6004` |
| Preview flag | `0x83DB9BCC` | Base +89652; native code writes/reads a byte, not a word |
| Script | `0x833B4C68` | Shared script object; live snapshot active byte is 1 and +28 advances during playback |

The live snapshot and capture belong to isolated run
`pictorialresearch-20261102-150035-916.log`, PID 23296. They remain in ignored
`out/open-world-research/`. That run played one Kasumi scene without additional
input and returned automatically to the scene selector. Both research save
payload hashes stayed unchanged. No vacation was entered.

## Source-derived lifecycle

| Phase | Native behavior |
|---|---|
| 4 | Wait for transition; clear actor update flags; prepare selected record through `826F1EC8`; request script resources with `824BF888` |
| 5 | Wait for native resource state, then start the selected script with `824B8AB0` |
| 6 | Wait for positive script progress and enter phase 7; some scene IDs additionally change music policy |
| 7 | Run playback/completion and input policy, including `826E28E0` and per-scene camera handling |
| 8 | Wait for transition, clear script/player state through `824B8B48`, restore original scene services and request native return |
| 9 | Wait for resource state, request transition with `826EDDA0`, enter 10 |
| 10 | Wait for transition; restore saved selector phase and stop scene audio priority |

These rows describe source control flow. Only phase 7 was verified by the
first live memory snapshot; do not infer that every scene uses every branch.
The source establishes reuse of script/resource services by pictorials and
vacation relaxation, but their outer playback controllers are different.

## New read-only trace and next gate

The diagnostics probe now logs `OWPROBE pictorial` on phase changes and every
30 updates while title screen 2 owns the matching descriptor. It records
selected record/scene ID, script clock, actor update mask and camera index.
Pointer-derived scene reads are confined to the known controller region and
pass the existing readable-memory check. This observer changes no guest state.

Next, compare an explicit stop with the completed phase cycle below, correlating
script start, camera ownership, actor update eligibility and resource release. Compare
a verified Niki scene with vacation relaxation before choosing a standalone
shell. Keep the existing vacation-only five-second hold trial disabled: its
admission and outer completion policy do not describe this pictorial path.

## Executed phase-cycle validation

Rebuilt probe run `pictorialresearch-20261102-150802-569.log`, PID 25108,
captured scene ID 68 at record `0x83DA6004`. The scene was allowed to finish
without further input:

| Update | Phase | Script frame | Actor update mask |
|---|---|---|---|
| 5352 | 4, loading begins | 719 from earlier title script | 0 |
| 5367 | 5 | 719 | 0 |
| 5377 | 6, script started | 0 | 0 |
| 5389 | 7, playback | 2 | 1 |
| 6433 | 8, ending | 1046 | 1 |
| 6448 | 9, cleanup | 1060 | 0 |
| 6449 | 10 | 1060 | 0 |
| 6464 | 1, selector restored | 1060 | 0 |

The outer screen stayed title-table screen 2, state 1, throughout the cycle.
Camera selector 0 remained selected. No calendar-advance call was logged during
playback, and the calendar trace stayed at its title-mode value -1. Both
research save hashes still matched the pre-playback hashes from the first run.
The game was closed normally after return. The observer compiled, linked and
executed successfully. Explicit-stop behavior, other scene IDs and Niki-specific
resource ownership remain future checks; this does not establish independent
actor spawning, movement or free-roam acceptance.
