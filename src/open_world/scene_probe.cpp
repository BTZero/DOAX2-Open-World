#include "scene_probe.h"
#include "finite_hold.h"
#include "orbit_camera.h"
#include "bounded_movement.h"
#include "exploration_terrain.h"

#include "generated/default/doax2_pch.h"

#include <array>
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <limits>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/hook.h>
#include <rex/runtime.h>
#include <rex/input/input_system.h>

REXCVAR_DEFINE_BOOL(doax2_open_world_probe, false, "DOAX2/OpenWorld",
                   "Read-only scene research trace; requires a diagnostics build")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_terrain_trial, false, "DOAX2/OpenWorld",
                   "Experimental Niki volleyball native terrain query; diagnostics build only")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_relaxation_hold_trial, false, "DOAX2/OpenWorld",
                   "Hold one verified relaxation script clock for five seconds, then release")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_unlock, false, "DOAX2/OpenWorld",
                   "Expose virtual pictorial characters and clips for research without changing ownership")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_hold_trial, false, "DOAX2/OpenWorld",
                   "Five-second script wait hold in the verified pictorial scene 58 only")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_lifetime_trial, false, "DOAX2/OpenWorld",
                   "Hold scene 58 script and outer lifetime clocks together for five seconds")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_camera_trial, false, "DOAX2/OpenWorld",
                   "Five-second coordinated scene/camera clock hold in pictorial scene 58")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_actor_trial, false, "DOAX2/OpenWorld",
                   "Five-second coordinated scene/camera/actor clock trial in scene 58")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_long_actor_trial, false, "DOAX2/OpenWorld",
                   "Fifteen-second coordinated scene/camera/actor clock trial in scene 58")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_orbit_trial, false, "DOAX2/OpenWorld",
                   "Bounded right-stick camera orbit during scene 58 fifteen-second actor hold")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_motion_research, false, "DOAX2/OpenWorld",
                   "Read-only native motion selection, track and root-displacement research")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_idle_bank_trial, false, "DOAX2/OpenWorld",
                   "Load and release a spare native Kasumi idle bank without rebinding the actor")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_idle_trial, false, "DOAX2/OpenWorld",
                   "Fifteen-second native Kasumi idle with bounded orbit and native exit")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_movement_trial, false, "DOAX2/OpenWorld",
                   "Stationary left-stick idle/jog binding test in scene 58; no displacement")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_movement_displacement, false, "DOAX2/OpenWorld",
                   "Bounded planar travel in the idle/jog trial; fixed height, no collision")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(doax2_open_world_pictorial_exploration, false, "DOAX2/OpenWorld",
                   "Five-minute grounded exploration with full camera orbit in scene 58")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REX_EXTERN(sub_8266D040);
REX_EXTERN(sub_8266D330);
REX_EXTERN(__imp__sub_825160D0);
REX_EXTERN(__imp__sub_825BEE88);

