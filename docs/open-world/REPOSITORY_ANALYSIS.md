# Repository analysis

> Current evidence: SDK preparation, generation and separate baseline/probe builds succeeded; actual game execution supplied scene, calendar and animation leads. Missing setup artifacts described below have been resolved locally. See [RESEARCH_FINDINGS.md](RESEARCH_FINDINGS.md) for verified addresses and runtime limitations. This document retains the initial checkout snapshot.

Status: static investigation, 2026-11-01. Baseline `23026d262d7b215d30ddfa1d263fef0c3696d945`.

## Checkout and prerequisites: confirmed

| Item | Observation | Consequence |
|---|---|---|
| Git | Already on `feature/open-world`; initial status only `?? INSTRUCTIONS.txt` | Use existing dedicated branch; preserve that file; do not commit automatically |
| Checkout | Not sparse; tracked integration, scripts, tests and technical notes present | Repository source is available; proprietary generated source is intentionally excluded |
| SDK | `sdk/rexglue-sdk` at `c94f5ebdcb3c9d1a460ca48e04f9758448f8d518` (`v0.10.0-2-gc94f5eb`); clean status | Do not upgrade during prototype work |
| Recursive dependencies | `git submodule status --recursive` shows initialized dependencies without missing/modified revision prefixes | Checkout presence verified; compilation completeness is untested |
| SDK patch | Forward `git apply --check` succeeds; reverse check fails; SDK has no local changes | Project runtime changes are not yet applied; forward check is not a build test |
| Game data | `extracted game files here/default.xex`, AFS archives, `dat/`, `dic/`, `movie/` and other local disc material present | No download needed; asset completeness has not been exhaustively validated |
| Supported executable | SHA-256 `c4a14853e1f7af951be34f380604113703638780b36d5e74e674fe7431992870` | Matches [README](../../README.md) supported executable; title/media metadata was not independently decoded |
| Development input path | `game/` contains its README; `game/default.xex` absent | Current codegen manifest cannot consume the release-layout input without preparation |
| Generated source | No `generated/default/` or `generated/rexglue.cmake` | Cannot inspect guest bodies/call graphs or configure the project normally yet |
| Installed SDK | No `sdk/rexglue-sdk/out/install/win-amd64/bin/rexglue.exe` | Must build/install patched SDK before generation |
| Build output | No `out/game/` or top-level build output observed | No local executable baseline to run |
| Toolchain | Visual Studio C++ discovery returns a Community installation under `Microsoft Visual Studio/18`; Python, Git and CMake commands resolve | Full Clang/Ninja/components and SDK compatibility remain unvalidated; BUILDING.md specifies VS 2022 |
| Archived research artifacts | No `archive/`; historical logs, dumps and generated sources cited in notes unavailable | Treat historical observations as documentary evidence, not reproduced measurements |

No applicable root/project `AGENTS.md` was found. The SDK's isolated third-party o1heap instructions are outside this documentation work.

## Build architecture and preparation

Direct evidence: [CMakeLists.txt](../../CMakeLists.txt), [CMakePresets.json](../../CMakePresets.json), [BUILDING.md](../BUILDING.md), [manifest](../../doax2_manifest.toml), and [scripts](../../scripts).

The project requires CMake 3.25+, C++23 and generated ReXGlue CMake integration. `doax2` compiles `src/main.cpp` and `src/doax2_60fps.cpp`, then calls `rexglue_setup_target(doax2 GPU_PLUGINS xenos)`. Windows launcher targets are native C++; the public launcher also links SDL3, GDI+, WinHTTP and bcrypt. Host-side render/timing tests exist, largely `EXCLUDE_FROM_ALL`; there are no free-roam tests. `DOAX2_FUNC_CENSUS` is an optional diagnostic definition for host and recompiled targets.

Documented sequence, **not executed in this phase**:

1. `scripts/setup-sdk.ps1`: initializes submodules, detects/applies `patches/rexglue-sdk/0001-doax2-runtime-changes.patch`, builds/installs SDK. It also changes Git long-path configuration; review this during setup.
2. Supply the supported local disc tree under ignored `game/`, preserving the release-layout originals. Alternatively investigate a local temporary manifest with an adjusted input root; do not assume changing only the XEX path handles every asset path.
3. `scripts/generate-code.ps1`: invokes installed `rexglue.exe codegen doax2_manifest.toml --ignore-stamp`, checks generated C++ for unresolved calls, writes ignored `generated/`.
4. `scripts/configure-game.ps1`: imports the VS developer environment and configures Release at `out/game` with the installed SDK prefix. Then `cmake --build out/game`.
5. For the later experiment, configure a distinct build directory, e.g. `out/open-world`, with an explicit SDK prefix and proposed diagnostic options. Existing developer launcher output goes to the repository root in CMake; a separate build directory alone does not isolate that launcher output. Build only the game target or explicitly relocate experimental launcher outputs.

`build-sdk.ps1` can rewrite a libmspack path in SDK third-party CMake for Windows symlink compatibility. All such preparation should be reviewed as setup changes, not hidden inside gameplay work. No setup was performed here.

`.gitignore` excludes game trees, generated code, saves, captures, logs and binaries. `scripts/check-personal-data.ps1` is the repository's pre-publication check. Never add local game data, derived guest C++, personal paths or save packages to these documents or commits.

## Verified integration evidence and research leads

The following addresses appear in checked-in hooks/configuration. **Their function bodies are not available in this checkout.** Descriptions of guest semantics below come from existing wrappers/comments and historical notes, and require revalidation against regenerated code and runtime traces.

| Area | Source / symbol or guest location | What the available evidence establishes / does not establish |
|---|---|---|
| Application | `src/main.cpp`, `src/doax2_app.h` | Generated `doax2_init.h` supplies image metadata; `REX_DEFINE_APP` creates `Doax2App`; `OnPostSetup` installs guest frame statistics. No gameplay-mode manager exists here |
| Host lifecycle | `sdk/rexglue-sdk/include/rex/rex_app.h` | Setup, XEX-load, dialogs, shutdown and path override hooks exist; these are application lifecycle hooks, not a known guest gameplay tick |
| Hook ABI | `sdk/rexglue-sdk/include/rex/hook.h` | `REX_HOOK_RAW` receives `PPCContext&` and guest base; project calls `__imp__sub_*` originals. Use endian-aware guest memory access and valid PPC context |
| Game loop | `src/doax2_60fps.cpp`: `sub_8258E500` | Existing wrapper tracks engine updates, timing and capture; candidate observation boundary, not automatically a safe position-write boundary |
| Screen transitions | Same file: `sub_825A0FD8`, `Native60ScreenUpdateFunction`, manager `0x834BCFE0` | Existing code reads current screen and descriptor-table update callback (36-byte descriptors, table at +68). Screen IDs and Niki Beach mapping are unknown |
| Scene update dispatch | Same file: `sub_825C19C8`, `sub_825C27C8` | Existing capture/phase probes and documented scene-specific virtual dispatch; not a mapped environment-loading API |
| Fibers | `config/fibers.toml`; `sub_8258CC00` wrapper | Scheduler comments identify 24 slots at `0x834A3258`, stride 28, current slot `0x834A34F8`; slots 0/10/14/15 are system fibers; camera slot 10. No proof scripts can be removed safely |
| Fiber runtime substitutions | `config/fibers.toml` | ConvertThreadToFiber `0x82782FF0`, ConvertFiberToThread `0x82783080`, CreateFiber `0x827830D0`, DeleteFiber `0x827831B0`, SwitchToFiber `0x82785670`; `0x82783210` thunk documented |
| Camera | `sub_824180E8`, `sub_8241A630`, `sub_8241AF08`, `sub_8241C360`, `sub_82740E08` | Existing camera probes; notes identify slot-10 update/publish path. Scene `0x839BC0F0`, selected index +968, output sources `0x839BC1D0`/`0x839BC210`. No verified orbit-camera setter or collision query |
| Character transforms | `sub_82510218` wrapper | Source identifies root-motion delta `0x83426FD0 + player*116`, player records `0x833B8520 + player*152`, position +12/+16/+20. Existing Flags smoothing touches this path; no spawn/despawn or general locomotion API established |
| Character rig / motion | `sub_82513930`, `sub_82514BC8`, `sub_825153A0`, `sub_825A4550`; volleyball `sub_8268DAE8` | Rig-update and keyframe/blend research exists; volleyball wrapper observes motion requests. Animation IDs, skeleton portability, walk/run selection and blending contract unknown |
| Beach Flags | `sub_826AC790`, `sub_826ACB20`, `sub_826AB800`, `sub_826ABE00` | Wrappers document game object and runner records, root-motion irregularity and keyframed transforms. Does not establish free steering, Niki Beach initialization, or arbitrary terrain collision |
| Volleyball | `sub_82687C20`, `sub_82687B38`, `sub_82688388`, `sub_826886D0` | Phase record `0x839528D8`, frame counter `0x839525B4`, match clock `0x839525C0`, countdown `0x839528E4` documented in wrappers. Phase handler table comment needs original-image verification before use |
| Volleyball callback | Manifest `0x82688438`, pointer-table reference `0x82AE4A24` | Explicit function-discovery repair; not a scene loader |
| Jet ski | `sub_826496A0`, `sub_82640B70`, `sub_82630458`, `sub_82643028`, `sub_82633FE0` | Race manager, countdown/controls, input-history and pole-animation leads; clock `0x839865CC`. Safe water extent, race teardown and checkpoint owners unknown |
| Timing | `sub_8250A118`, `sub_8250A568`, `sub_825C19C8` | Current comments describe base `0x83426298`, modifier `0x834262A4`, step `0x833B84CC`, shared tick `0x82E2DD44`; fixed-per-call countdowns `sub_8265E7B8` and springs `sub_825B7C08` remain separate |
| Input | SDK `src/kernel/xam/xam_input.cpp`, `src/input/input_system.cpp`, `include/rex/input/input_system.h` | `XamInputGetState_entry` routes to `InputSystem::GetState`, which reads assigned devices and merges states. Game-side processed input/cache location unknown |
| Movie distinction | `sub_829BF270` / `sub_829BD948` wrappers and local `movie/*.sfd` | Existing boot/opening movie handling and movie files are confirmed; not every relaxation scene can be classified as video or live 3D from this evidence |

