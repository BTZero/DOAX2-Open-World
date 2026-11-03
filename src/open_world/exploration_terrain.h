#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace doax2::open_world {
using TerrainPoint = std::array<float, 3>;
struct TerrainTriangle {
  TerrainPoint a, b, c;
  uint16_t flags{};
};
struct TerrainSurface { float height{}, slope{}; uint16_t flags{}; };

// A conservative geometric guard, not a replacement for native collision.
// Flags are compared with the admitted spawn, without assigning material names.
struct ExplorationTerrain {
  std::vector<TerrainTriangle> triangles;
  uint16_t admitted_flags{};
  float minimum_y{};

  bool Surface(float x, float z, float reference_y, TerrainSurface& result) const {
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(reference_y)) return false;
    float closest = std::numeric_limits<float>::infinity();
    for (const auto& t : triangles) {
      const float ux = t.b[0]-t.a[0], uy = t.b[1]-t.a[1], uz = t.b[2]-t.a[2];
      const float vx = t.c[0]-t.a[0], vy = t.c[1]-t.a[1], vz = t.c[2]-t.a[2];
      const float d = ux*vz-uz*vx;
      if (std::abs(d) < 1e-7f) continue;
      const float b = ((x-t.a[0])*vz-(z-t.a[2])*vx)/d;
      const float c = (ux*(z-t.a[2])-uz*(x-t.a[0]))/d;
      if (b < -1e-5f || c < -1e-5f || b+c > 1.00001f) continue;
      const float y = t.a[1]+b*uy+c*vy;
      const float difference = std::abs(y-reference_y);
      if (!std::isfinite(y) || difference >= closest) continue;
      const float nx = uy*vz-uz*vy, nz = ux*vy-uy*vx;
      result = {y, std::hypot(nx,nz)/std::abs(d), t.flags};
      closest = difference;
    }
    return std::isfinite(closest);
  }

  bool Supported(float x, float z, float previous_y, float& height) const {
    // Check center and four points around a 0.15-unit footprint. Missing,
    // steep, different-flag or sharply discontinuous ground rejects the step.
    for (auto offset : std::array<std::array<float,2>,5>{{{0,0},{.15f,0},{-.15f,0},{0,.15f},{0,-.15f}}}) {
      TerrainSurface hit;
      if (!Surface(x+offset[0], z+offset[1], previous_y, hit) ||
          hit.flags != admitted_flags || hit.slope > .6f || hit.height < minimum_y ||
          std::abs(hit.height-previous_y) > .15f) return false;
      if (offset[0] == 0 && offset[1] == 0) height = hit.height;
    }
    return true;
  }

  // Segment/triangle test includes vertical triangles, unlike height sampling.
  // Return a fraction of the segment, or 1 if unobstructed.
  float Clip(const TerrainPoint& from, const TerrainPoint& to) const {
    const auto sub = [](auto a, auto b) { return TerrainPoint{a[0]-b[0],a[1]-b[1],a[2]-b[2]}; };
    const auto cross = [](auto a, auto b) { return TerrainPoint{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; };
    const auto dot = [](auto a, auto b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; };
    const auto direction = sub(to,from);
    float fraction = 1;
    for (const auto& t : triangles) {
      const auto u = sub(t.b,t.a), v = sub(t.c,t.a), p = cross(direction,v);
      const float determinant = dot(u,p);
      if (std::abs(determinant) < 1e-7f) continue;
      const auto distance = sub(from,t.a);
      const float b = dot(distance,p)/determinant;
      const auto q = cross(distance,u);
      const float c = dot(direction,q)/determinant;
      const float f = dot(v,q)/determinant;
      if (b >= 0 && c >= 0 && b+c <= 1 && f > .0001f && f < fraction) fraction = f;
    }
    return fraction;
  }
};
}  // namespace doax2::open_world
