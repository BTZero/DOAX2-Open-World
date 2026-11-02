#include "scene_probe.h"

#include "generated/default/doax2_pch.h"

#include <array>
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
  uint32_t pictorial_unlock_seen{};
  uint32_t pictorial_clips_seen[9]{};
  bool hold_started{}, hold_released{};
  float hold_frame{};
  std::chrono::steady_clock::time_point hold_started_at{};
  std::chrono::steady_clock::time_point started{};
};
thread_local Trace trace;

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
  Log("OWPROBE v7 enabled: terrain_trial={} relaxation_hold_trial={} pictorial_unlock={}; raw guest identities; "
      "sample interval=30 updates line_limit={} thread={}",
      REXCVAR_GET(doax2_open_world_terrain_trial),
      REXCVAR_GET(doax2_open_world_relaxation_hold_trial),
      REXCVAR_GET(doax2_open_world_pictorial_unlock), kLineLimit, GetCurrentThreadId());
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
  // Screen 7's verified descriptor passes this state block as r4 to
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
        uint32_t actor_mask{};
        for (uint32_t slot = 0; slot < 4; ++slot) {
          if (Readable(base, kPlayers + slot * 152 + 10, 1) &&
              base[kPlayers + slot * 152 + 10]) actor_mask |= 1u << slot;
        }
        Read32(base, kScene + 968, camera_index);
        Log("OWPROBE pictorial: update={} screen_state={} phase={} selected={:08X} "
            "scene_valid={} scene_id={} script_frame={:.3f} actor_update_mask={:X} camera_index={}",
            trace.frame, trace.screen.state, phase, selected, have_scene,
            have_scene ? scene_id : ~0u, FloatAt(base, 0x833B4C68 + 28), actor_mask, camera_index);
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

// The native rig pipeline calls the height method, then animation/rig services.
// Slot validity is established by observing this actual native call, not by
// inferring occupancy from nonzero coordinates in a cached player record.
void doax2::open_world::ObserveRig(const uint8_t* base, uint32_t slot) {
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

namespace doax2::open_world {
namespace {
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

ScriptCommandScope::ScriptCommandScope(const uint8_t* base, uint32_t state,
                                      uint32_t handler) {
  constexpr uint32_t script = 0x833B4C68;
  uint32_t id{}, table{}, callback{}, pointer{};
  if (!Enabled() || trace.script_commands >= 128 || state != script ||
      !Readable(base, kScreen, 1) || (id = base[kScreen]) != 7 ||
      !Read32(base, kScreen + 68, table) || table != 0x82AACFD0 ||
      !Read32(base, table + id * 36 + 4, callback) || callback != 0x826E2618 ||
      !Read32(base, script + 52, pointer) || !Readable(base, pointer, 2) ||
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
// opcode-16 active-wait contract. Only its +28 clock is held; native animation,
// camera services, input, the original wait, and normal cleanup continue.
REX_EXTERN(__imp__sub_824B9EA0);
REX_HOOK_RAW(sub_824B9EA0) {
  using namespace doax2::open_world;
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
