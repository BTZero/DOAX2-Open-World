# Pictorial movement and exploration trial

## Five-minute grounded exploration

`Launch Pictorial Movement Trial.cmd` now enables default-off, restart-required
`doax2_open_world_pictorial_exploration` alongside movement. It holds the
coordinated script/camera session for 300 seconds and expands the radial limit
from 1.25 to 20 guest units. Other launch modes disable exploration. For a clean
animation-bank admission, launch a fresh process and play scene 58 directly:
Virtual Pictorials, Kasumi, Edit Playlist, move right ten thumbnails from the
first Kasumi clip to the daylight beach thumbnail, then Y/R to view it.
Playing another clip first can populate all spare banks; the trial then declines.

Controls: left stick or WASD moves relative to the camera; right stick or IJKL
rotates the camera through a full turn, with bounded pitch; LT/RT or Q/E zooms
in/out between 1.5 and 6 guest units. The camera follows actor height and targets
0.85 units above the owned root. B/Backspace retains the native early-exit path.
Focus loss/disconnect neutralizes control input; the wall-clock deadline continues.

Exploration copies and validates the current stage's terrain at admission and
checks its buffer identities/counts during movement. Candidate steps require
support under a 0.15-unit footprint, the spawn's raw triangle flags, slope at
most 0.6, at most 0.15 units of height change, and height no lower than 0.5 units
below spawn. These are conservative experiment limits, not decoded material or
water classifications. Accepted steps also query native `825BEE88`, bypassing
activity height overrides, and require agreement within 0.12 units of projected
terrain. Unsupported steps stay at the last owned pose so input can retreat.

Foot/torso segment tests reject intersections in the terrain mesh. Camera rays
retract before terrain intersections. Decorative props and other collision
systems may be absent from that mesh; complete obstacle collision and camera
clearance are not established. This is a larger bounded exploration experiment,
not unrestricted access to every visible surface.

The native root routine can subtract a loop-origin offset even after root
samples are zeroed. A scoped post-call commit now reapplies the owned pose
before rig evaluation to prevent that secondary displacement. Original native
root services still run; scene/bank ownership gates protect the commit.

Build/link and focused finite-hold, orbit, bounded-movement and terrain-guard
host checks passed. The host checks cover the five-minute deadline, full orbit,
zoom clamps, expanded radial boundary, footprint edges, slopes, flags, height
discontinuities and two-sided triangle obstruction. Runtime findings are recorded
below as available; these checks alone do not establish visual foot contact,
complete collision, cleanup or a five-minute runtime soak.

### Runtime validation and resume checkpoint

Capture `pictorialmovementtrial-20261102-235955-043.log` admitted scene 58 at
update 1832 with 943 vertices, 1729 triangles and spawn flags 1. The hold released
at update 10816 after **300.033 seconds**, with eligibility retained. Original
pose and native motion were restored at that update. The spare bank released
at 11761, and the original selector subsequently returned to phase 1.
`tools/check_exploration_trial.py` passed: **287 accepted native ground samples,
maximum observed radius 7.4556, 12 blocked samples**, stable consecutive neutral
poses, timed release, restoration, bank release and selector return. Full yaw,
pitch and keyboard zoom were exercised in the live session; captures showed
the actor standing on the beach and the camera following the larger route.
The full 20-unit boundary was host-tested but not reached in this runtime run.
The normal diagnostics-disabled `out/game` target also compiled and linked.

Resume here: run a fresh Movement Trial process, enter the reference thumbnail
directly, then inspect movement around the rock/material boundaries, foot
contact during jog turns, camera obstruction and early exit. Geometry guards
can intentionally stop on flags differing from spawn; decode those flags and
the remaining native obstacle system before relaxing them. Disconnect/reconnect,
focus loss, save-hash comparison, repeated entry/exit and broader performance
comparison remain outstanding. A final small camera guard ensures eye and
target never coincide when a ray hit lies very close to the target; the final
probe was rebuilt after the soak. No generated code, game assets, saves, logs
or captures are included in the commit. Existing SDK working-tree changes
remain separate from this exploration checkpoint.

## Earlier checkpoint: bounded displacement and two-minute trial

