# Five-second pictorial wait experiment

Launch **Launch Pictorial Hold Trial.cmd**, select the same Kasumi/Niki
thumbnail validated as scene 58, and use **Y / R** to view it. Let it finish
without input, then report visual behavior and return to the selector. Keep
the game open for log inspection. This mode uses isolated research saves.

The dedicated probe build enables restart-required
`doax2_open_world_pictorial_hold_trial`, false by default. Ordinary pictorial
research explicitly disables it. Baseline omits the diagnostic module.
The shortcut does not enable the separate vacation hold or terrain trials.

## Admission and change

The experiment admits only the observed title table `0x82B83C80`, screen 2,
update `825A2270`, active state 1 and pictorial phase 7. The selected record
must be `0x83DA61BC`, scene ID 58. Shared script `0x833B4C68` must be active,
executing its opcode-16 wait at `0x82C2F758`, header/operand `0x081000D2`
(absolute target 210). Slot mapping must be (0,1,2,3), actor update mask 1,
slot-0 motion 27 and camera selector 0. Script +28 must be in [30,120).

Once admitted, only float script +28 is restored to its admission value,
for at most five seconds of host steady-clock time. Eligibility loss releases
the experiment at the next wait callback. It cannot start again in the same
thread/process. Every invocation retains the original wait handler exactly
once. Scene instructions, actor transforms, camera services, input, native
completion policy, saves and cleanup calls are unchanged.

This does not pause the whole picture. Animation and the camera continue under
their native clocks. The interpreter also advances script +24, +32, +36 and
+40 separately; those clocks remain untouched. Pictorial logs now record +24
as `script_elapsed` alongside `script_frame` (+28), so we can determine whether
outer completion follows a different clock. No five-second increase in total
scene duration is assumed. This finite experiment does not establish a usable
indefinite scene, looping idle, independent camera or locomotion.

## Validation

The diagnostic game and host timer test compiled and linked. The timer tests
passed for uneven callback cadence, the exact five-second boundary, ownership
loss, delayed callbacks, invalid numeric inputs and prevention of retriggering.
The user-operated runtime result is recorded below. The
prior unmodified scene-58 completion and early-exit runs are documented in
[PICTORIAL_LIFECYCLE.md](PICTORIAL_LIFECYCLE.md).

## User-operated result

Run `pictorialholdtrial-20261102-154427-868.log`, PID 18876, enabled the
pictorial hold with the vacation hold and terrain trial disabled. Scene 58
entered playback at update 576 with both clocks at 2. The trial began at
update 604, script frame 30, and released at update 755 after 5.033 host
seconds. Admission remained valid. Samples throughout the hold showed script
+28 at 31 (the interpreter increments it after the hook), actor mask 1 and
motion 27. Camera XYZ changed, and rig grounding continued to run.

The untouched +24 elapsed clock advanced from 30 to 181 during the hold.
Playback still began ending at elapsed 976, update 1550, while script +28
was only 826. Cleanup reached elapsed 990 / script frame 840, actor mask 0,
then restored selector phase 1 at update 1581. Normal completion previously
began ending at script frame 976 and cleaned up at 990. Thus this experiment
separated the clocks by 150 frames without extending the outer lifetime.
It must not be reported as a five-second extension of the scene.

The user reported completion but did not provide a visual-quality assessment
for this run. Both research save payload hashes still match the baseline;
no calendar advancement was logged. No crash was observed in the trace.

Source follow-up confirms `826F0540` at `826F1A14` reads shared script +24,
subtracts it from selected-record +16 and begins phase 8 when the remaining
duration is at most 15 frames. The end request is distinct from the observed
native input-exit branch beginning at `826F1A94`. Next is a separate bounded
experiment coordinating the script wait and this verified outer-lifetime
clock, retaining manual exit and all normal cleanup. Do not generalize the
single-clock hold into an indefinite scene.

## Coordinated lifetime experiment (pending runtime test)

