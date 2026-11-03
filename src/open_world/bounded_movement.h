#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace doax2::open_world {
// Camera-relative planar proposal. The caller validates ground/obstructions
// before committing it; the radial bound alone establishes no terrain contract.
struct BoundedMovement {
  static constexpr float kRadius = 1.25f;
  static constexpr float kSpeed = 3.f;
  static constexpr float kMaximumRadius = 30000.f;
  static constexpr float kTau = 2 * std::numbers::pi_v<float>;
  float anchor_x{}, anchor_z{}, heading{};
  float radius = kRadius;
  bool active{};

  bool Begin(float x, float z, uint32_t facing) {
    active = std::isfinite(x) && std::isfinite(z) &&
        std::abs(x) < 10000 && std::abs(z) < 10000;
    anchor_x = x;
    anchor_z = z;
    heading = float(facing & 65535) * kTau / 65536.0f;
    return active;
  }
  bool Step(float& x, float& z, float stick_x, float stick_y,
            float camera_yaw, double seconds) {
    if (!active || !std::isfinite(x) || !std::isfinite(z) ||
        !std::isfinite(stick_x) || !std::isfinite(stick_y) ||
        !std::isfinite(camera_yaw) || !std::isfinite(seconds) || seconds < 0 ||
        !std::isfinite(radius) || radius <= 0 || radius > kMaximumRadius ||
        std::hypot(x - anchor_x, z - anchor_z) > radius + 0.001f) return false;
    const float magnitude = std::hypot(stick_x, stick_y);
    if (magnitude <= 0.18f) return true;
    const float strength = (std::min(magnitude, 1.0f) - 0.18f) / 0.82f;
    const float dx = (stick_x * std::cos(camera_yaw) - stick_y * std::sin(camera_yaw)) / magnitude;
    const float dz = (-stick_x * std::sin(camera_yaw) - stick_y * std::cos(camera_yaw)) / magnitude;
    const float dt = float(std::min(seconds, 0.05));
    // Native 8268E5F8 computes facing as atan2(-delta_z, delta_x),
    // scaled to 65536 units per turn. Follow that convention.
    const float desired = std::atan2(-dz, dx);
    const float difference = std::remainder(desired - heading, kTau);
    heading = std::remainder(heading + std::clamp(difference, -kTau * dt, kTau * dt), kTau);
    float next_x = x + dx * kSpeed * strength * dt;
    float next_z = z + dz * kSpeed * strength * dt;
    const float distance = std::hypot(next_x - anchor_x, next_z - anchor_z);
    if (distance > radius) {
      next_x = anchor_x + (next_x - anchor_x) * radius / distance;
      next_z = anchor_z + (next_z - anchor_z) * radius / distance;
    }
    x = next_x;
    z = next_z;
    return true;
  }
  uint32_t Facing() const {
    return uint32_t(int32_t(heading * 65536.0f / kTau)) & 65535;
  }
};
}  // namespace doax2::open_world
