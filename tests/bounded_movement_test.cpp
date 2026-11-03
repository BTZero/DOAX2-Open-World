#include "../src/open_world/bounded_movement.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using doax2::open_world::BoundedMovement;
void Check(bool value) { if (!value) throw std::runtime_error("Movement check failed"); }
bool Near(float a, float b) { return std::abs(a - b) < 0.0001f; }
int main() {
  try {
    BoundedMovement a, b;
    Check(a.Begin(0, 0, 0) && b.Begin(0, 0, 0));
    a.radius = b.radius = 4;
    float ax = 0, az = 0, bx = 0, bz = 0;
    for (int i = 0; i < 30; ++i) Check(a.Step(ax, az, 1, 0, 0, 1.0 / 30));
    for (int i = 0; i < 60; ++i) Check(b.Step(bx, bz, 1, 0, 0, 1.0 / 60));
    Check(Near(ax, 3.f) && Near(az, 0) && Near(ax, bx) && Near(az, bz));
    Check(a.Step(ax, az, 0, 0, 0, 0.033) && Near(ax, 3.f));
    for (int i = 0; i < 300; ++i) Check(a.Step(ax, az, 1, 0, 0, 1.0 / 30));
    Check(Near(ax, a.radius));
    Check(a.Step(ax, az, -1, 0, 0, 1.0 / 30) && ax < a.radius);

    // Diagonal input cannot increase speed, and camera forward is away from eye.
    Check(a.Begin(0, 0, 0)); ax = az = 0;
    Check(a.Step(ax, az, 1, 1, 0, 0.05));
    Check(Near(std::hypot(ax, az), 3.f * 0.05f) && ax > 0 && az < 0);
    Check(a.Begin(0, 0, 0)); ax = az = 0;
    Check(a.Step(ax, az, 0, 1, BoundedMovement::kTau / 4, 0.05));
    Check(ax < 0 && Near(az, 0));
    Check(a.Begin(0, 0, 0)); ax = az = 0;
    Check(a.Step(ax, az, 0.59f, 0, 0, 0.05));
    Check(Near(ax, 3.f * 0.5f * 0.05f));
    Check(a.Step(ax, az, 0.1f, 0.1f, 0, 0.05));
    Check(Near(ax, 3.f * 0.5f * 0.05f));

    // Turn across the wrap by the short route, at a bounded angular rate.
    Check(a.Begin(0, 0, 65500)); ax = az = 0;
    Check(a.Step(ax, az, 1, 0, 0, 1.0 / 30));
    Check(a.Facing() == 0);
    Check(a.Begin(0, 0, 0)); ax = az = 0;
    Check(a.Step(ax, az, -1, 0, 0, 0.01));
    Check(std::abs(a.heading) <= BoundedMovement::kTau * 0.01001f);

    // A stall cannot produce a catch-up teleport; invalid input never mutates pose.
    Check(a.Begin(0, 0, 0)); ax = az = 0;
    Check(a.Step(ax, az, 1, 0, 0, 5));
    Check(Near(ax, 3.f * 0.05f));
    const float before = ax;
    Check(!a.Step(ax, az, std::numeric_limits<float>::quiet_NaN(), 0, 0, 0.03));
    Check(Near(ax, before));
    Check(!a.Step(ax, az, 1, 0, 0, -1));
    ax = a.radius+1;
    Check(!a.Step(ax, az, 1, 0, 0, 0.03) && ax == a.radius+1);
    Check(!a.Begin(std::numeric_limits<float>::infinity(), 0, 0));
    Check(a.Begin(0,0,0)); ax = az = 0;
    a.radius = 100;
    for (int i=0;i<2000;++i) Check(a.Step(ax,az,1,0,0,.05));
    Check(Near(ax,100) && Near(az,0));
    Check(a.Step(ax,az,-1,0,0,.05) && ax < 100);
    a.radius = BoundedMovement::kMaximumRadius+1;
    Check(!a.Step(ax,az,1,0,0,.05));
    std::cout << "Bounded movement checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