**Launch Pictorial Lifetime Trial.cmd** selects a separate default-off flag,
`doax2_open_world_pictorial_lifetime_trial`. It disables the original
single-clock, vacation-hold and terrain flags. It uses the same scene-58
admission and one-shot five-second host timer, but records the initial +24
value and restores that value alongside script +28 while the hold is active.
Both clocks resume from their anchors after release; no accumulated time is
added on release. Camera and animation clocks, input, manual exit and cleanup
remain native. Other pictorial modes explicitly leave this flag disabled.

The prediction is approximately five additional seconds of scene lifetime,
with +24 and +28 remaining aligned through the hold, followed by normal return.
This prediction still needs the user-operated trace. The diagnostic game
compiled and linked and the finite-timer tests passed after the change.
Use the same Kasumi/Niki thumbnail, play once without input, and report visual
behavior and return to the selector. Keep the game open for log inspection.

## Coordinated result: lifetime extended, visual regression

User-operated run `pictoriallifetimetrial-20261102-154845-194.log`, PID 2920,
admitted scene 58 at update 527 with both clocks at 30. It released at update
677 after 5.001 host seconds with both clocks at 31. Phase 7 began at update
499 and phase 8 began at 1622, with both clocks at 976. Cleanup reached 990,
actor mask 0, then returned to selector phase 1 at update 1653. Playback ran
about five seconds longer than the unmodified scene. Both save hashes remained
unchanged. The user reported that the camera snapped away from the character
near the end, held briefly, and then faded. This is a visual regression, not
a successful extended-scene acceptance test.

Camera selector 0 stayed selected. While still in phase 7, the sampled XYZ
changed from (1.5201,-1.0641,-4.9736) at update 1470 / script frame 824 to
(0.3044,-0.9134,4.1112) at update 1500 / frame 854. By update 1560 / frame 914
the camera was near (0.0375,-0.9149,3.5776), and remained near that position
through update 1620 / frame 974, just before the fade. These samples support
the user's reported jump/hold, but do not identify the exact writer or prove
the camera's script reached its end.

Independent camera timeline advancement is already established in source:
`8241D038` advances object +3540 and may advance/clear script pointer +3536.
Desynchronization between that timeline and the held scene clocks is therefore
the current hypothesis. Next, trace the active camera object's script pointer,
timeline, entry index and transition flags alongside the scene clocks before
changing camera ownership or adding further holds. Animation clocks also
remain independent. Do not expand this trial to an unlimited hold or describe
its return/unchanged saves as proof of visual correctness.

## Camera script observation for the repeat

Probe v10 adds a read-only before/after scope to the existing `8241D038`
wrapper. It admits only scene 58 in the verified title pictorial owner,
phases 4..10, and the selected camera object calculated from the native
object array and bounded selector. The full 4144-byte record must pass the
existing readable-memory check. No camera pointer is replaced and the original
camera evaluator still runs exactly once, retaining existing native60 logic.

`OWPROBE camera script` records script pointer +3536, timeline +3540,
subframe +3544, entry index +3552, target +3556, mode +3580, active +3620,
and transition flags +2928/+2932. It reads the current entry's time and type
only through guarded reads. Logs are emitted on pointer/index/flag transitions
and every 30 updates, capped at 256 per scene. The scene's +24/+28 clocks
are included for correlation. This is not a new camera hold or an end fix.

Repeat **Launch Pictorial Lifetime Trial.cmd**, the same scene 58, without
input until return, and report the snap again if present. The new evaluator
trace has compiled and linked; runtime coverage of it is pending this repeat.

## Repeat result: early camera-script termination confirmed

The user repeated scene 58 and observed the same snap. Probe-v10 run
`pictoriallifetimetrial-20261102-160230-999.log`, PID 26424, recorded 43
camera-script observations. Camera object `0x839E9530` began its script at
update 496, advancing from entry `0x82D92A38` to `0x82D92A48`. The latter
entry has time 991 and type 0, the source branch that clears the camera script.

The scene-clock hold began at update 526, frame 30, and released at update
677 after 5.034 seconds. Camera timeline advancement continued during that
hold: at update 540 it was 44 to 45 while both scene clocks were 31.
At update 1487, with scene clocks only 842, the camera timeline reached
991 to 992, script pointer changed from `0x82D92A48` to zero, and flags
changed (3,0) to (1,1). Selector 0 and active=1 remained unchanged.
The scene did not enter its ending phase until update 1622, frame 976,
about 4.47 seconds later. This identifies a camera-script termination
150 frames ahead of the held scene timeline, consistent with the visual
regression. It is stronger evidence than XYZ samples alone.

