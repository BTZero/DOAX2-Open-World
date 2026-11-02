# Bounded pictorial camera orbit trial

Probe v16 adds **Launch Pictorial Orbit Trial.cmd**, selecting the separate,
default-off, restart-required `doax2_open_world_pictorial_orbit_trial` flag.
The five-second and fifteen-second actor reference modes remain unchanged.
This trial incorporates the same fifteen-second coordinated hold, scene-58
admission and native exit path verified in [PICTORIAL_HOLD_TRIAL.md](PICTORIAL_HOLD_TRIAL.md).

## Camera boundary and implementation

Generated `sub_8241A630` evaluates cinematic camera tracks and modifiers,
copies the source pose to object +192, then invokes `sub_8241AF08` after its
later camera modifiers. AF08 calls `sub_8241D278` on that evaluated block
with mode 1 to derive angles from its eye/target pair. It feeds the vectors
at +192 and +208 to `sub_824199B0`, stores the resulting view matrix at +512,
builds projection at +576 and combined output through `sub_8241B080`. It
also builds the block at +752. These are source-derived contracts; runtime
visual acceptance of the new override remains outstanding.

The new scope temporarily supplies eye/target XYZ in the evaluated pose
immediately around the original AF08 call, then restores all sixteen words
of that input block, including its derived angles. The original runs once;
its output matrices remain. Source pose, track descriptors, native projection
parameters, actor transforms, resource management and native exit calls are
preserved. Other modes perform no orbit override.

On first eligible call during the coordinated hold, the orbit captures the
native eye, target and radius. It retains that target and radius rather than
guessing a character target offset. Right-stick yaw is limited to +/-0.6
radians from the initial shot and pitch to +/-0.25 radians, with an absolute
pitch limit of +/-1.2. This is a small orbit of the native shot target, not
yet an actor-follow camera or a full orbit with collision avoidance.

Player 1 input comes through `Runtime::input_system()` / `InputSystem::GetState(0)`;
that implementation uses assigned devices, controller remaps and merged
synthetic keyboard input. Existing I/J/K/L right-stick bindings apply. A
radial 0.2 deadzone and normalized magnitude limit feed 0.8 radians/second
angular speed. Updates integrate host steady time once per observed game
update with a 0.1-second hitch cap. Missing input, loss of application focus,
or B/Start/Back yields zero angular input. Ownership/phase loss or deadline
expiry immediately returns to native camera evaluation; the orbit cannot
restart after release. The underlying finite hold remains once per process.

## Validation and manual checks

The probe compiled and linked. Host tests cover initial pose reconstruction,
fixed radius, deadzone, cadence, angular limits, hitch capping and invalid
input/pose rejection. The finite-hold regression tests passed. The research
launcher parsed and diff whitespace checks passed.

Runtime checks require a fresh process each:

1. Select the same Kasumi/Niki scene 58, press Y/R, and use the right stick
   (or J/L for yaw, I/K for pitch) during the opening hold. Confirm visible
   response, stable framing when input stops, and no cinematic override
   fighting the controls. Let the hold expire and playback finish. Assess
   the return to native camera and ordinary scene cleanup.
2. Repeat in a fresh process, rotate during the hold, and press B/Backspace
   before it expires. Confirm prompt fade and return to the selector.

Leave the game open after each check for trace and isolated save comparison.
Camera collision, full-circle orbit, indefinite lifetime, locomotion and
runtime acceptance are not established by this implementation or host tests.

## First visual run: orbit did not activate

Run `pictorialorbittrial-20261102-202419-095.log`, PID 3856, enabled the new
flag and completed its fifteen-second hold and ordinary cleanup. The user
observed right-stick up/left/right response, no down response, and return
toward the character on releasing input. Both isolated saves matched the
pre-run baseline. No `OWPROBE orbit` admission or update lines were emitted.

Inspection found a hook wiring error: it passed the native60 observation
helper's selected object, which is zero with native60 observation/correction
flags disabled. The orbit never activated, so the observed response cannot
validate this implementation or diagnose its pitch limits. The hook now
passes its actual incoming r3 camera object to the orbit's own ownership
guards. The corrected probe compiled and linked after the previous process
closed. A fresh visual run is required; no successful orbit admission has
yet been observed.

## Second visual run: selected-record admission mismatch