namespace doax2::open_world {
namespace {
constexpr uint32_t kScreen = 0x834BCFE0;
constexpr uint32_t kScene = 0x839BC0F0;
constexpr uint32_t kCamera = 0x839BC1D0;
constexpr uint32_t kPlayers = 0x833B8520;
constexpr uint32_t kFibers = 0x834A3258;
constexpr uint32_t kCalendar = 0x83989EA7;
constexpr uint32_t kIntroFlags = 0x83989EAE;
constexpr uint32_t kLineLimit = 12000;
constexpr std::array<uint32_t, 3> kActivities{0x82687C20, 0x826ACB20, 0x826496A0};

struct Fiber {
  uint32_t flags{}, handle{}, callback{};
  bool operator==(const Fiber&) const = default;
};
struct Screen {
  uint32_t id{}, state{}, table{}, callback{};
  bool operator==(const Screen&) const = default;
};
struct Activity {
  uint32_t argument{};
  uint64_t calls{};
  uint64_t last_update = ~uint64_t{0};
};
struct Trace {
  uint64_t frame{};
  uint32_t lines{};
  bool announced{}, have_screen{}, have_fibers{}, have_scene{};
  Screen screen{};
  std::array<Fiber, 24> fibers{};
  std::array<Activity, 3> activities{};
  uint32_t activity_mask{}, camera_index{}, objects{};
  int32_t calendar = -999;
  uint32_t intro_flags = ~0u;
  uint64_t ground_sample[2]{~uint64_t{0}, ~uint64_t{0}};
  uint64_t rig_sample[4]{~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
  bool terrain_query_trial{};
  uint32_t relaxation_phase = ~0u;
  uint32_t pictorial_phase = ~0u;
  uint32_t script_commands{};
  uint32_t camera_events{};
  std::array<uint32_t, 8> motion_identity{};
  uint32_t motion_events{};
  uint64_t motion_sample = ~uint64_t{0};
  bool have_motion{};
  uint64_t camera_sample = ~uint64_t{0};
  uint32_t pictorial_unlock_seen{};
  uint32_t pictorial_clips_seen[9]{};
  bool hold_started{}, hold_released{};
  FiniteScriptHold pictorial_hold;
  float pictorial_elapsed_anchor{};
  bool camera_hold_active{};
  uint32_t camera_hold_object{};
  double camera_hold_until{};
  uint32_t camera_hold_calls{};
  OrbitCamera orbit;
  std::array<float, 3> orbit_target_anchor{};
  bool orbit_started{}, orbit_stopped{};
  uint64_t orbit_frame = ~uint64_t{0};
  double orbit_time{};
  uint32_t motion_research_events{};
  bool idle_bank_attempted{}, idle_bank_owned{}, idle_bank_ready{};
  uint32_t idle_bank_slot = ~0u;
  bool idle_actor_active{}, idle_actor_restore{}, idle_actor_done{};
  uint32_t idle_original_motion{}, idle_original_rate{}, idle_original_step{}, idle_original_seek{};
  uint8_t idle_original_bank{}, idle_original_mirror{}, idle_original_flag73{};
  float idle_resume_phase{};
  uint32_t idle_loop_command{}, idle_loops{};
  uint32_t trial_motion = 270;
  std::vector<uint32_t> idle_variants;
  uint32_t selected_idle = 270;
  double idle_variant_time{};
  bool idle_cycle_down{};
  uint64_t movement_input_frame = ~uint64_t{0};
  uint64_t movement_root_frame = ~uint64_t{0};
  BoundedMovement movement;
  ExplorationTerrain terrain;
  uint32_t terrain_vertices{}, terrain_triangles{}, terrain_vertex_count{}, terrain_triangle_count{};
  TerrainPoint movement_pose{};
  uint32_t movement_facing{};
  bool exploration{};
  TerrainRejection contact_reason=TerrainRejection::None;
  bool contact_sliding{}, contact_blocked{};
  std::array<uint32_t, 4> movement_original_pose{};
  float movement_x{}, movement_y{};
  double movement_time{}, movement_seconds{};
  std::array<uint32_t, 4> rig_motion_identity{~0u, ~0u, ~0u, ~0u};
  std::array<uint32_t, 4> root_motion_identity{~0u, ~0u, ~0u, ~0u};
  std::array<uint64_t, 4> motion_rig_sample{~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
  std::array<uint64_t, 4> root_sample{~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}, ~uint64_t{0}};
  float hold_frame{};
  std::chrono::steady_clock::time_point hold_started_at{};
  std::chrono::steady_clock::time_point started{};
};
thread_local Trace trace;
uint32_t ActorUpdateMask(const uint8_t* base);
bool IdleActorExpected(const uint8_t* base, uint32_t motion);
void StartIdleActor(uint8_t* base);
void RestoreIdleActor(uint8_t* base);
bool IdleResourcesReady(const uint8_t* base);

bool Enabled() {
  return REXCVAR_GET(doax2_open_world_probe) && trace.lines <= kLineLimit;
}

// Admission guard for pointer-derived reads. VirtualQuery is observational;
// calls run inline with guest hooks. This checks mapped memory, not semantic
// object lifetime; no arbitrary actor/resource pointer is followed here.
bool Readable(const uint8_t* base, uint32_t address, size_t size) {
  if (!base || !size || uint64_t(address) + size > (uint64_t{1} << 32)) return false;
  const auto begin = reinterpret_cast<uintptr_t>(base) + address + REX_PHYS_HOST_OFFSET(address);
  MEMORY_BASIC_INFORMATION info{};
  if (!VirtualQuery(reinterpret_cast<const void*>(begin), &info, sizeof(info)) ||
      info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
  const DWORD access = info.Protect & 0xFF;
  if (access != PAGE_READONLY && access != PAGE_READWRITE && access != PAGE_WRITECOPY &&
      access != PAGE_EXECUTE_READ && access != PAGE_EXECUTE_READWRITE &&
      access != PAGE_EXECUTE_WRITECOPY) return false;
  const auto region_end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
  return begin <= region_end && size <= region_end - begin;
}

bool Read32(const uint8_t* base, uint32_t address, uint32_t& value) {
  if ((address & 3) || !Readable(base, address, sizeof(value))) return false;
  std::memcpy(&value, base + address + REX_PHYS_HOST_OFFSET(address), sizeof(value));
  value = std::byteswap(value);  // Xbox guest big endian, Windows host little endian.
  return true;
}

float FloatAt(const uint8_t* base, uint32_t address) {
  uint32_t bits{};
  return Read32(base, address, bits) ? std::bit_cast<float>(bits)
                                    : std::numeric_limits<float>::quiet_NaN();
}

bool TerrainTrialActive(const uint8_t* base) {
  if (!REXCVAR_GET(doax2_open_world_terrain_trial) || !Readable(base, kScreen, 1) ||
      base[kScreen] != 11) return false;
  uint32_t table{}, state{}, mode{}, predicate{}, height{}, vertices{}, triangles{};
  return Read32(base, kScreen + 68, table) && table == 0x82AACFD0 &&
      Read32(base, kScreen + 40, state) && state == 1 &&
      Read32(base, 0x833B7454, mode) && mode == 0 &&
      Read32(base, 0x82E2DB50 + 112, predicate) && predicate == 0x8268AE80 &&
      Read32(base, 0x82E2DB50 + 116, height) && height == 0x827609B8 &&
      Read32(base, 0x82E2DB50 + 40, vertices) && vertices >= 3 && vertices <= 65536 &&
      Read32(base, 0x82E2DB50 + 44, triangles) && triangles > 0 && triangles <= 131072;
}

template <typename... Args>
void Log(fmt::format_string<Args...> format, Args&&... args) {
  if (trace.lines >= kLineLimit) {
    if (trace.lines++ == kLineLimit)
      REXLOG_INFO("OWPROBE limit reached; further observations disabled for this thread");
    return;
  }
  ++trace.lines;
  REXLOG_INFO(format, std::forward<Args>(args)...);
}

void Announce() {
  if (trace.announced) return;
  trace.announced = true;
  trace.started = std::chrono::steady_clock::now();
  Log("OWPROBE v22 enabled: terrain_trial={} relaxation_hold_trial={} pictorial_unlock={} pictorial_hold_trial={} pictorial_lifetime_trial={} pictorial_camera_trial={} pictorial_actor_trial={} pictorial_long_actor_trial={} pictorial_orbit_trial={} motion_research={} idle_bank_trial={} idle_trial={} movement_binding_trial={}; raw guest identities; "
      "sample interval=30 updates line_limit={} thread={}",
      REXCVAR_GET(doax2_open_world_terrain_trial),
      REXCVAR_GET(doax2_open_world_relaxation_hold_trial),
      REXCVAR_GET(doax2_open_world_pictorial_unlock),
      REXCVAR_GET(doax2_open_world_pictorial_hold_trial),
      REXCVAR_GET(doax2_open_world_pictorial_lifetime_trial),
      REXCVAR_GET(doax2_open_world_pictorial_camera_trial),
      REXCVAR_GET(doax2_open_world_pictorial_actor_trial),
      REXCVAR_GET(doax2_open_world_pictorial_long_actor_trial),
      REXCVAR_GET(doax2_open_world_pictorial_orbit_trial),
      REXCVAR_GET(doax2_open_world_motion_research),
      REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial),
      REXCVAR_GET(doax2_open_world_pictorial_idle_trial),
      REXCVAR_GET(doax2_open_world_pictorial_movement_trial), kLineLimit, GetCurrentThreadId());
}
}  // namespace

bool OverridePictorialUnlock(uint32_t character, uint32_t caller) {
  // These two verified call sites control title-menu visibility and pictorial
  // character availability. Bypass the original predicate here: on success it
  // can cache a collection-completion bit in profile data. Other calls retain
  // native behavior, including the original native60 diagnostic wrapper.
  if (!REXCVAR_GET(doax2_open_world_pictorial_unlock) || character >= 9 ||
      (caller != 0x825A0110 && caller != 0x826EEA58)) return false;
  const uint32_t bit = 1u << (character + (caller == 0x826EEA58 ? 9 : 0));
  if (Enabled() && !(trace.pictorial_unlock_seen & bit)) {
    Announce();
    trace.pictorial_unlock_seen |= bit;
    Log("OWPROBE pictorial unlock: character={} caller={:08X}; result-only override, no ownership write",
        character, caller);
  }
  return true;
}

// The clip predicate and its callee 826EDAE8 only read discovery bits. Keep
// their original call, then override the result at the two verified list
// builders. Category-specific bounds come from the native static count table.
// No discovery, ownership or profile flags are changed.
void OverridePictorialClip(const uint8_t* base, PPCContext& ctx,
                          uint32_t character, uint32_t category,
                          uint32_t index, uint32_t caller) {
  uint32_t count{};
  constexpr uint32_t counts = uint32_t((-32085LL * 65536) - 27796);
  if (!REXCVAR_GET(doax2_open_world_pictorial_unlock) || character >= 9 || category >= 9 ||
      (caller != 0x826EE050 && caller != 0x826EE468) ||
      !Read32(base, counts + category * 4, count) || count > 256 || index >= count) return;
  ctx.r3.u64 = 1;
  const uint32_t bit = 1u << category;
  if (Enabled() && !(trace.pictorial_clips_seen[character] & bit)) {
    trace.pictorial_clips_seen[character] |= bit;
    Log("OWPROBE pictorial clip unlock: character={} category={} count={} caller={:08X}; result-only override",
        character, category, count, caller);
  }
}

void ObserveScreen(const uint8_t* base, const char* boundary) {
  if (!Enabled()) return;
  Announce();
  if (!Readable(base, kScreen, 72)) return;
  Screen screen{};
  // The screen ID is a signed byte in the existing screen-manager wrapper.
  screen.id = base[kScreen];
  if (!Read32(base, kScreen + 40, screen.state) ||
      !Read32(base, kScreen + 68, screen.table)) return;
  const uint64_t callback_address = uint64_t(screen.table) + screen.id * 36 + 4;
  if (screen.id < 128 && screen.table && callback_address <= UINT32_MAX)
    Read32(base, uint32_t(callback_address), screen.callback);
  if (trace.have_screen && screen == trace.screen) return;
  const bool descriptor_changed = !trace.have_screen || screen.id != trace.screen.id ||
                                  screen.table != trace.screen.table;
  uint32_t frames{}, duration{}, fiber{};
  Read32(base, kScreen + 52, frames);
  Read32(base, kScreen + 56, duration);
  Read32(base, 0x834A34F8, fiber);
  Log("OWPROBE screen: update={} boundary={} id={} state={} table={:08X} "
      "callback={:08X} frames={} duration={} fiber={}", trace.frame,
      boundary, int32_t(int8_t(screen.id)), screen.state,
      screen.table, screen.callback, frames, duration, int32_t(fiber));
  if (descriptor_changed && screen.id < 128 && screen.table) {
    const uint64_t descriptor = uint64_t(screen.table) + screen.id * 36;
    uint32_t at0{}, at12{}, at16{}, at24{}, at32{};
    if (descriptor + 36 <= (uint64_t{1} << 32) &&
        Read32(base, uint32_t(descriptor), at0) &&
        Read32(base, uint32_t(descriptor + 12), at12) &&
        Read32(base, uint32_t(descriptor + 16), at16) &&
        Read32(base, uint32_t(descriptor + 24), at24) &&
        Read32(base, uint32_t(descriptor + 32), at32))
      Log("OWPROBE screen descriptor: update={} id={} at0={:08X} at12={:08X} "
          "at16={:08X} at24={:08X} parameter={:08X}; callback roles need verification",
          trace.frame, screen.id, at0, at12, at16, at24, at32);
  }
  trace.screen = screen;
  trace.have_screen = true;
}

void ObserveScheduler(const uint8_t* base) {
  if (!Enabled()) return;
  Announce();
  for (uint32_t i = 0; i < trace.fibers.size(); ++i) {
    const uint32_t slot = kFibers + i * 28;
    Fiber next{};
    if (!Read32(base, slot, next.flags) || !Read32(base, slot + 8, next.handle) ||
        !Read32(base, slot + 12, next.callback)) continue;
    if (trace.have_fibers && next == trace.fibers[i]) continue;
    uint32_t wait{};
    Read32(base, slot + 4, wait);
    Log("OWPROBE fiber: update={} slot={} flags={:08X} handle={:08X} "
        "callback={:08X} wait={}", trace.frame, i, next.flags, next.handle,
        next.callback, int32_t(wait));
    trace.fibers[i] = next;
  }
  trace.have_fibers = true;
}

void ObserveActivity(uint32_t function, uint32_t argument) {
  if (!Enabled()) return;
  Announce();
  for (size_t i = 0; i < kActivities.size(); ++i) {
    if (function != kActivities[i]) continue;
    trace.activities[i].argument = argument;
    ++trace.activities[i].calls;
    trace.activities[i].last_update = trace.frame;
    return;
  }
}

void ObserveFrame(const uint8_t* base) {
  if (!Enabled()) return;
  Announce();
  ++trace.frame;
  // Some screen initializers yield while a UI is active. Observe from the
  // independent frame boundary too, without waiting for that callback to return.
  ObserveScreen(base, "frame");
  // Title screen 2 retains its descriptor during pictorial playback. Its
  // nested controller 826F0540 owns phases 4..10 for scene load/play/return.
  // These fixed fields were checked against a live phase-7 image snapshot.
  if (trace.screen.id == 2 && trace.screen.table == 0x82B83C80 &&
      trace.screen.callback == 0x825A2270) {
    uint32_t phase{}, selected{}, scene_id{}, camera_index{};
    constexpr uint32_t state = 0x83DA3D98;
    if (Read32(base, state + 89788, phase) && phase <= 11 &&
        Read32(base, state + 89140, selected)) {
      const bool have_scene = phase >= 4 && phase <= 10 &&
          selected >= state && uint64_t(selected) + 4 <= uint64_t(state) + 89788 &&
          Read32(base, selected, scene_id);
      if (phase != trace.pictorial_phase || trace.frame % 30 == 0) {
        if (phase == 4 && phase != trace.pictorial_phase) {
          trace.script_commands = 0;
          trace.camera_events = 0;
          trace.motion_events = 0;
          trace.have_motion = false;
          trace.motion_sample = ~uint64_t{0};
          trace.camera_sample = ~uint64_t{0};
        }
        uint32_t actor_mask{};
        for (uint32_t slot = 0; slot < 4; ++slot) {
          if (Readable(base, kPlayers + slot * 152 + 10, 1) &&
              base[kPlayers + slot * 152 + 10]) actor_mask |= 1u << slot;
        }
        Read32(base, kScene + 968, camera_index);
        Log("OWPROBE pictorial: update={} screen_state={} phase={} selected={:08X} "
            "scene_valid={} scene_id={} script_frame={:.3f} actor_update_mask={:X} camera_index={} script_elapsed={:.3f}",
            trace.frame, trace.screen.state, phase, selected, have_scene,
            have_scene ? scene_id : ~0u, FloatAt(base, 0x833B4C68 + 28), actor_mask, camera_index,
            FloatAt(base, 0x833B4C68 + 24));
      }
      trace.pictorial_phase = phase;
    }
  } else {
    trace.pictorial_phase = ~0u;
  }
  // 826E2618 -> 826E2EE8. Observe its native state machine, never hold or
  // advance it. The same numeric screen ID in another table is unrelated.
  if (trace.screen.id == 7 && trace.screen.table == 0x82AACFD0 &&
      trace.screen.callback == 0x826E2618) {
    constexpr uint32_t state = 0x83975920;
    constexpr uint32_t script = 0x833B4C68;
    uint32_t parameter{}, phase{}, duration{}, sound{};
    if (Read32(base, 0x82AACFD0 + 7 * 36 + 32, parameter) && parameter == state &&
        Read32(base, state, phase) && Read32(base, state + 8, duration) &&
        Read32(base, state + 28, sound) && Readable(base, state + 24, 2)) {
      if (phase != trace.relaxation_phase || trace.frame % 30 == 0) {
        uint32_t enabled_slots{}, readable_slots{}, camera_index = ~0u;
        Read32(base, kScene + 968, camera_index);
        for (uint32_t i = 0; i < 4; ++i) {
          const uint32_t flag = kPlayers + i * 152 + 10;
          if (Readable(base, flag, 1)) {
            readable_slots |= 1u << i;
            if (base[flag]) enabled_slots |= 1u << i;
          }
        }
        Log("OWPROBE relaxation: update={} screen_state={} phase={} "
            "part={} parts={} duration={} script_frame={:.3f} sound={} "
            "actor_update_mask={:X} readable_mask={:X} camera_index={}",
            trace.frame, trace.screen.state, phase, base[state + 24],
            base[state + 25], int32_t(duration), FloatAt(base, script + 28),
            int32_t(sound), enabled_slots, readable_slots, camera_index);
      }
      trace.relaxation_phase = phase;
    }
  } else {
    trace.relaxation_phase = ~0u;
  }
  if (Readable(base, kCalendar, 1) && Readable(base, kIntroFlags, 9)) {
    const int32_t calendar = int8_t(base[kCalendar]);
    uint32_t flags{};
    for (uint32_t i = 0; i < 9; ++i) if (base[kIntroFlags + i]) flags |= 1u << i;
    if (calendar != trace.calendar || flags != trace.intro_flags) {
      Log("OWPROBE calendar: update={} period_counter={} intro_flags={:03X}; "
          "UI day/period and end-of-vacation behavior require correlation",
          trace.frame, calendar, flags);
      trace.calendar = calendar;
      trace.intro_flags = flags;
    }
  }
  uint32_t mask{};
  for (size_t i = 0; i < trace.activities.size(); ++i)
    if (trace.activities[i].calls) mask |= 1u << i;
  if (mask != trace.activity_mask) {
    Log("OWPROBE activity calls: update={} mask={} (1=volleyball 2=flags 4=jet); "
        "call presence only, not a location ID", trace.frame, mask);
    trace.activity_mask = mask;
  }
  uint32_t index{}, objects{};
  if (Read32(base, kScene + 968, index) && Read32(base, kScene + 928, objects) &&
      (!trace.have_scene || index != trace.camera_index || objects != trace.objects)) {
    Log("OWPROBE camera selection: update={} index={} objects={:08X}",
        trace.frame, index, objects);
    trace.camera_index = index;
    trace.objects = objects;
    trace.have_scene = true;
  }
  if (trace.frame == 1 || trace.frame % 30 == 0) {
    const double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - trace.started).count();
    Log("OWPROBE sample: update={} elapsed={:.3f}s camera_xyz=({:.4f},{:.4f},{:.4f}) "
        "activity_r3=({:08X},{:08X},{:08X}) calls_this_update=({},{},{})",
        trace.frame, elapsed, FloatAt(base, kCamera + 48), FloatAt(base, kCamera + 52),
        FloatAt(base, kCamera + 56), trace.activities[0].argument,
        trace.activities[1].argument, trace.activities[2].argument,
        trace.activities[0].calls, trace.activities[1].calls, trace.activities[2].calls);
    for (uint32_t i = 0; i < 4; ++i) {
      const uint32_t record = kPlayers + i * 152;
      Log("OWPROBE player_record: update={} slot={} address={:08X} "
          "xyz=({:.4f},{:.4f},{:.4f}); slot occupancy unverified", trace.frame, i,
          record, FloatAt(base, record + 12), FloatAt(base, record + 16),
          FloatAt(base, record + 20));
    }
  }
  for (auto& activity : trace.activities) activity.calls = 0;
}
}  // namespace doax2::open_world