Both save hashes remained unchanged and cleanup still reached the usual
selector phases. Next, coordinate the verified camera-script time and audit
its underlying camera motion-track progression against the same finite hold.
Merely delaying the outer fade or adding a longer scene hold would leave this
camera/scene divergence unresolved. Camera motion tracks and character
animation still require separate ownership/cadence analysis.

## Coordinated camera-clock trial (runtime pending)

**Launch Pictorial Camera Trial.cmd** enables only the new default-off
`doax2_open_world_pictorial_camera_trial` experiment, together with tracing
and pictorial availability. It retains the same five-second scene-58 wait
admission and holds scene +24/+28. It additionally holds the selected camera's
verified clocks around its normal `8241A630` evaluator:

- Camera-script +3540/+3544.
- Two motion-track clocks +128/+132/+136/+140 relative to tracks +832/+1024.
- Camera sequence gate/subframe +1940/+1944. In `82424558`, these are
  +1108/+1112 relative to the +832 base, and gate expiration dispatches shots.

Admission requires the observed camera script entry `0x82D92A48`, mode 3,
active=1 and selected slot 0. Track type, table length, finite phases, remaining
time and the sequence gate must place both tracks inside a shot, away from
dispatch/end boundaries. The original evaluator runs once and publishes its
normal output; only these twelve clock words are restored afterward during
the finite hold. No camera matrix, script pointer, motion resource, actor
transform, input or cleanup state is restored.

The scope checks camera-script and track identities after the native call.
If a script/track transition occurred, it preserves the transition, declines
clock restoration and releases the scene hold. It also checks the owning
scene, object and host deadline before each admitted call. Diagnostic logs
show native versus restored camera time, track phases, gate and call count.
All older launch modes explicitly disable this new experiment.

The build and host timer/boundary-policy tests pass. Runtime validation is
pending. Repeat scene 58 without input, noting the brief camera pause during
the hold and whether the end snap persists. Character animation remains
native and may move during the camera hold; this is still a finite ownership
experiment, not an accepted extended scene or independent follow camera.

## Camera trial result: framing preserved, actor motion expires early

The user completed **PictorialCameraTrial** run
`pictorialcameratrial-20261102-161700-316.log`, PID 10612. They reported that
the camera remained on the character at the end, while the character began
a brief walk-in-place animation. The scene-58 hold began at update 490,
frame 30, and released at update 640 after 5.000 seconds with both scene
clocks at 31. Camera hold logs confirm native timeline 31 restored to 30,
both track phases held at 30 and gate held at 961. The native evaluator
continued running. No camera-hold abort was reported.

The prior premature camera-script termination at timeline 991 / scene frame
842 did not appear in this run. Scene phase 8 began at update 1585 with
both clocks at 976. Cleanup reached frame 990 / actor mask 0 at update
1600 and restored the selector at 1616. Both save hashes remain unchanged.
This supports the camera-clock correction for this finite test, not a
general camera-ownership or extended-scene acceptance claim.

The rig trace last sampled motion request 27 at update 1440 / script frame
831, then sampled 8192 at update 1470 / script frame 861 and onward while
actor mask stayed 1. That is before the delayed fade at frame 976. The
8192 value was previously an inactive/request sentinel; do not label it a
verified walk animation ID. The user's visual report and this transition
indicate that native actor motion timing still diverges from the held scene.
Next, trace the animation phase/completion and fallback transition before
coordinating actor clocks. Do not freeze the whole character update or
interpret the walk-in-place as validated controllable locomotion.

## Actor transition observation (probe v12, runtime pending)

The camera-trial executable now observes slot 0 immediately before and after
the existing native `82513158` rig call. Admission requires the verified title
table, screen 2, update callback, active state, scene-58 record and pictorial
phases 5..8. No actor clocks or transforms are changed.

