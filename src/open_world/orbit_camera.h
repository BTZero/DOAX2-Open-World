#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace doax2::open_world {
// Small, fixed-radius orbit around the admitted native shot's target.
// Deliberately bounded until camera obstruction queries are established.
struct OrbitCamera {
  std::array<float, 3> target{};
  float radius{}, yaw{}, pitch{}, yaw_anchor{}, pitch_anchor{};
  bool Begin(const std::array<float, 3>& eye, const std::array<float, 3>& aim) {
    for (int i = 0; i < 3; ++i)
      if (!std::isfinite(eye[i]) || !std::isfinite(aim[i]) ||
          std::abs(eye[i]) > 10000 || std::abs(aim[i]) > 10000) return false;
    const float x = eye[0] - aim[0], y = eye[1] - aim[1], z = eye[2] - aim[2];
    radius = std::sqrt(x*x + y*y + z*z);
    if (!std::isfinite(radius) || radius < 0.25f || radius > 100) return false;
    pitch = std::asin(std::clamp(y / radius, -1.0f, 1.0f));
    if (std::abs(pitch) > 1.2f) return false;
    yaw_anchor = yaw = std::atan2(x, z);
    pitch_anchor = pitch;
    target = aim;
    return true;
  }
  void Step(float x, float y, double seconds) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(seconds) || seconds < 0) return;
    const float magnitude = std::hypot(x, y);
    if (magnitude <= 0.2f) return;
    const float scale = (std::min(magnitude, 1.0f) - 0.2f) / (0.8f * magnitude);
    const float step = float(std::min(seconds, 0.1)) * 0.8f;
    yaw = std::clamp(yaw + x * scale * step, yaw_anchor - 0.6f, yaw_anchor + 0.6f);
    pitch = std::clamp(pitch + y * scale * step,
                       std::max(-1.2f, pitch_anchor - 0.25f),
                       std::min(1.2f, pitch_anchor + 0.25f));
  }
  std::array<float, 3> Eye() const {
    const float horizontal = radius * std::cos(pitch);
    return {target[0] + horizontal * std::sin(yaw),
            target[1] + radius * std::sin(pitch),
            target[2] + horizontal * std::cos(yaw)};
  }
};
}  // namespace doax2::open_world