Run `pictorialorbittrial-20261102-202750-174.log`, PID 16636, enabled the
corrected orbit trial. The user reported response in all right-stick
directions but no opening hold. The trace confirms that neither the hold
nor the orbit admitted: playback selected record `0x83DB77FC`, scene ID 58,
while this experiment requires the previously tested record `0x83DA61BC`.
The same scene ID and activation/motion/camera commands do not establish
record equivalence. The cause of the different record selection is not yet
confirmed; no admission guard has been widened on this evidence.

Playback phase 7 began at update 1054 / frame 2 and proceeded without a
hold. Ending began at update 2028 / frame 976, cleanup reached update 2043
/ frame 990 with actor mask 0, and phase 10 followed at update 2044. Both
isolated save hashes matched the pre-orbit baseline. This run does not
validate the new orbit despite the observed native right-stick response.

## Replay through reference access: hold passed, orbit released immediately

The user clarified that a different access option selected the other record.
Replaying through the original access method in the same process selected
`0x83DA61BC`. The user observed the hold, all camera directions working, and
slower apparent downward movement from the low shot. They subsequently
reported that the camera snapped back toward Kasumi on stick release.

The hold began at update 3528 and released at 3979 after 15.033 seconds.
Orbit admission logged at update 3528, but orbit release also logged in that
same update, so sustained custom camera ownership was not achieved. Native
ending and cleanup followed at updates 4924 and 4939, with actor mask 0 at
cleanup. Save hashes still matched baseline.

The scope previously treated any noneligible AF08 call as cancellation once
the orbit had begun, including callbacks for other camera objects. It now
ignores objects other than the admitted camera before considering release.
Release logging additionally records hold/deadline, phase, camera index and
mode to distinguish other ownership failures. A fresh runtime check must
show continuing orbit updates and no recentering on stick release during
the active hold; native camera return after the deadline is still expected.

## Successful bounded orbit and normal completion

Run `pictorialorbittrial-20261102-203144-729.log`, PID 7052, used the reference
access method and record `0x83DA61BC`. The user reported that the camera did
not snap back on stick release during the hold and that orbit worked properly.

The hold and orbit admitted at update 773. Orbit samples continued throughout
the hold, reflecting both horizontal and vertical stick input, with yaw in
the configured range approximately 0.251..1.451 and pitch -0.362..0.138.
The orbit released at update 1223 because its host deadline expired, while
phase 7, camera index 0 and mode 3 remained selected. The coordinated hold
released at update 1224 after 15.034 seconds. This was deadline release,
not premature camera ownership loss.

Ending began at update 2169 / frame 976. Cleanup reached update 2184 / frame
990 with actor mask 0, followed by phase 10 at update 2185. Both isolated
save payload hashes matched the recorded pre-orbit baseline.

This supports the bounded camera orbit and normal-completion path for the
reference entry, including retention of the chosen angle on stick release
during the hold. The next check is B/Backspace exit after rotating while
the orbit is active, in a fresh process. The earlier actor-only exit test
does not independently validate cleanup with this camera override active.

## Successful exit during active orbit

Fresh run `pictorialorbittrial-20261102-203414-843.log`, PID 14520, admitted
the coordinated hold and orbit at update 579. Samples show horizontal and
vertical input changing the orbit, including reaching yaw 1.451, its upper
configured limit. The user reported normal exit back to the menu and noted
that the camera could not rotate around to the character's front. The trial
deliberately restricts yaw to +/-0.6 radians (about 34 degrees) from the
initial shot; this result does not test unrestricted orbit.

The requested exit released the hold at update 715 after 4.533 host seconds,
with eligibility false and script PC cleared. Orbit release followed in
that update with hold inactive, deadline not expired and phase 8. Ending
was observed at update 716 / frame 32. Cleanup reached phase 9 at update
731 / frame 46 with actor mask 0, phase 10 at 732, then selector phase 1
at 747, approximately 1.04 seconds after release. Both isolated save payload
hashes still matched baseline.

The two bounded-orbit checks now pass for the reference entry: retention
of the chosen angle during the hold with normal completion afterward, and
native exit while the orbit is active. Full-circle camera movement, camera
obstruction handling, actor-follow targeting and indefinite lifetime remain
outside this validated milestone.