namespace doax2::open_world {
namespace {
constexpr uint32_t kMotionManager = 0x839472D8;
constexpr uint32_t kMotionBanks = 0x833B87A8;
bool ReferencePictorial(const uint8_t* base, uint32_t& phase) {
  uint32_t table{}, state{}, record{}, scene{};
  return Readable(base, kScreen, 1) && base[kScreen] == 2 &&
      Read32(base, kScreen + 68, table) && table == 0x82B83C80 &&
      Read32(base, kScreen + 40, state) && state == 1 &&
      Read32(base, 0x83DB99CC, record) && record == 0x83DA61BC &&
      Read32(base, record, scene) && scene == 58 &&
      Read32(base, 0x83DB9C54, phase);
}
bool BankBoundToActiveActor(const uint8_t* base, uint32_t slot) {
  for (uint32_t i = 0; i < 4; ++i) {
    const uint32_t player = kPlayers + i * 152;
    if (!Readable(base, player, 152) || (base[player + 10] && base[player + 4] == slot)) return true;
  }
  return false;
}
bool IdleActorExpected(const uint8_t* base, uint32_t motion) {
  return motion == 27 || (trace.idle_actor_active && motion == trace.trial_motion &&
      Readable(base, kPlayers, 152) && base[kPlayers + 4] == trace.idle_bank_slot);
}
bool IdleResourcesReady(const uint8_t* base) {
  if (!trace.idle_bank_ready || trace.idle_bank_slot >= 4) return false;
  uint32_t clips{}, clip{}, header{}, wrapper{}, scripts{}, command{}, first{}, second{};
  uint32_t shared{}, shared_scripts{}, loop{}, loop_head{}, loop_middle{}, loop_tail{};
  return Read32(base, kMotionBanks + 20 + trace.idle_bank_slot * 64, clips) &&
      Read32(base, clips + 270 * 4, clip) && Read32(base, clip, header) && header == 0x003C0073 &&
      Read32(base, kMotionBanks + 16 + trace.idle_bank_slot * 64, wrapper) &&
      Read32(base, wrapper, scripts) && Read32(base, scripts + 270 * 4, command) &&
      Read32(base, command, first) && first == 0x0000010E &&
      Read32(base, command + 4, second) && second == 0x00010002 &&
      Read32(base, kMotionBanks + 4, shared) && Read32(base, shared, shared_scripts) &&
      Read32(base, shared_scripts + 4, loop) && Read32(base, loop, loop_head) && loop_head == 0x00002001 &&
      Read32(base, loop + 4, loop_middle) && loop_middle == 0x00010002 &&
      Read32(base, loop + 8, loop_tail) && loop_tail == 0x000A0003 &&
      Readable(base, loop + 12, 2) && base[loop + 12] == 0 && base[loop + 13] == 255;
}
bool IdleVariantReady(const uint8_t* base, uint32_t id, uint32_t& frames) {
  uint32_t clips{},clip{},header{},wrapper{},scripts{},command{},first{},second{};
  if (!Read32(base,kMotionBanks+20+trace.idle_bank_slot*64,clips) ||
      !Read32(base,clips+id*4,clip) || !Read32(base,clip,header) ||
      header >> 16 != 60 || (header & 65535) <= 5 || (header & 65535) > 4096 ||
      !Read32(base,kMotionBanks+16+trace.idle_bank_slot*64,wrapper) ||
      !Read32(base,wrapper,scripts) || !Read32(base,scripts+id*4,command) ||
      !Read32(base,command,first) || first != id ||
      !Read32(base,command+4,second) || second != 0x00010002) return false;
  frames=header & 65535;
  return true;
}
bool JogResourcesReady(const uint8_t* base) {
  uint32_t clips{}, clip{}, header{}, wrapper{}, scripts{}, command{}, first{}, second{};
  // Shared motion 8223 uses index 31 independently of the character bank.
  // Reject missing resources and the five-frame fallback clip.
  return Read32(base, kMotionBanks + 8, clips) &&
      Read32(base, clips + 31 * 4, clip) && Read32(base, clip, header) &&
      (header >> 16) == 60 && (header & 0xFFFF) == 24 &&
      Read32(base, kMotionBanks + 4, wrapper) && Read32(base, wrapper, scripts) &&
      Read32(base, scripts + 31 * 4, command) &&
      Read32(base, command, first) && first == 0x0000201F &&
      Read32(base, command + 4, second) && second == 0x00010002;
}
void UpdateMovementBinding(uint8_t* base) {
  if (!REXCVAR_GET(doax2_open_world_pictorial_movement_trial) ||
      !trace.idle_actor_active || trace.movement_input_frame == trace.frame) return;
  trace.movement_input_frame = trace.frame;
  const double now = std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  trace.movement_seconds = std::clamp(now - trace.movement_time, 0.0, 0.05);
  trace.movement_time = now;
  float x = 0, y = 0;
  bool cycle_down=false;
  DWORD foreground_process{};
  GetWindowThreadProcessId(GetForegroundWindow(), &foreground_process);
  if (auto* runtime = rex::Runtime::instance();
      runtime && foreground_process == GetCurrentProcessId()) {
    if (auto* input = dynamic_cast<rex::input::InputSystem*>(runtime->input_system())) {
      rex::input::X_INPUT_STATE pad{};
      if (input->GetState(0, &pad) == 0 &&
          !(uint16_t(pad.gamepad.buttons) & (rex::input::X_INPUT_GAMEPAD_B |
              rex::input::X_INPUT_GAMEPAD_START | rex::input::X_INPUT_GAMEPAD_BACK))) {
        const auto normalize = [](int16_t v) { return float(v) / (v < 0 ? 32768.0f : 32767.0f); };
        x = normalize(int16_t(pad.gamepad.thumb_lx));
        y = normalize(int16_t(pad.gamepad.thumb_ly));
        cycle_down = (uint16_t(pad.gamepad.buttons) & rex::input::X_INPUT_GAMEPAD_X) != 0;
      }
    }
  }
  trace.movement_x = x;
  trace.movement_y = y;
  const bool cycle_pressed=cycle_down && !trace.idle_cycle_down;
  trace.idle_cycle_down=cycle_down;
  // Hysteresis avoids repeated rebinds near the radial deadzone. Travel
  // additionally waits for the native jog binder and the orbit to be ready.
  const float magnitude = std::min(1.0f, std::hypot(x, y));
  const bool moving=magnitude > (trace.trial_motion == 8223 ? 0.18f : 0.25f);
  if (!moving && trace.trial_motion != 8223 && !base[kPlayers+56] && trace.idle_variants.size()>1) {
    uint32_t frames{},native{};
    const bool near_end=IdleVariantReady(base,trace.selected_idle,frames) &&
        Read32(base,0x834271A0+108,native) && native >> 16 == trace.selected_idle &&
        FloatAt(base,0x834271A0+52) >= float(frames)-2;
    if (cycle_pressed || (now-trace.idle_variant_time >= 8 && near_end)) {
      const auto found=std::find(trace.idle_variants.begin(),trace.idle_variants.end(),trace.selected_idle);
      const size_t index=found == trace.idle_variants.end() ? 0 : size_t(found-trace.idle_variants.begin());
      trace.selected_idle=trace.idle_variants[(index+1)%trace.idle_variants.size()];
    }
  }
  const uint32_t desired = moving ? 8223 : trace.selected_idle;
  if (desired == trace.trial_motion || base[kPlayers + 56]) return;
  if (desired == 8223 && !JogResourcesReady(base)) {
    RestoreIdleActor(base);
    Log("OWPROBE movement binding: rejected update={}; shared jog resource mismatch", trace.frame);
    return;
  }
  uint32_t idle_frames{};
  if (desired != 8223 && !IdleVariantReady(base,desired,idle_frames)) {
    RestoreIdleActor(base);
    Log("OWPROBE idle variant: rejected update={} motion={}; resource changed",trace.frame,desired);
    return;
  }
  uint32_t pair{};
  if (!Read32(base, kPlayers + 52, pair)) return;
  trace.trial_motion = desired;
  trace.idle_variant_time=now;
  REX_STORE_U32(kPlayers + 52, (desired << 16) | (pair & 0xFFFF));
  base[kPlayers + 56] = 1;
  base[kPlayers + 60] = 0;
  base[kPlayers + 61] = 0;
  Log("OWPROBE movement binding: request update={} motion={} stick=({:.3f},{:.3f}) magnitude={:.3f} displacement={}",
      trace.frame, desired, x, y, magnitude, trace.movement.active);
}
bool ExplorationGeometryMatches(const uint8_t* base) {
  uint32_t vtable{}, vertices{}, triangles{}, vc{}, tc{};
  return Read32(base, 0x82E2DB50, vtable) && vtable == 0x823BFD30 &&
      Read32(base, 0x82E2DB50 + 20, vertices) && vertices == trace.terrain_vertices &&
      Read32(base, 0x82E2DB50 + 24, triangles) && triangles == trace.terrain_triangles &&
      Read32(base, 0x82E2DB50 + 40, vc) && vc == trace.terrain_vertex_count &&
      Read32(base, 0x82E2DB50 + 44, tc) && tc == trace.terrain_triangle_count;
}
bool LoadExplorationTerrain(const uint8_t* base) {
  trace.terrain = {};
  uint32_t vtable{};
  if (!Read32(base, 0x82E2DB50, vtable) || vtable != 0x823BFD30 ||
      !Read32(base, 0x82E2DB50 + 20, trace.terrain_vertices) ||
      !Read32(base, 0x82E2DB50 + 24, trace.terrain_triangles) ||
      !Read32(base, 0x82E2DB50 + 40, trace.terrain_vertex_count) ||
      !Read32(base, 0x82E2DB50 + 44, trace.terrain_triangle_count) ||
      !trace.terrain_vertex_count || trace.terrain_vertex_count > 8192 ||
      !trace.terrain_triangle_count || trace.terrain_triangle_count > 16384 ||
      !Readable(base, trace.terrain_vertices, trace.terrain_vertex_count * 16) ||
      !Readable(base, trace.terrain_triangles, trace.terrain_triangle_count * 8)) return false;
  std::vector<TerrainPoint> vertices;
  for (uint32_t i = 0; i < trace.terrain_vertex_count; ++i) {
    TerrainPoint vertex;
    for (uint32_t axis = 0; axis < 3; ++axis) {
      vertex[axis] = FloatAt(base, trace.terrain_vertices + i * 16 + axis * 4);
      if (!std::isfinite(vertex[axis]) || std::abs(vertex[axis]) > 10000) return false;
    }
    vertices.push_back(vertex);
  }
  for (uint32_t i = 0; i < trace.terrain_triangle_count; ++i) {
    uint32_t first{}, second{};
    if (!Read32(base, trace.terrain_triangles + i * 8, first) ||
        !Read32(base, trace.terrain_triangles + i * 8 + 4, second)) return false;
    const uint32_t a = first >> 16, b = first & 65535, c = second >> 16;
    if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size()) return false;
    trace.terrain.triangles.push_back({vertices[a],vertices[b],vertices[c],uint16_t(second)});
  }
  TerrainSurface spawn;
  if (!trace.terrain.Surface(trace.movement_pose[0], trace.movement_pose[2], trace.movement_pose[1], spawn) ||
      std::abs(spawn.height - trace.movement_pose[1]) > .15f || spawn.slope > .6f) return false;
  trace.terrain.admitted_flags = spawn.flags;
  // Cover all loaded terrain, including material seams and lower ground.
  // Footprint, slope, discontinuity, native-height and obstruction gates remain.
  trace.terrain.match_flags = false;
  trace.terrain.minimum_y = spawn.height;
  trace.movement.radius = BoundedMovement::kRadius;
  for (const auto& triangle : trace.terrain.triangles) {
    for (const auto& vertex : {triangle.a,triangle.b,triangle.c}) {
      trace.movement.radius = std::max(trace.movement.radius,
          std::hypot(vertex[0]-trace.movement.anchor_x,vertex[2]-trace.movement.anchor_z)+.001f);
      trace.terrain.minimum_y = std::min(trace.terrain.minimum_y,vertex[1]);
    }
  }
  if (trace.movement.radius > BoundedMovement::kMaximumRadius) return false;
  float height{};
  return trace.terrain.Supported(trace.movement_pose[0], trace.movement_pose[2], spawn.height, height);
}
void StartIdleActor(uint8_t* base) {
  if ((!REXCVAR_GET(doax2_open_world_pictorial_idle_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_movement_trial)) || trace.idle_actor_done ||
      trace.idle_actor_active || !IdleResourcesReady(base) ||
      !Readable(base, kPlayers, 152)) return;
  uint32_t shared{}, scripts{}, loop{};
  if (!Read32(base, kPlayers + 52, trace.idle_original_motion) ||
      trace.idle_original_motion >> 16 != 27 ||
      !Read32(base, kPlayers + 48, trace.idle_original_rate) ||
      !Read32(base, kPlayers + 44, trace.idle_original_step) ||
      !Read32(base, kPlayers + 116, trace.idle_original_seek) || trace.idle_original_seek ||
      !Read32(base, kMotionBanks + 4, shared) || !Read32(base, shared, scripts) ||
      !Read32(base, scripts + 4, loop)) return;
  trace.idle_resume_phase = FloatAt(base, 0x834271A0 + 52);
  if (!std::isfinite(trace.idle_resume_phase) || trace.idle_resume_phase < 1 || trace.idle_resume_phase > 120) return;
  trace.idle_original_bank = base[kPlayers + 4];
  trace.idle_original_mirror = base[kPlayers + 58];
  trace.idle_original_flag73 = base[kPlayers + 73];
  trace.idle_loop_command = loop + 10;
  if (REXCVAR_GET(doax2_open_world_pictorial_movement_trial) &&
      REXCVAR_GET(doax2_open_world_pictorial_movement_displacement)) {
    for (uint32_t i = 0; i < 4; ++i)
      if (!Read32(base, kPlayers + 12 + i * 4, trace.movement_original_pose[i])) return;
    if (!std::isfinite(std::bit_cast<float>(trace.movement_original_pose[1])) ||
        !trace.movement.Begin(std::bit_cast<float>(trace.movement_original_pose[0]),
            std::bit_cast<float>(trace.movement_original_pose[2]), trace.movement_original_pose[3])) return;
    trace.movement_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    trace.idle_variant_time=trace.movement_time;
    trace.movement_pose = {FloatAt(base,kPlayers+12),FloatAt(base,kPlayers+16),FloatAt(base,kPlayers+20)};
    trace.movement_facing = trace.movement_original_pose[3];
    trace.exploration = REXCVAR_GET(doax2_open_world_pictorial_exploration);
    if (trace.exploration) {
      if (!LoadExplorationTerrain(base)) {
        trace.movement.active = false;
        Log("OWPROBE exploration: rejected update={}; terrain admission failed", trace.frame);
        return;
      }
      Log("OWPROBE exploration: admitted update={} vertices={} triangles={} flags={} minimum_y={:.4f} radius={:.4f} duration=300",
          trace.frame, trace.terrain_vertex_count, trace.terrain_triangle_count,
          trace.terrain.admitted_flags, trace.terrain.minimum_y, trace.movement.radius);
    }
    Log("OWPROBE movement: begin update={} anchor=({:.4f},{:.4f},{:.4f}) facing={} radius={} speed={} exploration={}",
        trace.frame, FloatAt(base, kPlayers + 12), FloatAt(base, kPlayers + 16), FloatAt(base, kPlayers + 20),
        trace.movement_original_pose[3], trace.movement.radius, BoundedMovement::kSpeed, trace.exploration);
  }
  trace.idle_actor_active = true;
  base[kPlayers + 4] = uint8_t(trace.idle_bank_slot);
  REX_STORE_U32(kPlayers + 52, (270u << 16) | (trace.idle_original_motion & 0xFFFF));
  REX_STORE_U32(kPlayers + 48, std::bit_cast<uint32_t>(1.0f));
  REX_STORE_U32(kPlayers + 44, std::bit_cast<uint32_t>(1.0f));
  base[kPlayers + 56] = 1;  // Native primary-motion request, as in 826923A0.
  base[kPlayers + 58] = 0;
  base[kPlayers + 60] = 0;
  base[kPlayers + 61] = 0;
  Log("OWPROBE idle actor: begin update={} bank={} motion=270 resume_bank={} resume_phase={:.3f}",
      trace.frame, trace.idle_bank_slot, trace.idle_original_bank, trace.idle_resume_phase);
}
void RestoreIdleActor(uint8_t* base) {
  if (!trace.idle_actor_active) return;
  trace.idle_actor_active = false;
  trace.idle_actor_done = true;
  uint32_t motion{};
  if (!Readable(base, kPlayers, 152) || base[kPlayers + 4] != trace.idle_bank_slot ||
      !Read32(base, kPlayers + 52, motion) || motion >> 16 != trace.trial_motion) {
    trace.movement.active = false;
    Log("OWPROBE idle actor: ownership changed update={}; native actor transition retained", trace.frame);
    return;
  }
  if (trace.movement.active) {
    uint32_t phase{};
    if (ReferencePictorial(base, phase)) {
      for (uint32_t i = 0; i < 4; ++i)
        REX_STORE_U32(kPlayers + 12 + i * 4, trace.movement_original_pose[i]);
      Log("OWPROBE movement: pose restored update={} xyz=({:.4f},{:.4f},{:.4f}) facing={}",
          trace.frame, FloatAt(base, kPlayers + 12), FloatAt(base, kPlayers + 16),
          FloatAt(base, kPlayers + 20), trace.movement_original_pose[3]);
    } else {
      Log("OWPROBE movement: pose restoration skipped update={}; scene ownership changed", trace.frame);
    }
    trace.movement.active = false;
  }
  // Request the original clip through the native binder with its supported
  // one-shot seek. Never restore stale track/resource pointers.
  trace.idle_actor_restore = true;
  base[kPlayers + 4] = trace.idle_original_bank;
  REX_STORE_U32(kPlayers + 52, trace.idle_original_motion);
  REX_STORE_U32(kPlayers + 48, trace.idle_original_rate);
  REX_STORE_U32(kPlayers + 44, trace.idle_original_step);
  REX_STORE_U32(kPlayers + 116, std::bit_cast<uint32_t>(trace.idle_resume_phase));
  base[kPlayers + 56] = 1;
  base[kPlayers + 58] = trace.idle_original_mirror;
  base[kPlayers + 60] = 0;
  base[kPlayers + 61] = 0;
  base[kPlayers + 73] = trace.idle_original_flag73;
  Log("OWPROBE idle actor: restore requested update={} bank={} motion=27 phase={:.3f} loops={}",
      trace.frame, trace.idle_original_bank, trace.idle_resume_phase, trace.idle_loops);
}
void RequestIdleBank(uint8_t* base, PPCContext& ctx) {
  if (!REXCVAR_GET(doax2_open_world_probe) ||
      (!REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_movement_trial)) || trace.idle_bank_attempted) return;
  uint32_t phase{}, package_list{}, package{};
  if (!ReferencePictorial(base, phase) || phase != 4 ||
      !Read32(base, 0x82B2AC00 + 5 * 8, package_list) || package_list != 0x82B29F78 ||
      !Read32(base, package_list, package) || package != 0x01BE01BF) return;
  trace.idle_bank_attempted = true;
  // Only claim an entirely empty native slot. Never evict selector caches
  // or change the bank of an actor in this resource-only experiment.
  for (uint32_t slot = 1; slot < 4; ++slot) {
    const uint32_t state = kMotionManager + 200 + slot * 16;
    uint32_t script_handle{}, clip_handle{};
    if (!Readable(base, state, 16) || base[state] != 255 || base[state + 2] ||
        base[state + 3] != 255 || base[state + 4] != 255 ||
        base[state + 6] != 255 || base[state + 7] != 255 ||
        BankBoundToActiveActor(base, slot) ||
        !Read32(base, kMotionBanks + 16 + slot * 64, script_handle) ||
        !Read32(base, kMotionBanks + 20 + slot * 64, clip_handle) ||
        !script_handle || !clip_handle || !Readable(base, script_handle, 4) ||
        !Readable(base, clip_handle, 4)) continue;
    auto call = ctx;
    call.r3.u64 = slot;
    call.r4.u64 = 5;  // Verified reference Kasumi variant.
    call.r5.u64 = 0;  // Native default package at 82B29F78: assets 446/447.
    call.r6.u64 = 255;
    sub_8266D040(call, base);
    trace.idle_bank_owned = call.r3.u32 == 1;
    trace.idle_bank_slot = slot;
    Log("OWPROBE idle bank: request update={} slot={} accepted={} character=5 package=0 actor_bank={}",
        trace.frame, slot, trace.idle_bank_owned, base[kPlayers + 4]);
    return;
  }
  Log("OWPROBE idle bank: declined update={}; no entirely empty unbound bank", trace.frame);
}
}  // namespace

