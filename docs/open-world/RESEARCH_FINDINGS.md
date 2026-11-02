# Executed research findings

> Newer checkpoint: [TERRAIN_RESEARCH.md](TERRAIN_RESEARCH.md) documents verified Niki terrain geometry, the native height override, actor update contracts and a separately default-off grounding trial. Statements here about terrain availability or no gameplay experiment describe the earlier checkpoint.

Research date: 2026-11-01 local time (logs and captures extend into 2026-11-02 UTC). Source baseline `23026d262d7b215d30ddfa1d263fef0c3696d945`, branch `feature/open-world`, with uncommitted research changes. This report supersedes the checkout/build/runtime unknowns in the original Phase 0 snapshot. The user subsequently authorized preparation, experiments, game execution and keyboard navigation. No free-roam gameplay modification has been implemented.

## Assessment

A small Niki Beach prototype remains plausible. Actual game execution confirms a live rendered beach, animated characters, playable volleyball and normal return to location selection. This does **not** establish a standalone scene loader, terrain outside the court, or a reusable single-character controller. Those contracts must be mapped before disabling competition services or writing actor transforms.

The smallest useful code experiment was a default-off, observational scene probe. It now builds and runs. The next experiment should establish the Niki actor/resource and ground-query contracts, rather than globally removing timers or the vacation ending.

## Preparation and builds: confirmed

- Recursive SDK dependencies are present. SDK revision `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518` was prepared using the repository's existing SDK patch and setup scripts. The SDK working tree consequently contains preparation changes; its recorded submodule revision was not advanced.
- Local executable SHA-256: `C4A14853E1F7AF951BE34F380604113703638780B36D5E74E674FE7431992870`. Only the locally supplied executable was copied to ignored `game/default.xex`; runtime assets remain in the existing extraction tree. No proprietary files were downloaded or added to source control.
- Code generation succeeded, producing 249 local generated files. Guest image base is `0x82000000`, size `0x01E90000`; code begins at `0x82410000`. Generated C++ is evidence for this executable revision, not an authored engine API.
- Separate `out/open-world-baseline` and `out/open-world-probe` executables built successfully with VS 2026 18.10.4, Clang 23.1.4, Ninja and Windows SDK 10.0.26100. Existing deprecation/SDK warnings remain; no blocking compiler or linker error remained in these builds.
- Four existing host tests passed: render operation sequence, render region versions, render owner lineage and native60 phase. These tests do not validate free roam.

## Runtime evidence

Research launches use isolated `out/open-world-research/userdata`; no original user save was copied or edited. Tutorial progress and purchases during navigation affect only this experimental save namespace. Runs used the conservative original 30 FPS profile, D3D12, 1280x720. Existing native60 and alternate renderer compatibility remain untested here.

| Scenario | Observed result | Evidence / limit |
|---|---|---|
| Baseline boot and navigation | Rendered title, character selection and Day 1 location selection | Actual execution; not just compilation |
| Probe-enabled tutorial | Shops, Marine Race, Pool Hopping, Niki volleyball and hotel/day rollover executed | `probe-20261101-232907-775.log`; ignored local log |
| Niki volleyball | Four animated characters, ball, original movement and serving; normal Give Up return | Court only; no free-roam movement or ground-query test |
| Repeated Niki entry | Entered again on Day 2 Morning from normal location selection | `probe-20261102-004530-052.log`, request 11 at local 19:50:59 |
| Niki exit and calendar tracing | Request 1, normal teardown, counter 4 to 5, Day 2 Daytime location selection | Same log, local 19:54:08; calendar caller `0x826CF994` |
| Logging bound | Old probe reached its 12,000-line per-thread limit and gameplay continued | Prevents unbounded research logging; not a performance benchmark |
| Runtime probe disabled | Diagnostics-capable executable rendered the title and accepted Start; zero `OWPROBE` lines | `probedisabled-20261102-005800-001.log`; boot/navigation check, not a full activity regression |
| Terrain outside court / one actor / free camera | Not tested or implemented | Phase 1 acceptance remains outstanding |

No crash was observed in those scenarios. This is not a soak-test result or proof of absence of regressions. GPU timing output exists, but no controlled baseline/probe performance comparison has been completed.

## Verified guest integration map

All addresses below refer only to the checked local executable. Names such as “screen” and “calendar” are source/runtime annotations, not original symbols.

