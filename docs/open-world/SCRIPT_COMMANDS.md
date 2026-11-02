# Verified relaxation actor and camera commands

Latest experiment: [RELAXATION_HOLD_TRIAL.md](RELAXATION_HOLD_TRIAL.md) records a five-second script-clock hold with an active actor and independently moving native camera. Cleanup succeeded; the user may have ended playback with Enter, so the return's cause is unconfirmed. Reliable extended lifetime remains unresolved. The report also identifies main-menu and character-selection unlock predicates for a runtime-only pictorial test mode.

Checkpoint: 2026-11-02 UTC. This extends [SCENE_OWNERSHIP.md](SCENE_OWNERSHIP.md). Findings apply to the locally verified executable; no game assets or generated function bodies were changed.

## Dispatch and encoding

`sub_824B95F0` reads the instruction pointer from script state +52, loads a big-endian 16-bit header, and dispatches through table `0x82C3E148`. The low 10 header bits select the handler. The traced handlers advance by `(header >> 10) * 2` bytes, so the upper six bits encode command length in halfwords. This length rule is confirmed for the handlers below; control-flow/termination commands need their own contracts. Only the needed table entries were inspected; no claim of decoding the entire instruction set is made.

The active script state is `0x833B4C68`. Bytes +138..+141 map four script actor references to native player slots. Handlers generally use that mapping before indexing 152-byte player records at `0x833B8520`. Identity setup, resource allocation, activation, motion and placement are separate operations.

| Opcode | Handler | Confirmed source behavior |
|---|---|---|
| 22 | `0x824BA0E0` | Resource preparation through metadata and character/motion resource services; not independently runtime traced here |
| 25 | `0x824BA2C0` | Resolve an actor through the script mapping and `sub_824FEC88`, reset several motion fields |
| 28 | `0x824BA508` | Apply four packed nibbles to mapped player +10 update flags; nibble 15 preserves that slot |
| 30 | `0x824BA6C8` | Reset motion state and set player +66 from an operand; this differs from opcode 31 |
| 31 | `0x824BA758` | Select mapped actor, assign 16-bit motion +52 and an additional +58 operand, reset playback fields |
| 32 | `0x824BA820` | Select mapped actor and optional signed placement-preset index; set X/Z, query native stage height for Y, copy facing, update associated state and call native refresh services |
| 34 | `0x824BAA98` | Update player +75 flags through mapped slots; this is distinct from +10 activation |
| 85 | `0x824BC560` | Configure camera slot 0's script/preset pointer and fields, select slot 0 if active, initialize/reset slot 1 as needed |
| 86 | `0x824BC728` | Configure camera slot 7; not independently runtime traced here |
| 87 | `0x824BCB50` | Pass current camera selection and operand to `sub_8241A338`; not independently runtime traced here |
| 90 | `0x824BC898` | Pass a preset vector/parameter to `sub_82418DC0` for the selected camera; not independently runtime traced here |

Opcode 32's preset source is `0x82219000`, with 12-byte X/Z/facing records. A preset index of -1 skips position assignment but still runs other refresh work. This is a native grounded placement path, not proof that every preset is valid for every stage or that the underlying query provides obstacle collision.

Opcode 85 receives a signed camera preset operand and reads a pointer table at `0x82E15A30`. It writes camera record +3536/+3540/+3544/+3548/+3552 and a parameter block at +3696. A negative operand clears selected script fields. The subsequent per-frame camera evaluator still needs tracing before replacing it with a follow camera. Calling this handler alone does not establish independent camera ownership.

## Runtime verification

Probe v5 adds bounded read-only observations around opcodes 28, 30, 31 and 85. Each observation saves command words and mapping before the original advances its instruction pointer, then records actor mask, motion fields and camera selection afterwards. It retains the original guest call exactly once. The first three handlers already had native60 diagnostic wrappers; observations now live inside those wrappers, preserving their scan/depth behavior and early-return paths. An initial duplicate-symbol link failure exposed that overlap and was corrected before validation.

The observer checks script identity, vacation table, screen 7, its verified callback, mapped command bytes and declared length. It reads at most three operand halfwords and stops after 128 observed commands per thread, in addition to the existing trace line cap. Unavailable operand positions in the log are zero-filled; `words` determines which are present. The actor mask represents nonzero +10 flags, not an allocation count. Failed admission checks leave the original call intact. Ordinary Baseline builds omit these observations; the probe CVar is false by default.

Run `probe-20261102-035324-271.log`, PID 14408, used Probe with terrain trial disabled. The actual original menu offered **Tranquil Beach Relax** on Day 4 Evening. Niki was occupied, so this validation must not be described as new Niki runtime evidence.

At update 3456:

| Observed command | Result |
|---|---|
| Opcode 28 at `0x82C2F442`, operand `0xFFF1`, mapping (0,1,2,3) | Actor update mask changed 0 to 1; the other three flags were preserved |
| Opcode 85 at `0x82C2F44A`, operand 93 | Camera selection changed 6 to 0 |
| Opcode 31 at `0x82C2F47A`, operands (0,33,0) | Player slot 0 motion became 33; other slots retained motion 8192 |

The captured scene at 03:55:33 UTC showed Kasumi standing on the beach. This supports the observed native activation/motion path; it does not establish that motion 33 is a generic looping idle or portable to a separately loaded actor.

The same locally captured script contains opcode 32 at `0x82C2F482`, targeting reference 0 and preset 77. Read-only inspection of the preset gives X=16.687, Z=-11.083, facing=49516. This is source/image evidence associated with the observed script, not an independently hooked opcode-32 execution or a Niki spawn recommendation. The normal native handler must validate its own stage conditions.

Playback completed normally. Calendar progression from `0x826E2E5C` advanced 14 to 15 at update 4401; active location selection resumed at update 4503, followed by the original hotel transition. The application was closed normally afterwards. No crash was observed.

## Validation and next experiment

Both Probe and diagnostics-off Baseline builds compiled and linked. Each reported the existing five `fopen` deprecation warnings; no remaining compiler or linker error occurred. `git diff --check` passed with the existing manifest line-ending warning. The native60 counter-scan-enabled path was preserved in source but was not separately runtime-tested in this run. No disabled-mode activity regression or performance comparison was performed.

The next narrow experiment can now target a **native-script-initialized single actor**, preserving the real resource preparation, activation, motion, placement and teardown sequence. Before holding that scene, establish a safe script wait/control point and whether its motion loops without script maintenance; trace per-frame camera evaluation so it can continue rendering while script commands are held. Give the experiment an explicit exit that resumes normal stop/cleanup. Suppress progression only within the experiment, at the verified caller boundary, without skipping cleanup or modifying original saves.

This is still a preparation milestone. Arbitrary actor spawning, walk/run clip bindings, independent camera control, unlimited scene lifetime and free-roam movement are not implemented.
