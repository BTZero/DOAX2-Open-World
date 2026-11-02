# Native standing, idle and locomotion research

The user identified standing, delayed idle, walking/jogging during volleyball
and a faster animation in Beach Flags as native sources to investigate.
No semantic animation ID is accepted solely from that observation or a table.

## Source-derived contracts

- Volleyball chooser `8268DAE8` accepts a player-record pointer in r3,
  high-level motion ID in low r4 and rate in f1. ID 65535 does nothing.
  It indexes 32-byte entries at `82B11F68`, writes player +64 (high ID),
  +62 (category byte), +52 (native motion), +44 (initial phase), +48 (rate),
  control flags and a position/facing anchor. It can call `82682590` for
  ball/action scheduling; this is not a portable, animation-only setter.
- Table +0 is category, +2 repeats the high-level ID, +4 is native motion,
  and +6 is a successor consumed by `8268DA38`. The latter applies the
  successor when the queued ID at player +66 is 65535 or zero, then clears
  movement-related fields. `8268D9D8` applies a queued +66 ID and resets it.
  These are native transition paths, not proof that a chosen clip loops.
- `8268D840` handles queued facing transitions and special high IDs
  817..837, including per-update facing adjustment. ID 817 maps to native
  8323 in an existing captured image. Its visual meaning remains unverified.
- `8250FC50` reads native player +52/+54 and initializes an animation track;
  `8250FE28` updates tracks at `834271A0 + (slot*2 + track)*144`.
  Track +52 is phase and +112 is a halfword end boundary. The new observer
  reads both tracks for each of the four slots, without changing them.
- `82510218(slot)` rotates the delta at `83426FD0 + slot*116` by facing
  and applies native displacement under its flags/conditions. Existing
  Beach Flags timing-hook comments describe zero/double root steps around
  run transitions, including at original cadence. New tracing records the
  input delta and actual position difference; it does not add movement.

Read-only inspection of existing image `image-23296-20261102-150404-658.bin`
found high IDs 0 -> native 8192, 1 -> 8193, 2 -> 8213 and 817 -> 8323.
These are mapping examples, not standing/idle/walk labels. Pictorial motion
27 cannot be replaced with one of them until resource/bank compatibility
and native binding requirements are established.

The static caller audit is recorded in ignored
`out/open-world-research/motion-request-callers.json`. It identifies fifteen
callers, including `8268C198`, `8268C320`, `8268CAB0`, `8268D258`,
`8268D840`, `8268D9D8`, `8268DA38`, `8268E290` and `82691FE0`. Live
call-site correlation is required to assign semantic transition names.

## Probe v17 and capture

**Launch Motion Research.cmd** selects `MotionResearch` and the separate,
default-off restart-required `doax2_open_world_motion_research`. Other modes
explicitly disable it. Research uses the existing isolated userdata and
original timing configuration; all hold/orbit/terrain trial flags are off.

The diagnostics build integrates a scope in the existing chooser wrapper
and a scope in the existing root-motion wrapper. Each retains its original
native call exactly once, including early returns. Rig observations occur
at the existing native rig boundary. These observers never write guest
memory, replace animation resources or select motions. The flag, actual
volleyball/Beach Flags update activity, mapped reads, a 1600-event motion
allowance and existing global trace cap gate the new observations. Periodic
rig/root samples use a thirty-update interval; rig motion-pair changes and
accepted chooser calls also log. This is not a complete per-frame capture.

The probe compiled and linked, the launcher parsed, and diff whitespace
checks passed. Startup confirmed `motion_research=true` with the other
experimental flags false. No native activity capture has been accepted yet.
Baseline isolated save hashes are in ignored `motion-save-hashes-before.json`;
ordinary match progression may legitimately update these research saves.

First capture: enter original Kasumi volleyball, leave the stick neutral
about eight seconds while in control, then move gently, move at full stick,
and release for another idle interval. Note which visible standing/idle or
movement behavior occurred. Finish or exit normally and leave the game
open for inspection. If the original game does not select a separate walk
at partial stick, record that rather than assigning one from metadata.

Inspect that capture before a separate fresh-process Beach Flags capture
of the native start/run/finish sequence. Correlate visual observations with
motion IDs, track phase/end, resource identity and root displacement before
preparing a stationary bounded Niki idle trial. Resource compatibility,
looping behavior, blending and displacement ownership remain discovery gates.

## Volleyball capture: starts, repeated movement and neutral variants

Run `motionresearch-20261102-204246-051.log`, PID 13728, recorded 1464 new
motion observations, below the 1600-event allowance. Volleyball activity
was present at updates 7170..9620; ordinary completion cleared its activity
mask at 9621. The user stopped, started, idled and played rallies to expose
additional animations, and reported identical movement at half and full
stick. No separate analog walk/jog threshold is established by this capture.
Stick magnitude was not recorded, so the trace does not independently label
the two input intervals or prove that a separate walk clip never exists.

