#pragma once

#include <cmath>
#include <cstdint>

namespace doax2::open_world {
inline bool ActorTrackCanHold(float phase, uint32_t end) {
  return std::isfinite(phase) && phase >= 1 && phase + 2 < float(end);
}
inline bool CameraTrackCanHold(uint32_t type, uint32_t limit, float phase, float remaining) {
  return type >= 1 && type <= 8 && limit <= 4096 && std::isfinite(phase) &&
      std::isfinite(remaining) && phase >= 1 && phase + 1 < float(limit) && remaining > 2;
}
inline bool CameraGateCanHold(float gate) {
  return std::isfinite(gate) && (gate > 2 || gate < 0);
}
enum class HoldAction { Idle, Begin, Hold, Release };
struct HoldDecision {
  HoldAction action = HoldAction::Idle;
  float frame = 0;
  double seconds = 0;
};

// A single bounded experiment per process/thread. The caller supplies
// steady-clock seconds and applies the anchor only for Begin/Hold decisions.
class FiniteScriptHold {
 public:
  HoldDecision Update(bool eligible, float frame, double now, double duration = 5.0) {
    if (released_) return {};
    const bool valid = std::isfinite(frame) && std::isfinite(now);
    if (!started_) {
      if (!eligible || !valid || !std::isfinite(duration) || duration <= 0 || duration > 300) return {};
      duration_ = duration;
      started_ = true;
      start_ = now;
      anchor_ = frame;
      return {HoldAction::Begin, anchor_, 0};
    }
    const double seconds = std::isfinite(now) && now >= start_ ? now - start_ : 0;
    if (!eligible || !valid || now < start_ || seconds >= duration_) {
      released_ = true;
      return {HoldAction::Release, anchor_, seconds};
    }
    return {HoldAction::Hold, anchor_, seconds};
  }

 private:
  bool started_ = false;
  bool released_ = false;
  double start_ = 0;
  double duration_ = 5;
  float anchor_ = 0;
};
}  // namespace doax2::open_world