The movement launcher now enables camera-relative planar displacement, analog
speed, bounded facing changes and a 1.25 guest-unit radial limit. The user
confirmed being able to run around a small area, estimated visually as 10x10;
that estimate is not a measurement of guest coordinate scale. Capture
`pictorialmovementtrial-20261102-222619-684.log` reached radius 1.2500 and
recorded pose restoration at update 937 and native animation restoration at
938. The process closed before spare-bank release was observed in that capture.

At the user's request, the movement trial now holds for **120 seconds** instead
of 15. The other trial durations retain their previous values. The shared
finite-hold policy accepts at most 120 seconds, retains its ownership-loss
release, and releases at the deadline without restarting. The script, camera
and movement use the same deadline. Host checks cover holding past 15 seconds,
the two-minute deadline and early ownership loss; runtime validation of the
longer session remains pending.

Movement still writes the original fixed Y coordinate. Native rig samples
already show changing terrain heights inside this patch. Ground following and
obstacle/edge rejection must be established before expanding the travel radius.
The stationary capture checker below applies only to earlier binding captures.

The remainder of this document records the earlier animation-binding stages.

Work resumed at the user's request after the idle checkpoint. The separate
`Launch Pictorial Movement Trial.cmd` enables default-off, restart-required
`doax2_open_world_pictorial_movement_trial`. Other launch modes explicitly
disable it. The idle reference remains available separately.

This first stage holds position while testing left-stick idle/jog
transitions. Displacement is pending native jogging validation, as required
by the recorded motion-research checkpoint. Directional travel, facing
changes and grounded movement are not implemented yet.

The trial reuses scene-58 admission, native spare Kasumi bank, fifteen-second
coordinated hold, bounded orbit, restoration and release. It begins with
idle 270. Left-stick magnitude above 0.25 requests shared candidate 8223
(volleyball high-level 18); release below 0.18 returns to 270. Hysteresis
prevents noise from repeatedly requesting animations. Direction is logged
but does not select directional clips. No distinct half-stick walk is claimed.

Input uses the runtime's merged controller path, once per guest update.
Disconnect, focus loss and B/Start/Back produce neutral intent. The previous
native motion request must clear before another is submitted. Jog admission
checks shared command index 31, selection prefix and 24-frame transform
header. A mismatch requests original-animation restoration and logs rejection.
These checks are runtime gates, not evidence of successful binding.

Both replacements use the native binder and existing native loop policy.
Slot 0's root delta is logged and consumed as zero by the original root
routine. No world transform, terrain query or asset is replaced. Hold,
orbit, root suppression and restoration recognize the currently requested
candidate. External ownership changes retain native transitions. Bank
release waits for restoration and native unbinding.

## Validation and next capture

Probe v20 compiled and linked. Existing finite-hold and orbit host checks
passed; the research launcher parsed and diff whitespace checks passed.
Run `pictorialmovementtrial-20261102-215029-138.log`, PID 12800, launched
into Virtual Pictorials. Native jogging admission and cleanup are pending;
startup alone does not validate those behaviors.

1. Select the usual Kasumi/Niki scene 58 and press Y/R for the trial hold.
2. Stay neutral briefly, hold left stick or W for several seconds, release,
   and repeat within the fifteen-second window. Observe idle/jog/idle.
3. Let the hold expire and playback finish normally. Leave the game open
   for inspection of requests, native cycles, root samples and bank release.
4. In a fresh process, repeat with B/Backspace during the jog to validate
   early exit and cleanup. Also inspect focus loss and controller disconnect.

Motion snapshots now include shared indices 24..46 for mismatch inspection.
Before displacement, correlate requested/native motion, phase progression,
cycle starts, raw root deltas and stationary position; confirm restoration,
bank release and isolated save hashes. Visual acceptance is needed for
transitions and loop quality. Beach Flags remains deferred.

## Resumed checkpoint

The completed capture above now provides 31 stationary native samples and
eight jogging cycle starts, including jog-to-idle return. Original motion 27
was restored at update 4291; the spare bank was released at update 5236.
The user confirmed that animation visual quality is perfect. This supersedes
the pending native-binding and visual-quality status above. Early exit,
focus loss, disconnect and save-integrity checks remain pending.

Run `python tools/check_movement_trial.py <capture.log>` to check the
stationary binding evidence, native motion agreement, return to idle,
restoration and subsequent bank release. It cannot assess visual quality.
The Probe build and finite-hold/orbit host checks passed again at this
checkpoint. Bounded directional displacement is the next implementation
stage and remains unimplemented in this checkpoint.
