#include "../src/open_world/orbit_camera.h"
#include <iostream>
#include <limits>
#include <stdexcept>

void Check(bool value) { if (!value) throw std::runtime_error("Orbit check failed"); }
int main() {
  using doax2::open_world::OrbitCamera;
  try {
    OrbitCamera a;
    Check(a.Begin({8, 3, 7}, {-2, 1, 9}));
    auto eye = a.Eye();
    Check(std::abs(eye[0] - 8) < 0.0001f && std::abs(eye[1] - 3) < 0.0001f &&
          std::abs(eye[2] - 7) < 0.0001f);
    const float yaw = a.yaw, pitch = a.pitch;
    a.Step(0.1f, 0.1f, 0.033);
    Check(a.yaw == yaw && a.pitch == pitch);
    OrbitCamera b = a;
    for (int i = 0; i < 9; ++i) a.Step(1, 0, 1.0 / 30);
    for (int i = 0; i < 18; ++i) b.Step(1, 0, 1.0 / 60);
    Check(std::abs(a.yaw - b.yaw) < 0.00001f);
    Check(std::abs(a.yaw - yaw - 0.24f) < 0.00001f);
    OrbitCamera hitch = a;
    hitch.Step(1, 0, 10);
    Check(std::abs(hitch.yaw - a.yaw - 0.08f) < 0.00001f);
    for (int i = 0; i < 500; ++i) a.Step(1, 1, 0.033);
    Check(a.yaw <= yaw + 0.60001f && a.pitch <= pitch + 0.25001f);
    eye = a.Eye();
    Check(std::abs(std::hypot(eye[0] - a.target[0], eye[1] - a.target[1],
                              eye[2] - a.target[2]) - a.radius) < 0.0001f);
    const auto before = eye;
    a.Step(std::numeric_limits<float>::infinity(), 0, 1);
    a.Step(1, 1, -1);
    Check(a.Eye() == before);
    Check(!b.Begin({0, 0, 0}, {0, 0, 0}));
    Check(!b.Begin({0, 1000, 0}, {0, 0, 0}));
    Check(!b.Begin({0, 2, 0}, {0, 0, 0}));
    Check(!b.Begin({std::numeric_limits<float>::quiet_NaN(), 0, 0}, {0, 0, 0}));
    std::cout << "Orbit camera checks passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