| Responsibility | Verified evidence | Implication / remaining unknown |
|---|---|---|
| Screen manager | Global `0x834BCFE0`: current signed-byte ID at +0, requested ID +2, state +40, frame fields +48/+52, duration +56, descriptor table pointer +68. Descriptor stride 36; update callback +4 | Identity must include table and callback; IDs are reused between tables |
| Screen request | `sub_825A15D8` takes requested ID in r3; invokes descriptor +24 during transition | Observe request and normal cleanup; do not replace it with a guessed host state |
| Screen update | `sub_825A0FD8`; initialization path `sub_825A1528` invokes descriptor +0 | Calls can yield; frame sampling is needed in addition to before/after hooks |
| Vacation location selection | Table `0x82AACFD0`, ID 1, initializer `0x826D1D98`, update `0x826D2FC8` | Verified normal return target |
| Niki volleyball | Same table, ID 11; shared initializer `0x826CF468`, update `0x826CF6A0`, descriptor +12/+24 both `0x826CF798` | Marine Race ID 23 shares this shell; ID alone is not an environment loader |
| Common game initialization | `sub_826CF468` calls `sub_826CDEF0`, prepares four character records and several fibers/camera objects, then activity dispatch `sub_824FE3A8` | Actor, environment and activity lifetimes are coupled; dependencies not fully separated |
| Activity dispatch | Selector `0x833B7454`, table `0x82BF5FD8`, stride 16; row 0 volleyball, row 1 Marine Race | Potential narrow adaptation boundary after dependencies are verified |
| Volleyball start | Row 0 +4 is `0x824FE4D0`; calls `sub_82687798`, starts fiber 4 with callback `0x82687668` | Not yet safe to omit the fiber; identify rendering/animation dependencies first |
| Volleyball cleanup | Row 0 +8 is `0x824FE4F8`, tail to `sub_826878E0`; stops fibers 4/5/6/7/9 and resets related state | Reuse normal ownership/cleanup rather than simply freezing a scene |
| Common completion | `sub_826CF798` calls `sub_826CF7C0`; activity cleanup, results and calendar advancement occur here | Blanket bypass could leak resources or skip required bookkeeping |
| Calendar | Signed byte `0x83989EA7`; UI computes day = quotient by 4 + 1 and period = remainder. `sub_826CB1A0` increments it | Runtime correlated 0..5 with displayed periods; source has comparisons to 56, consistent with 14 days, but full ending behavior is not tested |
| Niki calendar call | `sub_826CB1A0` observed caller LR `0x826CF994`, r3=1, r4=-1, r5=1, r6=0; counter 4 became 5 | A mode-local policy must preserve cleanup while suppressing progression; no calendar patch applied |
| Tutorial routing | `sub_826CC098` scans records at `0x82AB070C` and flags at `0x83989EAE`; returns a guided step or -1 | Forced tutorial navigation was not broken input. All nine flags became set in the research save |
| Player transforms | Records `0x833B8520`, stride 152; XYZ +12/+16/+20; native root delta `0x83426FD0`, stride 116; `sub_82510218` applies rotated delta | Sampled court positions all had Y=0. This does not establish terrain collision or a safe general spawn |
| Volleyball animation | `sub_8268DAE8` uses 32-byte entries at `0x82B11F68`, changes motion/frame/rate fields and calls `sub_82682590` | Native motion mechanism exists; compatible idle/walk/run IDs, root motion and independent rig lifetime remain unknown |
| Camera | Scene `0x839BC0F0`, camera selector +968, objects +928; camera data `0x839BC1D0`, XYZ +48/+52/+56 | Position observation works; writer ownership and collision query remain unknown |
| Scheduler | 24 records at `0x834A3258`, stride 28; observed flags, handle +8, callback +12 | Useful lifetime trace, not evidence that arbitrary fibers can be stopped safely |

Generated bodies are under `generated/default/`; locate definitions with `rg` or `scripts/inspect-guest-function.ps1 -Assembly`. `scripts/find-ppc-address-xrefs.py` was corrected to recognize `addi` (opcode 14) address construction as well as memory instructions. No generated body was modified.

## Asset and animation evidence