`OWPROBE pictorial motion` records scene +24/+28, actor update mask, player
+44/+48 as floats, motion halfwords +52/+54, and raw control words at
+56/+60/+64/+92. It also records the slot-table resource pointer used by
`82513158` and guarded raw words at resource +108/+112. Nested pointers are
not followed. These offset labels are observations, not established animation
phase or completion semantics. Identity/control changes log at either boundary;
unchanged state samples every 30 updates, capped at 256 records per scene.

Source inspection of `82511AB8` and `82515F30` establishes that motion values
at or above 8192 select a separate shared resource bank and subtract 8192
for lookup. Thus 8192 must not be treated solely as proof of inactivity or
as a known walk ID. Its meaning in this run still needs the transition trace.

The diagnostic executable compiled and linked, and `git diff --check` passed.
Runtime coverage is pending: launch **Launch Pictorial Camera Trial.cmd**,
repeat the same Kasumi/Niki scene 58 without input, report the character's
end behavior and leave the game open for inspection. Correlate the first
motion/control transition with both scene clocks before selecting another
hook or coordinating any actor clock. This instrumentation does not yet
identify the exact completion writer or implement an actor timing fix.

## Probe-v12 repeat: two playbacks, visual report needs attribution

Run `pictorialcameratrial-20261102-184731-650.log` contains two completed
scene-58 playbacks. The user reported a normal ending without walk-in-place;
which playback(s) this describes is awaiting clarification.

The first entered phase 7 at update 374, held from update 402 to 553 for
5.032 host seconds, and entered phase 8 at update 1498 / scene clocks 976.
Cleanup reached frame 990 / mask 0 at update 1513 and selector phase 1 by
1530. Rig samples still changed from motion 27 at update 1350 to 8192 at
1380 / scene clocks 858. This transition alone cannot override the user's
visual observation or establish that the visible regression was repaired.

The second loaded at update 1787 and ended at update 2779 / clocks 976,
then cleaned up at 2794 and restored selector phase 1 at 2810. The finite
hold is once per process, so this second playback had no hold. Rig samples
retained motion 27 through update 2790. Both research save hashes match
the baseline. No running `doax2` process was found during inspection.

The detailed observer exhausted its 256 records on each playback before
the ending: its alleged identity included the rotating rig-buffer pointer
and changing output words. All 512 recorded player control snapshots were
identical. Resource +108/+112 must not be interpreted as animation completion
fields from this evidence. Probe v13 compares only the fixed player control
words for transition events; rig-buffer values remain sampled payload.
It compiles and links, and `git diff --check` passes. The late transition
still needs runtime coverage with the corrected observer; no actor timing
fix has been made.

## Probe-v13 repeat: late actor completion captured

The user subsequently confirmed walk-in-place on their latest playback.
The latest log, `pictorialcameratrial-20261102-185021-169.log`, contains one
completed scene-58 playback with probe v13. The hold ran from update 442 to
593 for 5.033 host seconds. At update 1401 / scene frame 840, player +60
changed from `00000700` to `01010700` while motion remained 27. At update
1402 / frame 841, motion changed to 8192, +60 became zero and +64 became
`0000FFFF`. All were before-rig observations: the rig boundary does not
identify the writer. Fade began at update 1538 / frame 976, cleanup reached
990 / mask 0 at 1553, and selector phase 1 returned at 1569. Both research
save hashes still match the baseline. The visual regression remains present.

Source inspection identifies a matching completion writer in `8250FE28`:
it advances track +52, mirrors that float to +76 and its integer conversion
to +56/+72, compares +56 against unsigned halfword +112, and clears active
byte +127 when the end is exceeded. When both +125 and +127 are zero it
sets player byte +61; completion also sets player byte +60 on the applicable
path. This explains the shape of the observed control-word change, but does
not establish the exact subsequent writer of motion 8192. Native caller
`8250D708` invokes this routine for tracks 0 and 1. The track array starts
at `0x834271A0`, with 144-byte records and two records per actor slot.

## Coordinated actor-clock trial (probe v14, runtime pending)