void MaintainIdleBank(uint8_t* base, PPCContext& ctx) {
  if (!trace.idle_bank_owned) return;
  const uint32_t slot = trace.idle_bank_slot;
  const uint32_t state = kMotionManager + 200 + slot * 16;
  if (!Readable(base, state, 16)) return;
  const bool loaded = base[state] == 255 && base[state + 6] == 5 && base[state + 7] == 0;
  if (loaded && !trace.idle_bank_ready) {
    trace.idle_bank_ready = true;
    uint32_t clips{};
    Read32(base, kMotionBanks + 20 + slot * 64, clips);
    for (uint32_t id : {270u,271u,272u,273u,274u,275u,541u,542u,543u,544u,545u,546u,547u}) {
      uint32_t frames{};
      const bool accepted=IdleVariantReady(base,id,frames);
      if (accepted) trace.idle_variants.push_back(id);
      Log("OWPROBE idle variant: catalog update={} motion={} accepted={} frames={}",trace.frame,id,accepted,frames);
    }
    Log("OWPROBE idle bank: ready update={} slot={} clips={:08X}; actor binding unchanged", trace.frame, slot, clips);
    for (uint32_t id = 270; id <= 275; ++id) {
      uint32_t clip{}, header{};
      const bool valid = clips && Read32(base, clips + id * 4, clip) && clip && Read32(base, clip, header);
      Log("OWPROBE idle bank: clip update={} slot={} id={} valid={} pointer={:08X} header={:08X}",
          trace.frame, slot, id, valid, clip, header);
    }
  }
  uint32_t phase{};
  const bool reference = ReferencePictorial(base, phase);
  if (trace.idle_actor_active && (!reference || phase != 7 || !trace.camera_hold_active)) RestoreIdleActor(base);
  if (trace.idle_actor_restore && ActorUpdateMask(base) == 0 && Readable(base, kPlayers, 152) &&
      base[kPlayers + 4] == trace.idle_original_bank) {
    REX_STORE_U32(kPlayers + 116, trace.idle_original_seek);
    trace.idle_actor_restore = false;
    Log("OWPROBE idle actor: inactive cleanup update={}; no active track uses spare bank", trace.frame);
  }
  // Let the native asynchronous request complete before releasing it. The
  // actor never uses this bank, so release cannot invalidate its rig tracks.
  if (!loaded || trace.idle_actor_active || trace.idle_actor_restore ||
      (reference && phase >= 4 && phase <= 7) || BankBoundToActiveActor(base, slot)) return;
  auto call = ctx;
  call.r3.u64 = slot;
  sub_8266D330(call, base);
  trace.idle_bank_owned = false;
  Log("OWPROBE idle bank: released update={} slot={} phase={} state={} loaded_character={} loaded_package={}",
      trace.frame, slot, phase, base[state], base[state + 6], base[state + 7]);
}
IdleUpdateScope::IdleUpdateScope(uint8_t* base, uint32_t slot) {
  if (slot != 0) return;
  if (trace.idle_actor_active) {
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    uint32_t phase{}, motion{};
    if (!trace.camera_hold_active || now >= trace.camera_hold_until ||
        !ReferencePictorial(base, phase) || phase != 7 ||
        !Read32(base, kPlayers + 52, motion) || motion >> 16 != trace.trial_motion ||
        base[kPlayers + 4] != trace.idle_bank_slot) RestoreIdleActor(base);
  }
  if (trace.idle_actor_active) UpdateMovementBinding(base);
  if (trace.idle_actor_restore) base_ = base;
}
IdleUpdateScope::~IdleUpdateScope() {
  if (!base_ || !trace.idle_actor_restore) return;
  uint32_t native_motion{};
  if (!Read32(base_, 0x834271A0 + 108, native_motion) || native_motion >> 16 != 27 ||
      base_[kPlayers + 4] != trace.idle_original_bank || base_[kPlayers + 56] ||
      FloatAt(base_, 0x834271A0 + 52) < trace.idle_resume_phase) return;
  auto* base = base_;
  REX_STORE_U32(kPlayers + 116, trace.idle_original_seek);
  trace.idle_actor_restore = false;
  Log("OWPROBE idle actor: restored update={} native_phase={:.3f}; one-shot seek cleared", trace.frame,
      FloatAt(base, 0x834271A0 + 52));
}
IdleCommandScope::IdleCommandScope(uint8_t* base, PPCContext& ctx) {
  uint32_t command{};
  if (!trace.idle_actor_active || ctx.r3.u32 || ctx.r4.u32 ||
      !Read32(base, ctx.r5.u32, command) || command != trace.trial_motion) return;
  base_ = base;
  ctx_ = &ctx;
}
IdleCommandScope::~IdleCommandScope() {
  if (!base_ || !trace.idle_actor_active) return;
  uint32_t track{};
  if (!Read32(base_, 0x83426FCC, track) || track != 0x834271A0 ||
      !Readable(base_, track, 144) || base_[kPlayers + 4] != trace.idle_bank_slot) return;
  // Reuse the verified native opcode-3 command in shared entry 1. The real
  // idle script is otherwise interpreted untouched, including face cues.
  if (base_[track + 130] == 0) {
    auto call = *ctx_;
    call.r3.u64 = 0;
    call.r4.u64 = 0;
    call.r5.u64 = trace.idle_loop_command;
    __imp__sub_825160D0(call, base_);
  }
  ++trace.idle_loops;
  Log("OWPROBE idle actor: native cycle update={} cycle={} motion={} loop_state={} phase={:.3f}",
      trace.frame, trace.idle_loops, trace.trial_motion, base_[track + 130], FloatAt(base_, track + 52));
}
void HoldIdleRoot(uint8_t* base, PPCContext& ctx, uint32_t slot) {
  if (slot || !trace.idle_actor_active || !trace.camera_hold_active ||
      !Readable(base, kPlayers, 152) || base[kPlayers + 4] != trace.idle_bank_slot) return;
  uint32_t phase{}, motion{};
  if (!ReferencePictorial(base, phase) || phase != 7 ||
      !Read32(base, kPlayers + 52, motion) || motion >> 16 != trace.trial_motion ||
      !Readable(base, 0x83426FD0, 12)) return;
  if (trace.frame % 30 == 0)
    Log("OWPROBE idle actor: root held update={} raw_delta=({:.5f},{:.5f},{:.5f}) xyz=({:.4f},{:.4f},{:.4f}) phase={:.3f}",
        trace.frame, FloatAt(base, 0x83426FD0), FloatAt(base, 0x83426FD4), FloatAt(base, 0x83426FD8),
        FloatAt(base, kPlayers + 12), FloatAt(base, kPlayers + 16), FloatAt(base, kPlayers + 20),
        FloatAt(base, 0x834271A0 + 52));
  if (REXCVAR_GET(doax2_open_world_pictorial_movement_trial) && trace.frame % 15 == 0) {
    uint32_t native{};
    Read32(base, 0x834271A0 + 108, native);
    Log("OWPROBE movement binding: sample update={} requested={} native={} phase={:.3f} raw_delta=({:.5f},{:.5f},{:.5f}) xyz=({:.4f},{:.4f},{:.4f})",
        trace.frame, trace.trial_motion, native >> 16, FloatAt(base, 0x834271A0 + 52),
        FloatAt(base, 0x83426FD0), FloatAt(base, 0x83426FD4), FloatAt(base, 0x83426FD8),
        FloatAt(base, kPlayers + 12), FloatAt(base, kPlayers + 16), FloatAt(base, kPlayers + 20));
  }
  // Consume native animation roots exactly once. The bounded controller is
  // the sole translation owner; no native root travel is added to its step.
  REX_STORE_U32(0x83426FD0, 0);
  REX_STORE_U32(0x83426FD4, 0);
  REX_STORE_U32(0x83426FD8, 0);
  if (!trace.movement.active || trace.movement_root_frame == trace.frame) return;
  trace.movement_root_frame = trace.frame;
  const double now = std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  if (now >= trace.camera_hold_until) { RestoreIdleActor(base); return; }
  uint32_t native{};
  const bool traveling = trace.orbit_started && !trace.orbit_stopped &&
      trace.movement_input_frame == trace.frame && trace.trial_motion == 8223 &&
      !base[kPlayers + 56] && Read32(base, 0x834271A0 + 108, native) && native >> 16 == 8223;
  if (trace.exploration && !ExplorationGeometryMatches(base)) {
    Log("OWPROBE exploration: terrain ownership changed update={}", trace.frame);
    RestoreIdleActor(base);
    return;
  }
  float x = trace.movement_pose[0], z = trace.movement_pose[2];
  const float before_x = x, before_z = z;
  if (!trace.movement.Step(x, z, traveling ? trace.movement_x : 0,
          traveling ? trace.movement_y : 0, trace.orbit.yaw, trace.movement_seconds)) {
    Log("OWPROBE movement: rejected update={}; invalid or externally displaced pose", trace.frame);
    RestoreIdleActor(base);
    return;
  }
  float y = std::bit_cast<float>(trace.movement_original_pose[1]);
  bool blocked = false;
  if (trace.exploration) {
    float projected{};
    TerrainRejection reason=TerrainRejection::None;
    const auto validate = [&](TerrainPoint& candidate) {
      if (std::hypot(candidate[0]-trace.movement.anchor_x,candidate[2]-trace.movement.anchor_z) >
          trace.movement.radius+.00001f) return false;
      if (!trace.terrain.Supported(candidate[0],candidate[2],trace.movement_pose[1],projected,&reason)) return false;
      // Sweep foot and torso against triangles present in the stage terrain.
      for (float lift : {.2f,.9f}) {
        TerrainPoint from{before_x,trace.movement_pose[1]+lift,before_z};
        TerrainPoint to{candidate[0],projected+lift,candidate[2]};
        if (trace.terrain.Clip(from,to) < 1) { reason=TerrainRejection::Obstruction; return false; }
      }
      // Copy registers so this extra query cannot alter the root caller's ABI.
      // r6=1 bypasses activity-specific constant-height overrides.
      auto call = ctx;
      call.r3.u64 = 0x82E2DB50;
      call.r6.u64 = 1;
      call.f1.f64 = candidate[0]; call.f2.f64 = candidate[2]; call.f3.f64 = .1;
      __imp__sub_825BEE88(call,base);
      candidate[1] = float(call.f1.f64);
      if (!std::isfinite(candidate[1]) || std::abs(candidate[1]-projected) > .12f ||
          std::abs(candidate[1]-trace.movement_pose[1]) > .15f || candidate[1] < trace.terrain.minimum_y) {
        reason=TerrainRejection::NativeHeight;
        return false;
      }
      return true;
    };
    TerrainPoint proposed{x,trace.movement_pose[1],z};
    const auto requested=proposed;
    blocked=!validate(proposed);
    const auto original_reason=reason;
    const bool sliding=blocked && SlidePlanar(trace.movement_pose,proposed,validate);
    if (blocked && !sliding) proposed=trace.movement_pose;
    blocked=blocked && !sliding;
    x=proposed[0]; y=proposed[1]; z=proposed[2];
    if (trace.frame % 30 == 0)
      Log("OWPROBE exploration: ground update={} blocked={} native_y={:.4f} projected_y={:.4f} xyz=({:.4f},{:.4f},{:.4f})",
          trace.frame, blocked, y, projected, x,y,z);
    const bool contact_changed=original_reason != trace.contact_reason ||
        sliding != trace.contact_sliding || blocked != trace.contact_blocked;
    if (contact_changed || (trace.frame % 30 == 0 && original_reason != TerrainRejection::None))
      Log("OWPROBE exploration: contact update={} reason={} sliding={} blocked={} requested=({:.4f},{:.4f}) final=({:.4f},{:.4f})",
          trace.frame,RejectionName(original_reason),sliding,blocked,
          requested[0],requested[2],x,z);
    trace.contact_reason=original_reason;
    trace.contact_sliding=sliding;
    trace.contact_blocked=blocked;
  }
  trace.movement_pose = {x,y,z};
  if (traveling) trace.movement_facing = trace.movement.Facing();
  REX_STORE_U32(kPlayers + 12, std::bit_cast<uint32_t>(x));
  REX_STORE_U32(kPlayers + 16, std::bit_cast<uint32_t>(y));
  REX_STORE_U32(kPlayers + 20, std::bit_cast<uint32_t>(z));
  if (traveling) REX_STORE_U32(kPlayers + 24, trace.movement.Facing());
  if (trace.frame % 15 == 0)
    Log("OWPROBE movement: step update={} traveling={} xyz=({:.4f},{:.4f},{:.4f}) delta=({:.5f},{:.5f}) radius={:.4f} facing={} dt={:.5f}",
        trace.frame, traveling, x, FloatAt(base, kPlayers + 16), z, x - before_x, z - before_z,
        std::hypot(x - trace.movement.anchor_x, z - trace.movement.anchor_z),
        trace.movement.Facing(), trace.movement_seconds);
}
void CommitIdleMovement(uint8_t* base, uint32_t slot) {
  uint32_t phase{}, motion{};
  if (slot || !trace.movement.active || !trace.idle_actor_active ||
      !ReferencePictorial(base,phase) || phase != 7 ||
      !Read32(base,kPlayers+52,motion) || motion >> 16 != trace.trial_motion ||
      base[kPlayers+4] != trace.idle_bank_slot) return;
  // Native 82510218 can subtract a loop-origin offset even with zero root
  // samples. Commit the owned pose after it runs, before rig evaluation.
  for (uint32_t i=0; i<3; ++i)
    REX_STORE_U32(kPlayers+12+i*4,std::bit_cast<uint32_t>(trace.movement_pose[i]));
  REX_STORE_U32(kPlayers+24,trace.movement_facing);
}
}  // namespace doax2::open_world

