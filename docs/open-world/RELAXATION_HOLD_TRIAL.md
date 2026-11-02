# Finite relaxation script-clock experiment

The proposed menu unlock below has since been implemented. See [PICTORIAL_RESEARCH.md](PICTORIAL_RESEARCH.md) for its controls, confirmed title/character access, viewed-scene availability override and remaining playback checks.

Checkpoint: 2026-11-02 UTC. This follows [SCRIPT_COMMANDS.md](SCRIPT_COMMANDS.md). The experiment is preparation for independent actor lifetime; it is not free roam or an unlimited single-actor mode.

## Implementation

The diagnostic build adds restart-required CVar `doax2_open_world_relaxation_hold_trial=false`. Launch explicitly with `scripts/run-open-world-research.ps1 -Mode RelaxationHoldTrial`; ordinary Probe, ProbeDisabled and TerrainTrial launches explicitly disable it. Baseline omits the diagnostic module entirely.

The original opcode-16 handler `sub_824B9EA0` compares the script's float +28 clock with a signed instruction operand. Before the deadline it yields; after the deadline it advances the instruction pointer. The wrapper executes this native handler exactly once. The trial holds only float +28 at an admitted frame for five seconds of host monotonic time, then permanently releases the hold for that guest thread. It does not modify script instructions or assets. A changed admission condition also releases an active hold. No new scene, identity, resource, motion, transform, camera, calendar or save writes are added by this trial.

Admission requires all of:

- Verified script object `0x833B4C68` and vacation screen 7 / table `0x82AACFD0` / update `0x826E2618`.
- Active screen state and relaxation playback phase 3.
- Active script byte +0 equal to 1 and an instruction in the observed static script region `0x82C00000..0x82C3FFFF`.
- Command header `0x0810` (opcode 16, two halfwords), positive absolute frame target 40..600, with more than five script frames remaining before that target.
- Camera selection 0, identity script-slot mapping (0,1,2,3), and actor update mask 1.
- Player-0 motion in the native below-8192 resource range, and script frame at least 30 and below 120, with readable fixed-state and instruction memory.

During a successful hold the native interpreter can still increment the clock after each yield; the next invocation restores the admitted anchor. Therefore frame samples should remain near the anchor, rather than necessarily equal it at every independent sampling boundary. Other script counters are not frozen. This deliberately narrow experiment must not be generalized to arbitrary scripts without checking their other clocks and control flow.

The wrapper does not suppress normal calendar progression. The expected successful test is an extra five seconds of script wait followed by ordinary playback, stop/cleanup and the original return. Unlimited vacation policy remains a separate, mode-local change.

## Camera ownership finding

Camera update `sub_8241A630` checks object +3620, calls `sub_8241CD58` and camera-script evaluator `sub_8241D038`, then continues native track/output services. `sub_8241D038` reads camera script pointer +3536 and timeline +3540, evaluates script entries, advances the pointer in 16-byte increments, and advances its own timeline when the native pause condition allows. That clock is separate from the relaxation interpreter's +28 clock. Existing native60 wrappers already cover these functions and retain their original timing behavior.

Holding outer relaxation progress therefore does not guarantee a static camera, frozen character animation or constant character position. This trial intentionally leaves those services running so their behavior can be observed. A follow camera will need explicit ownership of the per-frame output path rather than merely selecting camera 0.

## Validation status

The updated Probe build compiled and linked successfully. Runtime launch `relaxationholdtrial-20261102-040036-414.log`, PID 4984, confirmed probe v6 and the hold flag enabled, with the terrain trial disabled. The isolated save resumed at the original hotel/night flow; normal navigation reached Day 5 Morning. Niki and Tranquil were occupied in that period, so the matching hold had not yet executed at this initial checkpoint.

No successful hold, stationary-actor milestone, repeatable cleanup or Niki hold is claimed from launch/build evidence alone. Original user saves remain outside the research namespace. Captures, logs and derived image/geometry data remain local under ignored `out/`.

Initial revisions admitted exact script addresses. Their runtime sessions exercised two Bass Island scenes and a Niki scene and returned normally without activating the hold. Bass scenes advanced the isolated calendar 16 to 17 and 17 to 18; Niki subsequently advanced it to 19. These were admission/normal-playback observations, not successful hold tests. Motion 34's script was additionally inspected in source/image, including its early opcode-16 target-40 wait.

The final implementation uses the shared, verified opcode-16 active-wait contract above instead of requiring a randomly selected clip's exact address. This accommodates normal scene variation while still checking the actual executing handler, instruction encoding, script identity, playback phase, actor and camera state, and clock/deadline bounds. It remains a five-second experiment, with no timer, resource or actor-control bypass. The revised Probe build compiled and linked successfully.

## Executed hold and its limitation

Final run `relaxationholdtrial-20261102-041459-543.log`, PID 4216, selected the actual original **Bass Island Relax** option on Day 6 Morning. Motion 809, camera preset 597 and actor mask 1 were initialized by native script commands. The trial admitted opcode 16 at `0x82C3C07C`, beginning at update 9473 / script frame 30. It released at update 9623 after exactly 5.000 host seconds, with eligibility still true and the sampled script frame 31. This is 150 native updates at the research 30 Hz settings.

Samples at updates 9480, 9510, 9540, 9570 and 9600 all showed script frame 31, playback phase 3, actor update mask 1 and camera index 0. Camera XYZ changed across those samples, confirming that native camera services continued independently. After release, update 9630 showed script frame 38. The attempted automatic screenshot was captured at 04:20:14 UTC, after scene exit; it is not visual evidence from inside the hold.

**Validation limitation:** cleanup began at update 9645, only 22 updates after release, while the clip's estimated duration was 1060 frames. Calendar progression from `0x826E2E5C` advanced 20 to 21, and active location selection resumed at update 9747. The user subsequently reported possibly pressing Enter and ending the scene accidentally. The cause is therefore unconfirmed: this run must not be treated as evidence that the clock hold caused premature completion. No crash was observed. The logs establish a five-second actor-active hold and a successful normal cleanup path; full authored playback after release needs a repeat without manual input. Reliable extended scene lifetime, a stationary actor, preserved animation synchronization and unlimited exploration remain unverified. Keep the trial off outside explicit research.

The original skip/completion branch needs additional observation before attributing the return to input, a particular clock or native completion state. The camera's separate timeline is confirmed. No native completion flags or cleanup waits were bypassed.

## Main-menu pictorial access as a test improvement

The user proposed unlocking virtual pictorials from the main menu to make testing repeatable. Source tracing identifies title initializer `sub_825A0060`: it hides menu record ID 5 (internal UTF-16 label `Slide`) and enables it if any character passes `sub_82410698`. The predicate is called at return address `0x825A0110`. The character-selection path `sub_826EE568` calls the same predicate at return address `0x826EEA58`.

`sub_82410698` checks character-specific collection/ownership data and can cache a successful result by setting bit 16 at profile record +31400. Therefore a research unlock should override the result at those two verified callers without executing the mutating predicate in the override path. It should be default-off, accept only character indices 0..8, and leave normal calls unchanged. This is a concrete runtime-only approach; it has not yet been implemented or runtime-validated.

The existing **Precious Memories** entry was inspected and displays the photo-album UI, not a demonstrated direct pictorial selector. Its title-table screen 3 uses `sub_82591C68` / `sub_82591CB8`. Do not conflate this album with the hidden `Slide` entry or assume its lifecycle matches vacation screen 7. The hidden entry's displayed label, clip availability and playback descriptor still need runtime confirmation. A direct selectable scene would avoid repeated vacation navigation and make the early-return issue much easier to reproduce.
