# Niki stage binding and relaxation lifecycle

Latest follow-up: [SCRIPT_COMMANDS.md](SCRIPT_COMMANDS.md) identifies and runtime-verifies actor activation, motion selection and camera setup commands. Native grounded placement is mapped in source. These findings replace the earlier outstanding-command list below; isolated lifetime and camera evaluation remain unresolved.

Research checkpoint: 2026-11-02 UTC, branch `feature/open-world`. Evidence is generated code for the locally verified executable and ignored local runtime captures. No generated game code or original user saves were modified.

## Stage resource binding

Confirmed source chain:

1. `sub_825BB430` reads the stage selector byte at `0x833B745F`. This is distinct from activity selector word `0x833B7454`.
2. It copies a pointer from `0x833B88B8 + selector*12` into `0x839850A8 + selector*4`, resets secondary terrain bindings, and calls `sub_825BB338`.
3. `sub_825BB338` selects a stage object through table `0x82B82CF8`. It clears the main terrain's additional geometry, associates its owner flag, and calls stage vtable +76 unless a separate suppression state requests the empty-terrain reset `sub_825C03E0`.
4. In the saved Niki image `image-16600-20261102-010921-946.bin`, selector 0 and variant 15 select table index 30, object `0x83C821E0`, vtable `0x823C1398`, method +76 = `sub_82613428`.
5. `sub_82613428` invokes `sub_82613218` and `sub_82613330`. The first binds the main terrain object `0x82E2DB50` with 453 vertices and 784 triangles; the second binds object `0x82E2DC90` with 471 vertices and 808 triangles. Both derive geometry addresses from relative offsets in the resource. The second object's gameplay purpose is not yet established.

The saved image's resource source and active pointer are both `0xE940F000`. The main vertex/triangle pointers observed at runtime are `0xE940F068` and `0xE9410CB8`. These values correlate the binding chain with the validated geometry capture. The code does not allocate or copy those buffers in the setter: `sub_825BAD40` stores the supplied pointers/counts and fallback height. Consequently, preserving the terrain object alone cannot preserve its resource lifetime. The allocation, loading, ownership and eventual release of the resource pointer remain to be traced.

The observation log `terraintrial-20261102-033037-702.log` records main-terrain binding from return address `0x82613290`, including during location selection and exit. A populated terrain object therefore does not by itself establish an active gameplay scene.

## Native terrain query policy

Both observed terrain objects use vtable `0x823BFD30`:

| Slot | Method | Confirmed behavior |
|---|---|---|
| +8 | `0x825BAD40` | Store main geometry pointers, counts, fallback height |
| +12 | `0x825BAD58` | Store additional geometry fields +96/+100/+104/+108 |
| +16 | `0x825BAD70` | Bind spatial grid, bounds, cell scale and dimensions |
| +24 | `0x825BAF98` | Return 1 outside X/Z bounds, 0 inside |
| +28 | `0x825BAFD8` | Return object +84 fallback height |
| +36 | `0x825BB530` | Native triangle/radius query |
| +44 | `0x825BEE88` | Optional override, otherwise native query and height selection |

`sub_825BEE88` returns the fallback method when the native query returns zero. Niki's observed fallback is approximately -2.0569. A numeric returned height is not sufficient evidence of a supported ground contact. A future controller must distinguish no-hit/fallback from valid support using the native query result; it must not simply assign every returned Y to the actor.

Multiple-contact cases use additional radius and height selection calculations. Offline triangle projection cannot replace that policy. Material flags, obstacle collision and slope limits remain unknown.

## Original Niki relaxation scene

Run `terraintrial-20261102-033451-717.log`, PID 9784, first entered evening Niki volleyball and exited normally. Calendar advanced 10 to 11 from `0x826CF994`; active location selection resumed at update 3773. Normal hotel Go to Bed then reached Day 4 Morning in the same process. With no partner and Niki unoccupied, its actual menu offered **Relax**.

Selecting Relax entered vacation descriptor table `0x82AACFD0`, screen 7:

| Responsibility | Address |
|---|---|
| Initialize | `0x826E2068` |
| Update | `0x826E2618` -> `0x826E2EE8` |
| Exit/transition callback | `0x826E2648` -> `0x826E2C18` |
| Other descriptor callbacks +16/+28 | `0x826D7838` |

Screen 7 reached active state at update 9072. The capture at 03:40:02 UTC showed Kasumi seated with a drink on a Niki lounger. The native rig hook recorded only slot 0 in this scene's sampled interval, with motion field +52 = 17 and resource identity `0x8342B220`. At update 9150, query caller `0x82513210` returned Y=0.2560 at X=12.1651, Z=6.0592; the rig's ground_y matched. This establishes a native rig and ground-query path behind the observed relaxation scene, rather than relying solely on its rendered appearance. It does not establish that all relaxation content is live 3D, or that this motion ID is portable idle/walk/run animation.

The scene installed predicate `0x824C0288` and height callback `0x824C02B8`. Source shows a state-dependent predicate and a callback reading a stored float; their presence does not establish which path produced every height. All sampled queries reported `terrain_trial=false`: the experiment remained confined to its volleyball guard.

The original sequence completed without further input. Screen 7 began exit at update 9746; the original calendar call from `0x826E2E5C` advanced 12 to 13; active location selection resumed at update 9878. Captures confirmed Day 4 Daytime. No crash was observed. A geometry snapshot attempted after completion correctly refused zero/unbound geometry and retained only the local image; it is not a valid relaxation geometry capture.