REX_EXTERN(__imp__sub_824BF888);
REX_HOOK_RAW(sub_824BF888) {
  __imp__sub_824BF888(ctx, base);
  doax2::open_world::RequestIdleBank(base, ctx);
}

// Newly verified generated functions; these wrappers only record requests and
// invoke each original exactly once. They are absent from ordinary builds.
REX_EXTERN(__imp__sub_825A15D8);
REX_HOOK_RAW(sub_825A15D8) {
  using namespace doax2::open_world;
  if (Enabled()) {
    Announce();
    Log("OWPROBE screen request: update={} requested={} caller={:08X}",
        trace.frame, int32_t(int8_t(ctx.r3.u32)), uint32_t(ctx.lr));
  }
  __imp__sub_825A15D8(ctx, base);
}

REX_EXTERN(__imp__sub_826CB1A0);
REX_HOOK_RAW(sub_826CB1A0) {
  using namespace doax2::open_world;
  const bool observe = Enabled() && Readable(base, kCalendar, 1);
  const int32_t before = observe ? int8_t(base[kCalendar]) : -999;
  const uint32_t caller = uint32_t(ctx.lr);
  if (observe) {
    Announce();
    Log("OWPROBE calendar advance entry: update={} before={} caller={:08X} "
        "r3={:08X} r4={:08X} r5={:08X} r6={:08X}", trace.frame, before, caller,
        ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32);
  }
  __imp__sub_826CB1A0(ctx, base);
  if (observe && Readable(base, kCalendar, 1))
    Log("OWPROBE calendar advance exit: update={} before={} after={} caller={:08X}",
        trace.frame, before, int32_t(int8_t(base[kCalendar])), caller);
}

// Verified stage height virtual method (vtable +44). Observe the native query.
// The separate default-off trial declines its volleyball height override;
// it never issues additional queries or replaces the native f1 result.
REX_EXTERN(__imp__sub_825BEE88);
REX_HOOK_RAW(sub_825BEE88) {
  using namespace doax2::open_world;
  const uint32_t object = ctx.r3.u32;
  const bool trial = object == 0x82E2DB50 && TerrainTrialActive(base);
  struct RestoreTrial {
    bool previous;
    ~RestoreTrial() { trace.terrain_query_trial = previous; }
  } restore{std::exchange(trace.terrain_query_trial, trial)};
  const uint32_t caller = uint32_t(ctx.lr), bypass = ctx.r6.u32;
  const uint32_t lane = caller == 0x82513210 ? 1 : 0;
  const bool observe = Enabled() && object == 0x82E2DB50 &&
      trace.frame % 30 == 0 && trace.ground_sample[lane] != trace.frame;
  const double x = ctx.f1.f64, z = ctx.f2.f64, radius = ctx.f3.f64;
  uint32_t predicate{}, height{}, vertices{}, triangles{}, triangle_count{};
  if (observe) {
    trace.ground_sample[lane] = trace.frame;
    Read32(base, object + 112, predicate);
    Read32(base, object + 116, height);
    Read32(base, object + 20, vertices);
    Read32(base, object + 24, triangles);
    Read32(base, object + 108, triangle_count);
  }
  __imp__sub_825BEE88(ctx, base);
  if (observe)
    Log("OWPROBE ground: update={} caller={:08X} x={:.4f} z={:.4f} radius={:.4f} "
        "bypass={} terrain_trial={} result={:.4f} predicate={:08X} height={:08X} "
        "vertices={:08X} triangles={:08X} extra_count={}", trace.frame, caller,
        x, z, radius, bypass, trial, ctx.f1.f64, predicate, height, vertices, triangles,
        triangle_count);
}

// Decline volleyball's flat-zero override only inside the narrowly gated
// native Niki query. The original predicate and general query still execute.
REX_EXTERN(__imp__sub_8268AE80);
REX_HOOK_RAW(sub_8268AE80) {
  __imp__sub_8268AE80(ctx, base);
  if (doax2::open_world::trace.terrain_query_trial) ctx.r3.u64 = 0;
}

REX_EXTERN(__imp__sub_825BAD40);
REX_HOOK_RAW(sub_825BAD40) {
  using namespace doax2::open_world;
  if (Enabled() && ctx.r3.u32 == 0x82E2DB50)
    Log("OWPROBE terrain bind: update={} caller={:08X} vertices={:08X} "
        "vertex_count={} triangles={:08X} triangle_count={} fallback={:.4f}",
        trace.frame, uint32_t(ctx.lr), ctx.r4.u32, ctx.r5.u32,
        ctx.r6.u32, ctx.r7.u32, ctx.f1.f64);
  __imp__sub_825BAD40(ctx, base);
}