**Launch Pictorial Actor Trial.cmd** enables the separate default-off
`doax2_open_world_pictorial_actor_trial`. It incorporates the verified finite
scene/camera hold and additionally restores the four clocks +52/+56/+72/+76
after each native `8250FE28` call for active slot-0 tracks. The native routine
still runs exactly once, and all animation, rig and grounding calls remain
enabled. No actor transforms, animation resources or completion bytes are
restored. The older camera-only launcher keeps actor holding disabled.

Admission retains the existing scene-58 owner and five-second one-shot wait
policy. At least one track must be active; each active track must have a
finite phase at least 1 and more than two frames before its native end.
Each actor callback rechecks title ownership, phase 7, selected record,
motion 27, actor mask, host deadline and track eligibility. Changes to the
sampled track descriptor/control words during the native call preserve the
transition, restore no clocks and release the coordinated hold. Actor hold
logs record native and restored phases every 30 updates.

The diagnostic executable compiled and linked. Finite-timer and actor-boundary
tests passed, the launcher parsed successfully, and `git diff --check` passed.
Runtime behavior remains unverified. On a fresh process, select the same
Kasumi/Niki scene 58 and play it once without input. Expected behavior is a
brief coordinated pause, then normal playback and fade without the prior
late walk-in-place. Check for position drift or repeated motion during the
pause as well; clock alignment alone does not prove correct root motion.
Leave the game open after returning so the trace can be inspected. Replaying
within the same process does not repeat the one-shot hold.

## Actor trial result: late fallback removed; camera jitter remains

User-operated run `pictorialactortrial-20261102-185818-429.log`, PID 23920,
enabled only the new actor trial. The user observed the pause and reported
that the scene ended without walk-in-place, with some camera jitter. The
timing of that jitter within playback has not yet been specified.

The hold began at update 595 and released at 746 after 5.033 host seconds.
Actor track `0x834271A0` repeatedly advanced natively from 31 to 32 and was
restored to 31. Camera timeline advanced from 30 to 31 and was restored to
30, with both camera track phases at 30 and gate at 961. No actor or camera
hold abort was logged. The ordinary one-frame phase relationship between
the callbacks is not evidence of an accumulating timing mismatch.

Motion 27 and player control word `00000700` remained in every late sampled
rig observation through scene frame 966; the earlier 8192 transition at
frame 841 did not recur. Camera script also remained live through frame
966. Phase 8 began at update 1691 / frame 976, cleanup reached 990 / mask 0
at 1706, and selector phase 1 returned at 1722. Both research save hashes
still match baseline. No running game process was found at inspection.

During the hold, sampled player position stayed at (-2.2798,0.0819,9.8097).
Camera XYZ varied slightly despite held script/track clocks: samples at
updates 600..720 ranged approximately X 8.3804..8.3856, Y 2.7201..2.7340,
Z 7.6059..7.6174. This establishes residual camera output variation during
the hold, not its writer or whether it accounts for all reported jitter.
Thirty-update samples cannot characterize frame-to-frame jitter. The next
camera investigation should distinguish variation within the native camera
evaluator from changes applied afterward, correlating actor/target position
and the release boundary before changing additional clocks or camera output.

This is one successful visual check of removal of the late actor fallback
in the finite extension. Camera smoothness remains unresolved; it is not
acceptance of an indefinite scene, independent camera or locomotion.

## Paused checkpoint — 2026-11-02

The user clarified that the camera jitter seemed intentional. Treat it as
likely native camera motion, not a confirmed regression. The residual camera
variation recorded above remains an observation; no jitter fix is currently
justified. This supersedes treating camera smoothness as a required defect
investigation based solely on this run.

The five-second coordinated extension milestone is supported by this single
user-operated run: the pause occurred, playback resumed, and the scene ended
without the prior walk-in-place or reported premature camera snap. Normal
cleanup and unchanged research saves were verified in the trace. This does
not establish indefinite playback or independent actor/camera control.

Work is paused at the user's explicit request. Probe v14 is built in
`out/open-world-probe/doax2.exe`; **Launch Pictorial Actor Trial.cmd** selects
the new experiment. No longer hold has been implemented or tested. No game
process was running at the last inspection. Existing workspace changes are
uncommitted and have been left in place.