Slot 0's player byte +1 was 5. Its observed movement requests included:

| High-level family | Native IDs in table | Observed transition evidence |
|---|---|---|
| 10..17 | 8222, 8225, 8228, 8231, 8234, 8216, 8237, 8219 | Ten-frame start clips, successors 18..25; slot 0 observed all except 12, which appeared on other slots |
| 18..25 | 8223, 8226, 8229, 8232, 8235, 8217, 8238, 8220 | Twenty-four-frame repeated movement clips, each with itself as successor; all observed on slot 0 |
| 26..57 (observed subset) | Includes 10224, 10218, 10239, 10380, 10381 | Short transitions around changes/stops; exact directional and stop semantics still require mapping |

For example, slot 0 selected high 11 / native 8225 at update 8033, then
the native successor handler selected high 19 / native 8226 at 8043. High
10 / native 8222 at update 8207 likewise became high 18 / native 8223 at
8217. The chooser requests from queued dispatcher `8268BEE8` return through
`8268BF84`; successor calls return through `8268DA9C`. Observed locomotion
request rates were 1; this is not a measured general free-roam speed contract.

High ID 0 is not a sufficient portable idle binding. Table native 8192
was followed by actual rig motions including 63, 270..275 and 543..547
while high ID stayed 0. At update 7837, native 543 began with track phase
481 / end 860 and position (-1,0,-3.5). At 7980 it was phase 624 at
(-1.1117,0,-4.5707), then another action interrupted it at 7988. Sampled
native root deltas during this interval matched actual displacement, e.g.
(-0.01104,0,-0.00172) at update 7860. It is not a demonstrated stationary
idle, and its segment bounds require investigation before reuse. Other
neutral clips had short independent track starts; no generic loop has been
accepted yet.

`82515F30` provides a concrete resource-bank lead: motion >=8192 subtracts
8192 and selects a shared bank; lower IDs select a bank using player +4.
Thus matching a clip number alone does not establish the same resource in
the pictorial shell. Some action requests also resolved to different native
rig IDs than their table entry, reinforcing the need to trace binding.

Normal match progression advanced calendar 2 -> 3 at update 9630 through
the previously observed `826CF994` caller. Earlier vacation actions in the
same run advanced 0 -> 1 -> 2. The isolated `rds.dat` hash remained at its
baseline; `ups.dat` changed, consistent with ordinary vacation progression.
This is not the unchanged-save condition of the pictorial-only trials.
No original user-data root was used. Raw observations, the slot-0 timeline
and parsed requests are retained under ignored `out/open-world-research/`.

Next is a fresh-process Beach Flags capture of its start/run/finish path,
then resource binding and a candidate stationary idle test. No native
animation replacement or locomotion controller is implemented by this audit.

## Beach Flags deferred by user

The user reported that Beach Flags requires unlocking and explicitly asked
to skip it for now. No unlock or progression override is authorized by that
report, and none was introduced. The fresh MotionResearch process prepared
for that capture is not evidence of a Beach Flags run.

Continue with the existing volleyball capture: map the neutral-motion
resolver and the shared versus character-specific bank selection, then
establish a stationary idle candidate compatible with the pictorial shell.
Beach Flags sprint research is deferred and is not a prerequisite for that
idle work. Do not claim a portable sprint or a distinct half-stick walk.

The stationary idle candidate now runs in the bounded pictorial trial.
See [PICTORIAL_IDLE_TRIAL.md](PICTORIAL_IDLE_TRIAL.md) for native bank loading,
four observed cycle starts, original-motion restoration, bank cleanup and
the user's confirmation of visible idle admission. Beach Flags remains deferred.

## Next step — paused at user request

The user asked to pause work here. Resume only when requested. The next
planned experiment is a short, controlled movement trial in the verified
pictorial shell: retain the bounded hold and orbit, use left-stick input
to move Kasumi within a small bounded area, and switch between the verified
idle and a volleyball jogging candidate when input starts or stops. Restore
the original scene on timeout or B. Validate the jogging candidate's resource
binding and native movement behavior before enabling displacement.

A distinct half-stick walking animation has not been established. Beach
Flags remains deferred; no unlock work is planned. The completed hold,
orbit and native idle checkpoint was pushed as commit `59e7fb2` on
`feature/open-world`. This next movement trial had not been implemented at
the paused checkpoint.

Work has resumed at the user's request. Probe v20 adds a separate stationary
left-stick idle/jog binding stage before displacement. See
[PICTORIAL_MOVEMENT_TRIAL.md](PICTORIAL_MOVEMENT_TRIAL.md) for implementation,
validation limits and the next runtime capture.