namespace doax2::open_world {
namespace {
bool MotionResearch() {
  if (!Enabled() || !REXCVAR_GET(doax2_open_world_motion_research) ||
      trace.motion_research_events >= 1600) return false;
  // Spend the observation allowance only while volleyball or Beach Flags
  // is actually updating, not on title/selector character previews.
  for (uint32_t i = 0; i < 2; ++i) {
    const auto last = trace.activities[i].last_update;
    if (last != ~uint64_t{0} && trace.frame >= last && trace.frame - last <= 1) return true;
  }
  return false;
}
uint32_t HalfAt(const uint8_t* base, uint32_t address) {
  uint32_t word{};
  return Read32(base, address & ~3u, word) ?
      ((word >> ((address & 2) ? 0 : 16)) & 0xFFFF) : ~0u;
}
void ObserveNativeMotion(const uint8_t* base, uint32_t slot) {
  if (!MotionResearch() || slot >= 4) return;
  const uint32_t player = kPlayers + slot * 152;
  if (!Readable(base, player, 152)) return;
  uint32_t identity{};
  if (!Read32(base, player + 52, identity)) return;
  const bool changed = identity != trace.rig_motion_identity[slot];
  if (!changed && (trace.frame % 30 || trace.motion_rig_sample[slot] == trace.frame)) return;
  trace.rig_motion_identity[slot] = identity;
  trace.motion_rig_sample[slot] = trace.frame;
  ++trace.motion_research_events;
  uint32_t resource{}, facing{};
  Read32(base, 0x83426FD0 + slot * 116 + 24, resource);
  Read32(base, player + 24, facing);
  const uint32_t track = 0x834271A0 + slot * 2 * 144;
  const bool have_track = Readable(base, track, 288);
  Log("OWPROBE motion rig: update={} screen={} slot={} changed={} character_byte1={} byte0={} enabled={} native52={} native54={} high64={} queued66={} category62={} flags60={:02X} phase44={:.3f} rate48={:.3f} xyz=({:.4f},{:.4f},{:.4f}) facing={} resource={:08X} track_phase=({:.3f},{:.3f}) track_end=({},{}) track_flags=({:08X},{:08X})",
      trace.frame, trace.screen.id, slot, changed, base[player + 1], base[player], base[player + 10],
      identity >> 16, identity & 0xFFFF, HalfAt(base, player + 64), HalfAt(base, player + 66),
      base[player + 62], base[player + 60], FloatAt(base, player + 44), FloatAt(base, player + 48),
      FloatAt(base, player + 12), FloatAt(base, player + 16), FloatAt(base, player + 20), facing, resource,
      have_track ? FloatAt(base, track + 52) : -1.0f,
      have_track ? FloatAt(base, track + 144 + 52) : -1.0f,
      have_track ? HalfAt(base, track + 112) : ~0u,
      have_track ? HalfAt(base, track + 144 + 112) : ~0u,
      have_track ? uint32_t(base[track + 124]) << 24 | uint32_t(base[track + 125]) << 16 |
          uint32_t(base[track + 126]) << 8 | base[track + 127] : 0,
      have_track ? uint32_t(base[track + 268]) << 24 | uint32_t(base[track + 269]) << 16 |
          uint32_t(base[track + 270]) << 8 | base[track + 271] : 0);
}
}  // namespace

VolleyballMotionScope::VolleyballMotionScope(const uint8_t* base, uint32_t player,
                                             uint32_t motion, double rate, uint32_t caller) {
  if (!MotionResearch() || player < kPlayers || player >= kPlayers + 4 * 152 ||
      (player - kPlayers) % 152 || motion == 0xFFFF || !Readable(base, player, 152)) return;
  base_ = base;
  player_ = player;
  motion_ = motion;
  caller_ = caller;
  rate_ = rate;
  before_ = HalfAt(base, player + 64);
}

VolleyballMotionScope::~VolleyballMotionScope() {
  if (!base_ || !MotionResearch()) return;
  const uint32_t slot = (player_ - kPlayers) / 152;
  ++trace.motion_research_events;
  // Read only the eight native metadata words actually addressed by this
  // request. Do not treat raw IDs or successor entries as semantic clip names.
  std::array<uint32_t, 8> row{};
  const uint32_t at = 0x82B11F68 + motion_ * 32;
  bool have_row = true;
  for (uint32_t i = 0; i < row.size(); ++i) have_row &= Read32(base_, at + i * 4, row[i]);
  Log("OWPROBE volleyball motion request: update={} screen={} slot={} character_byte1={} caller={:08X} high={}->{} actual_high={} native52={} native54={} rate={} table_valid={} category={} row_id={} row_native={} row_next={} words8_28=({:08X},{:08X},{:08X},{:08X},{:08X},{:08X})",
      trace.frame, trace.screen.id, slot, base_[player_ + 1], caller_, before_, motion_,
      HalfAt(base_, player_ + 64), HalfAt(base_, player_ + 52), HalfAt(base_, player_ + 54), rate_,
      have_row, row[0] >> 16, row[0] & 0xFFFF, row[1] >> 16, row[1] & 0xFFFF,
      row[2], row[3], row[4], row[5], row[6], row[7]);
}

RootMotionScope::RootMotionScope(const uint8_t* base, uint32_t slot) {
  if (!MotionResearch() || slot >= 4) return;
  const uint32_t player = kPlayers + slot * 152;
  uint32_t motion{};
  if (!Readable(base, player, 152) || !Read32(base, player + 52, motion)) return;
  const bool changed = motion != trace.root_motion_identity[slot];
  if (!changed && (trace.frame % 30 || trace.root_sample[slot] == trace.frame)) return;
  trace.root_motion_identity[slot] = motion;
  trace.root_sample[slot] = trace.frame;
  base_ = base;
  slot_ = slot;
  motion_ = motion >> 16;
  for (uint32_t i = 0; i < 3; ++i) {
    before_[i] = FloatAt(base, player + 12 + i * 4);
    delta_[i] = FloatAt(base, 0x83426FD0 + slot * 116 + i * 4);
  }
}
RootMotionScope::~RootMotionScope() {
  if (!base_ || !MotionResearch()) return;
  const uint32_t player = kPlayers + slot_ * 152;
  ++trace.motion_research_events;
  Log("OWPROBE root step: update={} screen={} slot={} native52={} delta_local=({:.5f},{:.5f},{:.5f}) applied_xyz=({:.5f},{:.5f},{:.5f}) flags93={} phase44={:.3f}",
      trace.frame, trace.screen.id, slot_, motion_, delta_[0], delta_[1], delta_[2],
      FloatAt(base_, player + 12) - before_[0], FloatAt(base_, player + 16) - before_[1],
      FloatAt(base_, player + 20) - before_[2], base_[player + 93], FloatAt(base_, player + 44));
}
}  // namespace doax2::open_world

// The native rig pipeline calls the height method, then animation/rig services.
// Slot validity is established by observing this actual native call, not by
// inferring occupancy from nonzero coordinates in a cached player record.
void doax2::open_world::ObserveRig(const uint8_t* base, uint32_t slot) {
  ObserveNativeMotion(base, slot);
  const bool observe = Enabled() && slot < 4 && trace.frame % 30 == 0 &&
      trace.rig_sample[slot] != trace.frame;
  if (observe) trace.rig_sample[slot] = trace.frame;
  if (observe) {
    const uint32_t rig = 0x8342CA30 + slot * 412;
    uint32_t facing{}, motion_word{}, rig_resource{};
    Read32(base, kPlayers + slot * 152 + 24, facing);
    Read32(base, kPlayers + slot * 152 + 52, motion_word);
    Read32(base, 0x83426FD0 + slot * 116 + 24, rig_resource);
    Log("OWPROBE rig: update={} slot={} ground_y={:.4f} facing={} "
        "motion52={} resource={:08X}", trace.frame, slot,
        FloatAt(base, rig + 44), facing, motion_word >> 16, rig_resource);
  }
}

// Observe the actual native rig boundary. Raw offset labels deliberately avoid
// interpreting a resource-bank selector as a verified animation identity.
void doax2::open_world::ObservePictorialMotion(const uint8_t* base, uint32_t slot,
                                             bool after) {
  if (!Enabled() || slot != 0 || trace.motion_events >= 256) return;
  uint32_t table{}, state{}, callback{}, phase{}, selected{}, scene{};
  if (!Readable(base, kScreen, 1) || base[kScreen] != 2 ||
      !Read32(base, kScreen + 68, table) || table != 0x82B83C80 ||
      !Read32(base, table + 2 * 36 + 4, callback) || callback != 0x825A2270 ||
      !Read32(base, kScreen + 40, state) || state != 1 ||
      !Read32(base, 0x83DB9C54, phase) || phase < 5 || phase > 8 ||
      !Read32(base, 0x83DB99CC, selected) || selected != 0x83DA61BC ||
      !Read32(base, selected, scene) || scene != 58) return;
  std::array<uint32_t, 8> identity{};
  // Fixed player record: motion pair and adjacent native control bytes/words.
  constexpr std::array<uint32_t, 5> fields{52, 56, 60, 64, 92};
  for (size_t i = 0; i < fields.size(); ++i)
    if (!Read32(base, kPlayers + fields[i], identity[i])) return;
  if (!Read32(base, 0x83426FD0 + 24, identity[5])) return;
  // 82513158 obtains this resource pointer from the fixed slot table.
  // Read the bounded record only; never follow its nested resource pointers.
  if (!identity[5] || !Readable(base, identity[5], 136) ||
      !Read32(base, identity[5] + 108, identity[6]) ||
      !Read32(base, identity[5] + 112, identity[7])) return;
  // The native rig buffer rotates and its output words change every update.
  // They are sampled payload, not motion identity: including them here spends
  // the entire event allowance before the late completion transition.
  const bool changed = !trace.have_motion ||
      !std::equal(identity.begin(), identity.begin() + 5,
                  trace.motion_identity.begin());
  const bool sample = !after && trace.frame % 30 == 0 &&
      trace.motion_sample != trace.frame;
  if (!changed && !sample) return;
  trace.have_motion = true;
  trace.motion_identity = identity;
  trace.motion_sample = trace.frame;
  ++trace.motion_events;
  Log("OWPROBE pictorial motion: update={} boundary={} phase={} changed={} "
      "script_elapsed={:.3f} script_frame={:.3f} actor_mask={:X} "
      "player44={:.3f} player48={:.3f} motion52={} motion54={} "
      "player56={:08X} player60={:08X} player64={:08X} player92={:08X} "
      "resource={:08X} resource108={:08X} resource112={:08X}",
      trace.frame, after ? "after_rig" : "before_rig", phase, changed,
      FloatAt(base, 0x833B4C68 + 24), FloatAt(base, 0x833B4C68 + 28),
      ActorUpdateMask(base), FloatAt(base, kPlayers + 44),
      FloatAt(base, kPlayers + 48), identity[0] >> 16, identity[0] & 0xFFFF,
      identity[1], identity[2], identity[3], identity[4], identity[5],
      identity[6], identity[7]);
}