On resume, retain this five-second trial as the reference. The next proposed
step is a separate longer bounded extension, followed by a manual-exit check
while its hold is active, verifying native cleanup and unchanged saves. Each
hold remains once per process, so comparisons require fresh launches. Do not
silently lengthen the existing reference trial or proceed directly to an
unlimited hold. No further testing or implementation is requested while paused.

## Resumed: separate fifteen-second actor trial (probe v15)

The user resumed work from the paused checkpoint. The five-second actor trial
remains the reference, with its original duration unchanged. No jitter fix
was introduced: the user's clarification that it seemed intentional stands.

**Launch Pictorial Long Actor Trial.cmd** selects the new default-off
restart-required flag `doax2_open_world_pictorial_long_actor_trial` and mode
`PictorialLongActorTrial`. It uses the same scene-58 admission, native calls,
scene/camera/actor clock restoration and transition checks as the reference,
with a fifteen-second host deadline shared by the scene and camera/actor
holds. Other launch modes explicitly disable the longer trial. No additional
clocks, transforms, resources or completion bytes are restored.

The host timer latches its duration on admission, rejects durations outside
(0,15] and nonfinite values, and remains one-shot after release or ownership
loss. Probe v15 logs both the new flag and the selected duration. The diagnostic
executable compiled and linked. Host tests passed for the original boundary,
fifteen-second boundary, duration latching, cancellation before expiry,
no retriggering and invalid durations. The launcher parsed and diff checks
passed. Runtime behavior of the longer hold remains unverified.

Next user-operated checks, each in a fresh process:

1. Launch the longer trial, select the same Kasumi/Niki scene 58 and use Y/R
   to view. Let playback finish without input. Observe the longer pause,
   resumed motion, end framing, any drift, and return to the selector.
   Leave the game open for trace and research-save hash inspection.
2. After inspecting that run and closing normally, launch the longer trial
   again. Enter the same scene and request the normal native exit while the
   fifteen-second hold is still active. Verify prompt return and native
   cleanup; leave the game open for the separate trace/save check.

The second run must use a fresh process because the hold cannot retrigger.
Neither runtime check has been performed at this checkpoint. Do not describe
this as accepted indefinite playback or independent actor/camera control.

## Fifteen-second actor trial result: normal completion

The user reported that the latest Long Actor trial succeeded: Kasumi held in
place for approximately fifteen seconds near the beginning, then the scene
ended normally, with no issues observed.

The latest trace, `pictoriallongactortrial-20261102-201545-911.log`, supports
that report. The coordinated hold began at update 956, script frame 30, and
released at update 1407 after 15.033 host seconds. No hold abort was logged.
Scene 58 entered ending phase 8 at update 2352 / frame 976, cleanup phase 9
at update 2367 / frame 990 with actor mask 0, then phase 10 at update 2368
and selector phase 1 at update 2383. Both research save payload hashes still
matched the recorded baseline. No game process was running at inspection.

This completes the visual normal-completion check for the longer bounded
hold. The separate fresh-process check of native exit while the hold is
active remains outstanding. Indefinite playback and independent actor/camera
control remain unverified.

## Fifteen-second actor trial result: exit during active hold

Fresh-process run `pictoriallongactortrial-20261102-201858-003.log`, PID
10516, verified requested exit during the longer hold. The user reported
successful return: Kasumi moved after B was pressed and the screen faded out.

The hold began at update 676 / frame 30 and released at update 711 after
1.167 host seconds with eligibility false, well before its fifteen-second
deadline. Ending phase 8 followed at update 712 / frame 32. Native actor
motion 27 and camera timeline advancement were observed during that ending,
consistent with the reported movement during fade. Cleanup phase 9 reached
update 727 / frame 46 with actor mask 0, phase 10 followed at update 728,
and selector phase 1 returned at update 743, about 1.04 seconds after hold
release. Both research save payload hashes still matched baseline. The game
remained running and responsive at inspection.

Both planned longer bounded-hold checks now passed: normal completion after
the full hold and native exit while the hold was active. This supports the
tested finite extension and cancellation path for scene 58; indefinite
playback and independent actor/camera control remain unverified.