Read-only AFS metadata inspection found 62 stage entries, 756 motion entries and 88 character entries. Stage entries include `stage_vNK.tp` and `svNK_dat.bin`, plus truncated `stage_vNK2.` / `svNK2_dat.b` names. Live-image strings include `D:\dat\stage\stage_vNK\svNK_dat.bin` and `stage_vNK.tpr`, referenced by data pointers at `0x82B341B4` / `0x82B341B0`. These are leads: an actual Niki archive-load association has **not** been traced, and strings could be diagnostic remnants.

Motion metadata includes `comon.mot`, `comon_flg.mot`, `comon_pwc.mot`, character-specific motion files and bed/gravure names. This establishes filenames only, not clip contents, skeleton compatibility or portability. Relaxation scenes were not independently verified as reusable live animation. A video-mode file path investigated during I/O research was configuration, not an archive-loader discovery.

## Implemented research architecture

- `DOAX2_OPEN_WORLD_DIAGNOSTICS` is OFF by default in CMake. Only ON builds compile `src/open_world/scene_probe.cpp`.
- The independent CVar `doax2_open_world_probe` is false by default and requires restart. The run helper explicitly enables it only in Probe mode.
- Existing guest hook wrappers retain their original call and timing logic; guarded observation is added around them. Two additional hooks observe screen requests and calendar calls and call the original once.
- Probe reads are endian-aware and check mapped/readable pages; arbitrary actor/resource pointers are not followed. Mapped memory is not proof of object lifetime. State is bounded per thread, sampled once per 30 updates for transforms, and logs stop at the cap.
- No transform, input, camera, timer, schedule or save writes are introduced by the probe. It is observation tooling, not the Phase 1 controller.
- Native keyboard/screenshot helper restricts itself to a `doax2` process under this workspace's `out` directory and verifies foreground ownership. Snapshot helper uses read-only process access, verifies the launch session/executable/hash and derives arena/image bounds from runtime/generated metadata. Dumps and screenshots remain ignored.

## Next experiment and completion gates

1. Follow Niki activity initialization from `0x826CF468` through the dispatch/fiber into actual stage/resource requests and character creation. Record owner, resource lifetime and normal disposal for each required service. Compare volleyball and an available Niki relaxation scene without patching either.
2. Find the native ground/terrain query and distinguish it from court clamps, animation grounding and visual geometry. Log query arguments/results at known court and boundary positions. A guessed flat plane is not a validated beach terrain solution.
3. Map actor input assignment, animation state and rig update outside match AI. Establish one compatible idle/walk/run set and the owner of displacement; do not apply root motion twice.
4. Only after those contracts exist, implement a default-off, mode-local stationary one-actor experiment with normal entry/exit. Preserve cleanup while preventing the verified progression request in that mode. Repeated entry/exit must pass before adding camera or locomotion.

T01 is complete; T02's build/runtime observation and disabled boot/navigation check are complete, but controlled performance and disabled activity comparisons remain outstanding. T03 and T05 have concrete findings but incomplete ownership/exit contracts. T04 (ground and animation contracts) is still the principal blocker. T06-T10 and Phases 2-7 are not implemented. Keep the original dependency order; later features do not resolve these blockers.

## Reproducing research

```powershell
.\scripts\build-open-world-research.ps1 -Mode Baseline -Jobs 8
.\scripts\build-open-world-research.ps1 -Mode Probe -Jobs 8
.\scripts\run-open-world-research.ps1 -Mode Probe
# Use the ProcessId printed by the launch helper:
.\scripts\research-game-window.ps1 -ProcessId <pid> -Action Capture
.\scripts\research-game-window.ps1 -ProcessId <pid> -Action Key -Key Space
.\scripts\research-game-window.ps1 -ProcessId <pid> -Action Close
# Wait for normal process exit before another launch:
.\scripts\run-open-world-research.ps1 -Mode ProbeDisabled
```

Keyboard mapping: Space=A, Backspace=B, Return=Start, Tab=Back, WASD=left stick, IJKL=right stick, arrows=D-pad. Input uses scan codes; focus is checked before injection. A selects Yes in a horizontal confirmation. Do not automate long blind sequences. The snapshot helper is optional; its image dumps contain proprietary derived data and must stay local.

The full feasibility matrix, module proposal, risk register and later roadmap remain in the six Phase 0 documents. Their original checkout snapshot is retained for provenance; use this report for executed findings and resolved preparation blockers.