namespace doax2::open_world {
namespace {
constexpr std::array<uint32_t, 10> kCameraScriptFields{
    3536, 3540, 3544, 3548, 3552, 3556, 3580, 3620, 2928, 2932};
constexpr std::array<uint32_t, 12> kCameraHoldClocks{
    3540, 3544, 832 + 128, 832 + 132, 832 + 136, 832 + 140,
    1024 + 128, 1024 + 132, 1024 + 136, 1024 + 140, 1940, 1944};
constexpr std::array<uint32_t, 11> kCameraHoldIdentity{
    3536, 832 + 112, 832 + 120, 832 + 124, 832 + 168, 832 + 184,
    1024 + 112, 1024 + 120, 1024 + 124, 1024 + 168, 1024 + 184};

bool CameraHoldReady(const uint8_t* base, uint32_t& object) {
  uint32_t objects{}, index{}, phase{}, pc{}, mode{}, active{};
  if (!Read32(base, kScene + 928, objects) || !objects ||
      !Read32(base, kScene + 968, index) || index != 0 ||
      !Read32(base, 0x83DB9C54, phase) || phase != 7 ||
      !Readable(base, objects, 4144) ||
      !Read32(base, objects + 3536, pc) || pc != 0x82D92A48 ||
      !Read32(base, objects + 3580, mode) || mode != 3 ||
      !Read32(base, objects + 3620, active) || active != 1) return false;
  for (uint32_t offset : kCameraHoldClocks)
    if (!std::isfinite(FloatAt(base, objects + offset))) return false;
  for (uint32_t track : {832u, 1024u}) {
    uint32_t type{}, limit{};
    if (!Read32(base, objects + track + 112, type) ||
        !Read32(base, objects + track + 124, limit) ||
        !CameraTrackCanHold(type, limit, FloatAt(base, objects + track + 128),
                           FloatAt(base, objects + track + 136))) return false;
  }
  // Stay inside the existing shot, away from its dispatch/end boundaries.
  const float gate = FloatAt(base, objects + 1940);
  if (!CameraGateCanHold(gate)) return false;
  object = objects;
  return true;
}

// Verified leaf command boundaries. Capture arguments before the command
// advances its instruction pointer; call its original exactly once even when
// observation is disabled or the script range fails validation.
uint32_t ActorUpdateMask(const uint8_t* base) {
  uint32_t mask{};
  for (uint32_t i = 0; i < 4; ++i) {
    const uint32_t flag = kPlayers + i * 152 + 10;
    if (Readable(base, flag, 1) && base[flag]) mask |= 1u << i;
  }
  return mask;
}
}  // namespace

namespace {
constexpr uint32_t kActorTracks = 0x834271A0;
constexpr std::array<uint32_t, 4> kActorClockFields{52, 56, 72, 76};
constexpr std::array<uint32_t, 6> kActorIdentityFields{48, 108, 112, 124, 128, 132};
bool ActorTracksReady(const uint8_t* base) {
  bool active = false;
  for (uint32_t i = 0; i < 2; ++i) {
    const uint32_t object = kActorTracks + i * 144;
    if (!Readable(base, object, 144)) return false;
    if (base[object + 124] || (!base[object + 125] && !base[object + 127])) continue;
    active = true;
    const float phase = FloatAt(base, object + 52);
    const uint32_t end = (uint32_t(base[object + 112]) << 8) | base[object + 113];
    if (!ActorTrackCanHold(phase, end)) return false;
  }
  return active;
}
}

OrbitCameraScope::OrbitCameraScope(uint8_t* base, uint32_t object) {
  if (!REXCVAR_GET(doax2_open_world_probe) ||
      (!REXCVAR_GET(doax2_open_world_pictorial_orbit_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_movement_trial)) || trace.orbit_stopped) return;
  // AF08 is called for multiple camera objects in the same update. A call
  // for an unrelated object is not an ownership loss of the admitted one.
  if (!trace.camera_hold_object || object != trace.camera_hold_object) return;
  const double now = std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  uint32_t selected{}, table{}, state{}, record{}, scene{}, motion{};
  const bool eligible = trace.camera_hold_active && object == trace.camera_hold_object &&
      now < trace.camera_hold_until && Readable(base, kScreen, 1) && base[kScreen] == 2 &&
      Read32(base, kScreen + 68, table) && table == 0x82B83C80 &&
      Read32(base, kScreen + 40, state) && state == 1 &&
      Read32(base, 0x83DB99CC, record) && record == 0x83DA61BC &&
      Read32(base, record, scene) && scene == 58 &&
      Read32(base, kPlayers + 52, motion) && IdleActorExpected(base, motion >> 16) &&
      ActorUpdateMask(base) == 1 && CameraHoldReady(base, selected) && selected == object;
  if (!eligible) {
    if (trace.orbit_started) {
      trace.orbit_stopped = true;
      uint32_t phase{}, index{}, mode{};
      Read32(base, 0x83DB9C54, phase);
      Read32(base, kScene + 968, index);
      Read32(base, object + 3580, mode);
      Log("OWPROBE orbit: release update={} object={:08X} hold_active={} deadline={} phase={} camera_index={} mode={} selected={:08X}; native pose retained",
          trace.frame, object, trace.camera_hold_active, now >= trace.camera_hold_until,
          phase, index, mode, selected);
    }
    return;
  }
  for (uint32_t i = 0; i < pose_.size(); ++i)
    if (!Read32(base, object + 192 + i * 4, pose_[i])) return;
  if (!trace.orbit_started) {
    const std::array<float, 3> eye{FloatAt(base, object + 192),
        FloatAt(base, object + 196), FloatAt(base, object + 200)};
    const std::array<float, 3> aim{FloatAt(base, object + 208),
        FloatAt(base, object + 212), FloatAt(base, object + 216)};
    if (!trace.orbit.Begin(eye, aim)) {
      trace.orbit_stopped = true;
      Log("OWPROBE orbit: admission rejected update={}; invalid native pose", trace.frame);
      return;
    }
    trace.orbit_started = true;
    trace.orbit.exploration = trace.exploration;
    if (trace.exploration) {
      trace.orbit.radius = std::clamp(trace.orbit.radius,1.5f,6.f);
      trace.orbit.pitch = std::clamp(trace.orbit.pitch,.05f,1.f);
    }
    trace.orbit_target_anchor = aim;
    trace.orbit_time = now;
    Log("OWPROBE orbit: begin update={} object={:08X} radius={:.3f} target=({:.3f},{:.3f},{:.3f}) yaw={:.3f} pitch={:.3f}",
        trace.frame, object, trace.orbit.radius, aim[0], aim[1], aim[2], trace.orbit.yaw, trace.orbit.pitch);
  }
  if (trace.orbit_frame != trace.frame) {
    float x = 0, y = 0, zoom = 0;
    DWORD foreground_process{};
    GetWindowThreadProcessId(GetForegroundWindow(), &foreground_process);
    if (auto* runtime = rex::Runtime::instance();
        runtime && foreground_process == GetCurrentProcessId()) {
      if (auto* input = dynamic_cast<rex::input::InputSystem*>(runtime->input_system())) {
        rex::input::X_INPUT_STATE pad{};
        if (input->GetState(0, &pad) == 0 &&
            !(uint16_t(pad.gamepad.buttons) & (rex::input::X_INPUT_GAMEPAD_B |
                rex::input::X_INPUT_GAMEPAD_START | rex::input::X_INPUT_GAMEPAD_BACK))) {
          x = float(int16_t(pad.gamepad.thumb_rx)) / 32768.0f;
          y = float(int16_t(pad.gamepad.thumb_ry)) / 32768.0f;
          zoom = (float(pad.gamepad.right_trigger)-float(pad.gamepad.left_trigger))/255.f;
          // Keyboard zoom is focus-gated with the controller sample.
          if (GetAsyncKeyState('E') & 0x8000) zoom += 1;
          if (GetAsyncKeyState('Q') & 0x8000) zoom -= 1;
        }
      }
    }
    trace.orbit.Step(x, y, now - trace.orbit_time);
    trace.orbit.Zoom(zoom, now - trace.orbit_time);
    trace.orbit_time = now;
    trace.orbit_frame = trace.frame;
    if (trace.frame % 30 == 0)
      Log("OWPROBE orbit: update={} stick=({:.3f},{:.3f}) yaw={:.3f} pitch={:.3f}",
          trace.frame, x, y, trace.orbit.yaw, trace.orbit.pitch);
  }
  trace.orbit.target = trace.orbit_target_anchor;
  if (trace.movement.active) {
    trace.orbit.target[0] += FloatAt(base, kPlayers + 12) - trace.movement.anchor_x;
    trace.orbit.target[2] += FloatAt(base, kPlayers + 20) - trace.movement.anchor_z;
    if (trace.exploration)
      trace.orbit.target = {trace.movement_pose[0],trace.movement_pose[1]+.85f,trace.movement_pose[2]};
  }
  auto eye = trace.orbit.Eye();
  if (trace.exploration && trace.movement.active) {
    // Terrain contains some vertical faces, but may omit decorative props.
    // Retract immediately on a ray hit; leave a small clearance before it.
    const float hit = trace.terrain.CameraClip(trace.orbit.target,eye);
    // Keep eye distinct from target even when a surface is very close.
    const float fraction = std::max(.05f/trace.orbit.radius,
        hit - (hit < 1 ? .15f/trace.orbit.radius : 0.f));
    for (uint32_t i=0;i<3;++i)
      eye[i] = trace.orbit.target[i]+(eye[i]-trace.orbit.target[i])*fraction;
    if (trace.frame % 60 == 0)
      Log("OWPROBE exploration: camera update={} radius={:.3f} fraction={:.3f} yaw={:.3f} pitch={:.3f}",
          trace.frame,trace.orbit.radius,fraction,trace.orbit.yaw,trace.orbit.pitch);
  }
  base_ = base;
  object_ = object;
  for (uint32_t i = 0; i < 3; ++i) {
    REX_STORE_U32(object + 192 + i * 4, std::bit_cast<uint32_t>(eye[i]));
    REX_STORE_U32(object + 208 + i * 4, std::bit_cast<uint32_t>(trace.orbit.target[i]));
  }
}

OrbitCameraScope::~OrbitCameraScope() {
  if (!base_) return;
  auto* base = base_;
  // AF08 derives angles in this 64-byte evaluated pose as well as building
  // matrices. Restore the whole input block so the orbit cannot feed back
  // into the next cinematic evaluation; retain the native output matrices.
  for (uint32_t i = 0; i < pose_.size(); ++i)
    REX_STORE_U32(object_ + 192 + i * 4, pose_[i]);
}

ActorClockHoldScope::ActorClockHoldScope(uint8_t* base, uint32_t slot, uint32_t track) {
  if ((!REXCVAR_GET(doax2_open_world_pictorial_actor_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_long_actor_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_orbit_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial)) ||
      !trace.camera_hold_active || slot != 0 || track > 1) return;
  const double now = std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  if (now >= trace.camera_hold_until) return;
  uint32_t phase{}, selected{}, motion{}, table{}, callback{}, state{};
  if (!Readable(base, kScreen, 1) || base[kScreen] != 2 ||
      !Read32(base, kScreen + 68, table) || table != 0x82B83C80 ||
      !Read32(base, table + 2 * 36 + 4, callback) || callback != 0x825A2270 ||
      !Read32(base, kScreen + 40, state) || state != 1 ||
      !Read32(base, 0x83DB9C54, phase) || phase != 7 ||
      !Read32(base, 0x83DB99CC, selected) || selected != 0x83DA61BC ||
      !Read32(base, kPlayers + 52, motion) || (motion >> 16) != 27 ||
      ActorUpdateMask(base) != 1 || !ActorTracksReady(base)) {
    trace.camera_hold_active = false;
    trace.pictorial_hold.Update(false, FloatAt(base, 0x833B4C68 + 28), now);
    Log("OWPROBE actor hold: aborted update={}; owner or track eligibility lost", trace.frame);
    return;
  }
  object_ = kActorTracks + track * 144;
  if (base[object_ + 124] || (!base[object_ + 125] && !base[object_ + 127])) return;
  for (size_t i = 0; i < clocks_.size(); ++i)
    if (!Read32(base, object_ + kActorClockFields[i], clocks_[i])) return;
  for (size_t i = 0; i < identity_.size(); ++i)
    if (!Read32(base, object_ + kActorIdentityFields[i], identity_[i])) return;
  base_ = base;
}

ActorClockHoldScope::~ActorClockHoldScope() {
  if (!base_) return;
  uint8_t* base = base_;
  for (size_t i = 0; i < identity_.size(); ++i) {
    uint32_t current{};
    if (!Read32(base, object_ + kActorIdentityFields[i], current) || current != identity_[i]) {
      trace.camera_hold_active = false;
      const double now = std::chrono::duration<double>(
          std::chrono::steady_clock::now().time_since_epoch()).count();
      trace.pictorial_hold.Update(false, FloatAt(base, 0x833B4C68 + 28), now);
      Log("OWPROBE actor hold: aborted update={} object={:08X} identity_offset={}; native transition preserved",
          trace.frame, object_, kActorIdentityFields[i]);
      return;
    }
  }
  const float native_phase = FloatAt(base, object_ + 52);
  for (size_t i = 0; i < clocks_.size(); ++i)
    REX_STORE_U32(object_ + kActorClockFields[i], clocks_[i]);
  if (trace.frame % 30 == 0)
    Log("OWPROBE actor hold: update={} object={:08X} phase_native={:.3f} restored={:.3f}; native clock routine ran",
        trace.frame, object_, native_phase, std::bit_cast<float>(clocks_[0]));
}

CameraClockHoldScope::CameraClockHoldScope(uint8_t* base, uint32_t object) {
  if ((!REXCVAR_GET(doax2_open_world_pictorial_camera_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_actor_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_long_actor_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_orbit_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_idle_trial) &&
       !REXCVAR_GET(doax2_open_world_pictorial_movement_trial)) ||
      !trace.camera_hold_active || object != trace.camera_hold_object) return;
  const double now = std::chrono::duration<double>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  uint32_t selected{}, table{}, scene{};
  if (now >= trace.camera_hold_until ||
      !Readable(base, kScreen, 1) || base[kScreen] != 2 ||
      !Read32(base, kScreen + 68, table) || table != 0x82B83C80 ||
      !Read32(base, 0x83DB99CC, selected) || selected != 0x83DA61BC ||
      !Read32(base, selected, scene) || scene != 58 ||
      !CameraHoldReady(base, selected) || selected != object) return;
  for (size_t i = 0; i < clocks_.size(); ++i)
    if (!Read32(base, object + kCameraHoldClocks[i], clocks_[i])) return;
  for (size_t i = 0; i < identity_.size(); ++i)
    if (!Read32(base, object + kCameraHoldIdentity[i], identity_[i])) return;
  base_ = base;
  object_ = object;
}

CameraClockHoldScope::~CameraClockHoldScope() {
  if (!base_ || !Readable(base_, object_, 4144)) return;
  uint8_t* base = base_;
  for (size_t i = 0; i < identity_.size(); ++i) {
    uint32_t current{};
    if (!Read32(base, object_ + kCameraHoldIdentity[i], current) || current != identity_[i]) {
      trace.camera_hold_active = false;
      const double now = std::chrono::duration<double>(
          std::chrono::steady_clock::now().time_since_epoch()).count();
      trace.pictorial_hold.Update(false, FloatAt(base, 0x833B4C68 + 28), now);
      Log("OWPROBE camera hold: aborted update={} identity_offset={}; native transition preserved",
          trace.frame, kCameraHoldIdentity[i]);
      return;
    }
  }
  const float native_timeline = FloatAt(base, object_ + 3540);
  for (size_t i = 0; i < clocks_.size(); ++i)
    REX_STORE_U32(object_ + kCameraHoldClocks[i], clocks_[i]);
  if (++trace.camera_hold_calls == 1 || trace.frame % 30 == 0)
    Log("OWPROBE camera hold: update={} object={:08X} timeline_native={:.3f} restored={:.3f} "
        "track_phase=({:.3f},{:.3f}) gate={:.3f} calls={}; native evaluator ran",
        trace.frame, object_, native_timeline, std::bit_cast<float>(clocks_[0]),
        std::bit_cast<float>(clocks_[2]), std::bit_cast<float>(clocks_[6]),
        std::bit_cast<float>(clocks_[10]), trace.camera_hold_calls);
}

CameraScriptScope::CameraScriptScope(const uint8_t* base, uint32_t object) {
  uint32_t table{}, callback{}, selected{}, scene{}, index{}, objects{};
  if (!Enabled() || trace.camera_events >= 256 ||
      !Readable(base, kScreen, 1) || base[kScreen] != 2 ||
      !Read32(base, kScreen + 68, table) || table != 0x82B83C80 ||
      !Read32(base, table + 2 * 36 + 4, callback) || callback != 0x825A2270 ||
      !Read32(base, 0x83DB9C54, phase_) || phase_ < 4 || phase_ > 10 ||
      !Read32(base, 0x83DB99CC, selected) || selected != 0x83DA61BC ||
      !Read32(base, selected, scene) || scene != 58 ||
      !Read32(base, kScene + 968, index) || index >= 8 ||
      !Read32(base, kScene + 928, objects) || !objects ||
      uint64_t(objects) + uint64_t(index) * 4144 != object ||
      !Readable(base, object, 4144)) return;
  for (size_t i = 0; i < kCameraScriptFields.size(); ++i)
    if (!Read32(base, object + kCameraScriptFields[i], before_[i])) return;
  if (before_[0]) {
    Read32(base, before_[0], entry_time_);
    if (before_[0] <= UINT32_MAX - 4) Read32(base, before_[0] + 4, entry_type_);
  }
  base_ = base;
  object_ = object;
}

CameraScriptScope::~CameraScriptScope() {
  if (!base_ || !Enabled() || !Readable(base_, object_, 4144)) return;
  std::array<uint32_t, 10> after{};
  for (size_t i = 0; i < kCameraScriptFields.size(); ++i)
    if (!Read32(base_, object_ + kCameraScriptFields[i], after[i])) return;
  const bool transition = before_[0] != after[0] || before_[4] != after[4] ||
      before_[5] != after[5] || before_[6] != after[6] || before_[7] != after[7] ||
      before_[8] != after[8] || before_[9] != after[9];
  const bool sample = trace.frame % 30 == 0 && trace.camera_sample != trace.frame;
  if (!transition && !sample) return;
  ++trace.camera_events;
  trace.camera_sample = trace.frame;
  Log("OWPROBE camera script: update={} phase={} object={:08X} transition={} "
      "pc={:08X}->{:08X} timeline={:.3f}->{:.3f} subframe={:.3f}->{:.3f} "
      "entry={}->{} target={}->{} mode={}->{} active={}->{} "
      "flags=({},{})->({},{}) entry_time={} entry_type={} "
      "script_frame={:.3f} script_elapsed={:.3f}",
      trace.frame, phase_, object_, transition, before_[0], after[0],
      std::bit_cast<float>(before_[1]), std::bit_cast<float>(after[1]),
      std::bit_cast<float>(before_[2]), std::bit_cast<float>(after[2]),
      before_[4], after[4], before_[5], after[5], before_[6], after[6],
      before_[7], after[7], before_[8], before_[9], after[8], after[9],
      int32_t(entry_time_), entry_type_, FloatAt(base_, 0x833B4C68 + 28),
      FloatAt(base_, 0x833B4C68 + 24));
}

ScriptCommandScope::ScriptCommandScope(const uint8_t* base, uint32_t state,
                                      uint32_t handler) {
  constexpr uint32_t script = 0x833B4C68;
  uint32_t id{}, table{}, callback{}, pointer{};
  if (!Enabled() || trace.script_commands >= 128 || state != script ||
      !Readable(base, kScreen, 1) ||
      !Read32(base, kScreen + 68, table)) {
    return;
  }
  id = base[kScreen];
  const bool vacation = id == 7 && table == 0x82AACFD0;
  const bool pictorial = id == 2 && table == 0x82B83C80;
  if ((!vacation && !pictorial) ||
      !Read32(base, table + id * 36 + 4, callback) ||
      callback != (vacation ? 0x826E2618u : 0x825A2270u)) return;
  if (pictorial) {
    uint32_t phase{};
    // The verified title pictorial controller starts the shared script in
    // phase 5, enters playback through 6/7, and begins ending in phase 8.
    if (!Read32(base, 0x83DB9C54, phase) || phase < 5 || phase > 8) return;
  }
  if (!Read32(base, script + 52, pointer) || !Readable(base, pointer, 2) ||
      !Readable(base, script + 138, 4)) {
    return;
  }
  uint16_t header{};
  std::memcpy(&header, base + pointer + REX_PHYS_HOST_OFFSET(pointer), 2);
  header = std::byteswap(header);
  const uint32_t words = header >> 10;
  if (words < 1 || !Readable(base, pointer, words * 2)) {
    return;
  }
  for (uint32_t i = 1; i < words && i <= 3; ++i) {
    uint16_t word{};
    std::memcpy(&word, base + pointer + REX_PHYS_HOST_OFFSET(pointer) + i * 2, 2);
    arguments_[i - 1] = std::byteswap(word);
  }
  for (uint32_t i = 0; i < 4; ++i) mapping_[i] = base[script + 138 + i];
  camera_before_ = ~0u;
  Read32(base, kScene + 968, camera_before_);
  before_ = ActorUpdateMask(base);
  handler_ = handler;
  pointer_ = pointer;
  header_ = header;
  words_ = words;
  base_ = base;
  ++trace.script_commands;
}

ScriptCommandScope::~ScriptCommandScope() {
  if (!base_) return;
  const uint8_t* base = base_;
  uint32_t camera_after = ~0u;
  Read32(base, kScene + 968, camera_after);
  std::array<uint32_t, 4> motion{};
  for (uint32_t i = 0; i < 4; ++i) {
    uint32_t word{};
    if (Read32(base, kPlayers + i * 152 + 52, word)) motion[i] = word >> 16;
    else motion[i] = ~0u;
  }
  Log("OWPROBE script command: update={} handler={:08X} pc={:08X} opcode={} "
      "words={} args_first3=({:04X},{:04X},{:04X}) map=({},{},{},{}) "
      "actor_mask={:X}->{:X} motion_after=({},{},{},{}) camera={}->{}",
      trace.frame, handler_, pointer_, header_ & 0x3FF, words_,
      arguments_[0], arguments_[1], arguments_[2], mapping_[0], mapping_[1],
      mapping_[2], mapping_[3], before_, ActorUpdateMask(base), motion[0], motion[1],
      motion[2], motion[3], camera_before_, camera_after);
}
}  // namespace doax2::open_world

