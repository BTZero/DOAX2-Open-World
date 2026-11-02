# Bounded native idle trial

The user authorized tracing volleyball idle resources and preparing an
animated stationary idle in the verified Kasumi/Niki pictorial shell.
Beach Flags remains explicitly deferred; no unlock was changed.

## Resolver and resource findings

`826923A0` selects a per-character neutral list, while `82692BD8` handles
random delayed idle requests and cooldowns. Both map character byte +1
through `82BE4B58`, index nine list pointers per character at `82AE2888`,
then resolve list indices through halfwords at `82AE2260`. For reference
character byte 5 (mapped index 1), controlled-neutral set 1 contains native
270..275. Set 0 contains 541..547. This explains the mixed low-level clips
in the volleyball capture even while high-level ID +64 stayed zero.

Live reference snapshot `image-3716-20261102-210405-819.motions.json`
showed that the pictorial's original bank does not contain these real idle
transforms: candidates 270..275 mostly point to the same five-frame fallback.
Motion 27 has the expected 990-frame data. A direct numeric substitution
without loading the normal package would therefore be invalid.

Native request `8266D040(slot, character, package, ...)` and asynchronous
loader `8266CE40` support separate character-motion banks. Default package
for byte 5 is the asset pair 446/447 at `82B29F78`, selected through
`8266D6F0`. The loader relocates the command table through `82518038` and
the transform table through `82511A28`. `8266D330` releases a bank through
the native resource manager. Native actor bank selection is player byte +4.

Probe v18's separate **Launch Pictorial Idle Bank Trial.cmd** tested only
loading an entirely empty, unbound bank. It left the actor on its original
motion and bank. Run `pictorialidlebanktrial-20261102-211121-452.log`, PID 4360,
requested slot 1 at update 2504 and reached ready state at 2511. Idle clip
headers were 115, 168, 86, 88, 91 and 153 frames for 270..275 respectively.
Snapshot `image-4360-20261102-211325-652.motions.json` captured the actual
normal-bank command streams and transforms. The fifteen-second hold ended
normally, phase 8 began at 3972, and native bank release in that update
cleared its loaded character/package to 255. Cleanup reached actor mask 0
at 3987 and selector phase 1 at 4003. This validates bank loading/release,
not an animated idle or a leak soak.

## Probe v19 implementation

**Launch Pictorial Idle Trial.cmd** enables the separate default-off,
restart-required `doax2_open_world_pictorial_idle_trial`. Reference hold,
actor, orbit and bank-only launch modes remain separate. This variant keeps
the verified fifteen-second scene/camera hold and bounded orbit, but lets
the replacement actor animation advance normally.

After native `824BF888` prepares reference scene 58, the experiment requests
the default Kasumi package only in a completely empty bank slot 1..3. It
does not evict cached banks. Admission verifies the real clip-270 header
and command prefix, plus the existing shared native loop command. It
requires the supported title owner, reference selected record, script wait,
slot mapping, actor mask, camera shot and normal initial actor track bounds.

On admission it records the original bank, motion pair, rate/step, mirror,
flag 73, primary track phase and zero seek override. It submits motion 270
through the same player fields used by the native neutral selector, with
rate/step 1 and the primary request byte +56. The native binding/update
pipeline still executes. No volleyball activity/controller or ball scheduler
is called.

Clip 270's native command stream includes its face cues and ends at frame
115. The trial reuses native opcode 3 from the verified shared command
stream to enable the engine's own loop policy after its first clip-selection
command. `8250FE28` then performs native reload/restart at the end. It does
not repeatedly restore animation clocks or synthesize a pose. Other native
commands still execute once. Native cycles are logged at the actual clip
selection boundary, not inferred from elapsed host time.

While this trial owns slot 0 and the active hold, it zeros only that actor's
root delta before the original `82510218` call. The native routine still
runs and consumes the delta; the consumed value is not restored. No actor
world transform or terrain query is replaced. Root/position/phase samples
record whether stationary placement is actually maintained.

Deadline, exit or ownership loss ends the idle. If the actor is still owned,
the original bank/motion are requested through the native binder with its
supported seek field +116 set to the saved track phase. A scope around the
native `8250D5F8` pipeline clears that seek after the original track has
rebound, avoiding repeated reinitialization. Stale track/resource pointers
are never restored. The spare bank is released only after restoration has
finished and no active actor binds it; inactive cleanup also clears the
one-shot seek. External actor transitions are preserved rather than
overwritten by an old snapshot.

## Validation scope

The probe compiled and linked; finite-hold and orbit host tests passed.
Both research PowerShell scripts parsed and diff whitespace checks passed.
Host tests do not validate native resource compatibility, loop seams, feet,
facial expression restoration or runtime cleanup. Those require the fresh
runtime trials recorded below. This is not locomotion or indefinite playback.

## First animated runtime result

Run `pictorialidletrial-20261102-212314-035.log`, PID 7684, loaded bank 1
at update 1175 and admitted motion 270 at 1242. Native cycle starts were
observed at 1242, 1357, 1472 and 1587. All sampled actor positions during
the hold remained `(-2.2475, 0.0830, 9.8510)` while animation phases advanced.
At update 1693 the original bank 0 / motion 27 resumed from saved phase 31;
the native pipeline reached phase 32 and cleared the one-shot seek.
Native phase 8 released bank 1 at update 2638, with state and loaded
character/package all 255; the log subsequently reached selector phase 1
with actor mask 0. Both isolated save-file hashes matched the pre-run baseline.

The user confirmed that the character entered the idle animation during
the hold. This confirms visible admission; smooth loop seams, foot contact,
camera interaction during this idle, and early B exit still need visual or
runtime acceptance. Do not infer those from successful native cycle logs.

The user subsequently reported a successful exit test: the character left
the idle, continued running, and the scene ended normally. This adds manual
acceptance of idle exit and resumed native scene progression. Loop seam and
foot-contact quality remain separate visual checks.