## Scene-specific investigation result

**Niki Beach:** no confirmed scene identifier, initializer, asset member, ground mesh, spawn transform or shutdown function. Text searches of authored sources/docs/config/scripts did not expose a Niki Beach implementation. The available Niki Marina intro research is not proof of beach behavior.

**Beach Flags:** substantial timing/root-motion hooks exist, but their minigame-owned movement may be unsuitable for a general controller. Need to trace load, runner creation, animation ownership, boundaries, finish conditions and teardown separately.

**Volleyball:** phase transitions and motion/collision probes are available leads. Ball/hand collision in `sub_8268DCF8` and block touch tests in `sub_82690160` do not establish terrain collision. A court-bounded simulation cannot be assumed to traverse the surrounding scenery.

**Relaxation/gravure:** historical notes describe procedural/keyframe and camera changes in title/relax scenes, supporting live animated content in some sampled scenes. No scene-by-scene asset/animation catalog exists. Determine live geometry versus script versus movie for each reuse candidate; inspect actor transforms, animation evaluation, draw submission and movie status together.

**Vacation/calendar:** no verified period/day fields, rollover function, forced ending branch, autosave path or vacation-state snapshot. Activity counters above must not be labeled vacation timers. Search constants such as 14 only as leads, then verify writers/readers and observed period changes.

## Reproducible next research

After generation, use `scripts/inspect-guest-function.ps1 -Address ... -Assembly` for the documented leads, inspect callers and vtables, and record source baseline, XEX hash, register arguments, memory ownership and trace evidence for every new semantic label. `scripts/find-ppc-address-xrefs.py` needs a decoded image passed with `--image`; its default references an absent archive. `scripts/inspect-xex.py` assumes `game/default.xex` and writes metadata, so it was not used as a read-only inspector here.

Historical [timing census](../notes/60hz-simulation-time-source-census.md) and [native60 notes](../notes/native60-timing-experiment.md) describe different stages of the timing investigation. Prefer current hooks for current integration behavior, preserve historical caveats, and verify generated bodies rather than treating an older claim that a scalar has no writers as current fact.

## Validation limits and unknowns

Static file inspection, Git/submodule inspection, executable hashing and patch applicability checks only. No code generation, compiler, linker, CTest, renderer, controller or runtime tests were run. Therefore no compiler/linker errors or original-mode runtime regressions can be assessed in this phase. Full asset membership, game logic and safe FreeRoam feasibility remain unresolved until the discovery gates in the roadmap pass.