REX_EXTERN(__imp__sub_824BC560);
REX_HOOK_RAW(sub_824BC560) {
  doax2::open_world::ScriptCommandScope observation(base, ctx.r3.u32, 0x824BC560);
  __imp__sub_824BC560(ctx, base);
}

// A finite lifetime experiment, confined to the verified script object and
// opcode-16 active-wait contract. The first trial holds +28; the coordinated
// pictorial lifetime trial also holds +24. Native animation, camera services,
// input, the original wait and normal cleanup continue.
REX_EXTERN(__imp__sub_824B9EA0);
REX_HOOK_RAW(sub_824B9EA0) {
  using namespace doax2::open_world;
  const bool idle_trial = REXCVAR_GET(doax2_open_world_pictorial_idle_trial) ||
                          REXCVAR_GET(doax2_open_world_pictorial_movement_trial);
  const bool long_actor_trial = REXCVAR_GET(doax2_open_world_pictorial_long_actor_trial) ||
                                REXCVAR_GET(doax2_open_world_pictorial_orbit_trial) ||
                                REXCVAR_GET(doax2_open_world_pictorial_idle_bank_trial) || idle_trial;
  const double duration = REXCVAR_GET(doax2_open_world_pictorial_movement_trial)
      ? (REXCVAR_GET(doax2_open_world_pictorial_exploration) ? 300.0 : 120.0)
      : (long_actor_trial ? 15.0 : 5.0);
  const bool actor_trial = REXCVAR_GET(doax2_open_world_pictorial_actor_trial) || long_actor_trial;
  const bool camera_trial = REXCVAR_GET(doax2_open_world_pictorial_camera_trial) || actor_trial;
  const bool lifetime_trial = REXCVAR_GET(doax2_open_world_pictorial_lifetime_trial) || camera_trial;
  if (REXCVAR_GET(doax2_open_world_pictorial_hold_trial) || lifetime_trial) {
    constexpr uint32_t script = 0x833B4C68;
    uint32_t table{}, callback{}, state{}, phase{}, selected{}, scene_id{}, pointer{}, header{}, camera{}, motion{}, camera_object{};
    const float frame = FloatAt(base, script + 28);
    const float elapsed_frame = FloatAt(base, script + 24);
    const bool eligible = ctx.r3.u32 == script &&
        Readable(base, kScreen, 1) && base[kScreen] == 2 &&
        Read32(base, kScreen + 68, table) && table == 0x82B83C80 &&
        Read32(base, table + 2 * 36 + 4, callback) && callback == 0x825A2270 &&
        Read32(base, kScreen + 40, state) && state == 1 &&
        Read32(base, 0x83DB9C54, phase) && phase == 7 &&
        Read32(base, 0x83DB99CC, selected) && selected == 0x83DA61BC &&
        Read32(base, selected, scene_id) && scene_id == 58 &&
        Readable(base, script, 1) && base[script] == 1 &&
        Read32(base, script + 52, pointer) && pointer == 0x82C2F758 &&
        Read32(base, pointer, header) && header == 0x081000D2 &&
        Read32(base, kScene + 968, camera) && camera == 0 &&
        Read32(base, kPlayers + 52, motion) && IdleActorExpected(base, motion >> 16) &&
        Readable(base, script + 138, 4) && base[script + 138] == 0 &&
        base[script + 139] == 1 && base[script + 140] == 2 && base[script + 141] == 3 &&
        ActorUpdateMask(base) == 1 && frame >= 30.0f && frame < 120.0f &&
        (!lifetime_trial || std::isfinite(elapsed_frame)) &&
        (!camera_trial || CameraHoldReady(base, camera_object)) &&
        (!actor_trial || trace.idle_actor_active || ActorTracksReady(base)) &&
        (!idle_trial || (!trace.idle_actor_done && IdleResourcesReady(base) &&
                        FloatAt(base, kPlayers + 116) == 0));
    const double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    const auto decision = trace.pictorial_hold.Update(eligible, frame, now, duration);
    if (decision.action == HoldAction::Begin) {
      trace.pictorial_elapsed_anchor = elapsed_frame;
      trace.camera_hold_object = camera_object;
      trace.camera_hold_until = now + duration;
    }
    trace.camera_hold_active = camera_trial &&
        (decision.action == HoldAction::Begin || decision.action == HoldAction::Hold);
    if (decision.action == HoldAction::Begin && idle_trial) StartIdleActor(base);
    if (decision.action == HoldAction::Release) RestoreIdleActor(base);
    if (decision.action == HoldAction::Begin || decision.action == HoldAction::Release) {
      Log("OWPROBE pictorial hold: {} update={} pc={:08X} frame={:.3f} "
          "anchor={:.3f} elapsed={:.3f}s eligible={} script_elapsed={:.3f} coordinated={} duration={:.1f}s",
          decision.action == HoldAction::Begin ? "begin" : "release", trace.frame,
          pointer, frame, decision.frame, decision.seconds, eligible, elapsed_frame, lifetime_trial, duration);
    }
    if (decision.action == HoldAction::Begin || decision.action == HoldAction::Hold) {
      REX_STORE_U32(script + 28, std::bit_cast<uint32_t>(decision.frame));
      if (lifetime_trial)
        REX_STORE_U32(script + 24, std::bit_cast<uint32_t>(trace.pictorial_elapsed_anchor));
    }
  }
  if (REXCVAR_GET(doax2_open_world_relaxation_hold_trial) && !trace.hold_released) {
    constexpr uint32_t script = 0x833B4C68;
    uint32_t table{}, state{}, callback{}, phase{}, pointer{}, camera{}, motion{}, header{};
    const float frame = FloatAt(base, script + 28);
    const bool eligible = ctx.r3.u32 == script && Readable(base, kScreen, 1) &&
        base[kScreen] == 7 && Read32(base, kScreen + 68, table) && table == 0x82AACFD0 &&
        Read32(base, table + 7 * 36 + 4, callback) && callback == 0x826E2618 &&
        Read32(base, kScreen + 40, state) && state == 1 &&
        Read32(base, 0x83975920, phase) && phase == 3 &&
        Readable(base, script, 1) && base[script] == 1 &&
        Read32(base, script + 52, pointer) &&
        pointer >= 0x82C00000 && pointer <= 0x82C3FFFC &&
        // Opcode 16, two halfwords, positive future absolute frame target.
        Readable(base, pointer, 4) &&
        (std::memcpy(&header, base + pointer, 4), header = std::byteswap(header),
            (header >> 16) == 0x0810 && (header & 0xFFFF) >= 40 &&
            (header & 0xFFFF) <= 600 && frame + 5.0f < float(header & 0xFFFF)) &&
        Read32(base, kScene + 968, camera) && camera == 0 &&
        Read32(base, kPlayers + 52, motion) && (motion >> 16) < 8192 &&
        Readable(base, script + 138, 4) && base[script + 138] == 0 &&
        base[script + 139] == 1 && base[script + 140] == 2 && base[script + 141] == 3 &&
        ActorUpdateMask(base) == 1 && frame >= 30.0f && frame < 120.0f;
    const auto now = std::chrono::steady_clock::now();
    if (eligible && !trace.hold_started) {
      trace.hold_started = true;
      trace.hold_started_at = now;
      trace.hold_frame = frame;
      Log("OWPROBE hold trial: begin update={} pc={:08X} frame={:.3f} seconds=5",
          trace.frame, pointer, frame);
    }
    if (trace.hold_started) {
      const double elapsed = std::chrono::duration<double>(now - trace.hold_started_at).count();
      if (!eligible || elapsed >= 5.0) {
        trace.hold_released = true;
        Log("OWPROBE hold trial: release update={} elapsed={:.3f}s eligible={} frame={:.3f}",
            trace.frame, elapsed, eligible, frame);
      } else {
        REX_STORE_U32(script + 28, std::bit_cast<uint32_t>(trace.hold_frame));
      }
    }
  }
  __imp__sub_824B9EA0(ctx, base);
}