This is same-process Niki volleyball -> normal exit -> hotel -> Niki relaxation -> normal exit coverage. It is not repeated entry into an identical activity, a soak test, or proof that all resources were released. At Day 4 Daytime, Niki was occupied and the menu offered gifts/partner requests; without a partner volleyball was unavailable. No availability or calendar state was forced to manufacture another entry.

## Actor setup boundaries and next experiment

Both common activity initialization `sub_826CF468` and relaxation initialization `sub_826E2068` call `sub_826CDEF0` to prepare character identities. The latter passes -2, which resolves an existing scene selection. That routine writes four-slot identity/setup records and recognizes 254/255 sentinels; it is not a verified independent actor constructor.

Native predicate `sub_82511FA8` checks player +10, identity +4, motion +52 and resource-state bytes. Relaxation cleanup `sub_826E2C18` clears player +10 and +75 across the four records before additional cleanup and progression. Setting unused identities or clearing an update flag alone would not establish safe resource cleanup or a standalone actor.

The smallest next research task is to trace relaxation's `sub_826E2AC0`, script startup `sub_824BF888`, rig/resource activation and teardown `sub_82502408` / `sub_826CEE40`. Compare the native single-slot path with volleyball's four-slot path and map compatible idle/walk/run resources. Preserve normal loading and cleanup while establishing the completion condition and camera writer. Only then attempt a separately gated stationary actor session with progression suppressed locally. Free roam, arbitrary spawning and locomotion are not implemented at this checkpoint.

## Relaxation startup and teardown follow-up

Further source tracing corrects the earlier provisional description of `sub_824BF888`: it prepares referenced resources, rather than starting playback. It obtains script metadata through `sub_824BF070`, iterates four character references, requests character/motion resources through `sub_8266CC48` / `sub_8266D3D0`, then handles stage and other referenced resources. `sub_8266CC48` searches reusable resource slots before claiming a free slot; it is not a standalone player constructor.

`sub_826E2AC0` builds a recent-scene history record. It gathers one actor's metadata, or two for its type-2 branch, using `sub_826E29B8`, and shifts eight 56-byte history records. It is not the missing actor spawn routine. `sub_824BEE88` walks script instructions to estimate duration; its result at relaxation state +8 is not a scene handle.

The actual shared playback controller is `sub_826E2EE8`, with state block `0x83975920` for vacation screen 7. `sub_825A0FD8` passes descriptor +32 as r4, confirming this block's ownership. Its source branches on state +0:

| State | Source behavior |
|---|---|
| 0 | Select current part, call `sub_824B8AB0` to initialize the script, compute duration and advance to 1 |
| 1 | Wait for positive script progress, initialize transition UI and advance to 2 |
| 2 | Wait for transition UI completion, set playback state 3 and optionally initialize sound |
| 3 | Run playback policy, compare estimated remaining frames and handle original input/ending paths; eventually advance to 4 |
| 4 | Wait for ending transition, call `sub_825020F0`, advance the part or request screen completion |

Script state is `0x833B4C68`. `sub_824B8AB0` resets per-script resources and calls `sub_824B8C00` / `sub_824B8C60` to initialize it. `sub_824B8978` runs a yielding script service loop, invoking `sub_824B95F0` and other per-frame services while active. The interpreter dispatches commands through a table and advances progress counters, including float +28. Playback cannot be safely replaced by merely freezing the outer screen state: script commands and their services still own actor and camera changes.

Teardown is also layered. `sub_825020F0` waits for native services, performs stop/update work while yielding, and releases a nonzero stored resource through `sub_8286FEA8`. `sub_82502408` waits for a resource/service state and preserves playback settings; it is not simply a rig destructor. Script cleanup `sub_824B91A0` clears script activity and actor +75 flags, resets camera slots/selection and restores stage-associated state. Outer relaxation exit additionally clears actor +10 flags and runs `sub_826CEE40`, which waits for other resource operations and restores location-selection state. None of these waits should be bypassed in an isolated mode.

### Read-only phase trace

Probe v4 observes the verified screen/table/update callback and descriptor parameter before reading the fixed relaxation state. It logs phase changes plus samples every 30 updates: part/count, estimated duration, script progress, actor +10 update-flag mask, readability mask and current camera index. The mask records update eligibility, not a count of allocated or visible actors. The trace does not call guest functions, change registers, write memory or alter playback policy. It shares the existing default-off diagnostic gate and line cap, and adds no hooks to ordinary builds.

Both incremental Probe builds compiled and linked after this change. Run `probe-20261102-034529-136.log`, PID 15312, used **Probe**, with the terrain trial off. Niki was occupied in the isolated save, so runtime validation used the available original **Tranquil Beach Relax** option, not a forced Niki scene. The shared screen-7 trace recorded phase 0 at update 3508, phase 1 at 3522, phase 2 at 3532 and phase 3 at 3547. Actor update mask changed from 0 to 1 as script progress became positive; camera selection changed from 6 to 0. Estimated duration was 990 frames and script progress advanced during playback. This validates the new instrumentation on the shared lifecycle; it does not add fresh Niki-specific geometry evidence.

Remaining work is to decode the script commands that activate actor +10, choose motion resources and configure camera slot 0. The verified yielding script loop and cleanup path are now concrete trace boundaries. No indefinite hold, progression suppression, independent spawning or controller change was introduced.

The validation run subsequently reached phase 4, stopped at script frame 990, and cleared the actor update mask to 0. Original cleanup advanced the calendar 13 to 14 from `0x826E2E5C` at update 4520 and returned to active location selection at update 4622. No crash was observed. The game was closed normally after this check. `git diff --check` passed, with only the existing manifest line-ending warning. The new observer is build- and runtime-validated; a disabled-build activity comparison and a repeated same-activity soak test remain outstanding.
