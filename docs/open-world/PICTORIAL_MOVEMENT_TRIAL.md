# Pictorial movement trial: animation binding stage

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
