#pragma once

#include <algorithm>
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
enum class TerrainRejection { None, Missing, Flags, Slope, MinimumHeight, HeightChange, Obstruction, NativeHeight };
inline const char* RejectionName(TerrainRejection reason) {
  switch (reason) {
    case TerrainRejection::None: return "none";
    case TerrainRejection::Missing: return "missing_ground";
    case TerrainRejection::Flags: return "surface_flags";
    case TerrainRejection::Slope: return "slope";
    case TerrainRejection::MinimumHeight: return "minimum_height";
    case TerrainRejection::HeightChange: return "height_change";
    case TerrainRejection::Obstruction: return "obstruction";
    case TerrainRejection::NativeHeight: return "native_height";
  }
  return "unknown";
}

// A conservative geometric guard, not a replacement for native collision.
// Flags are compared with the admitted spawn, without assigning material names.
struct ExplorationTerrain {
  std::vector<TerrainTriangle> triangles;
  uint16_t admitted_flags{};
  bool match_flags = true;
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

  bool Supported(float x, float z, float previous_y, float& height,
                 TerrainRejection* rejection = nullptr) const {
    if (rejection) *rejection = TerrainRejection::None;
    // Check center and four points around a 0.15-unit footprint. Missing,
    // steep, different-flag or sharply discontinuous ground rejects the step.
    for (auto offset : std::array<std::array<float,2>,5>{{{0,0},{.15f,0},{-.15f,0},{0,.15f},{0,-.15f}}}) {
      TerrainSurface hit;
      TerrainRejection reason = TerrainRejection::None;
      if (!Surface(x+offset[0], z+offset[1], previous_y, hit)) reason = TerrainRejection::Missing;
      else if (match_flags && hit.flags != admitted_flags) reason = TerrainRejection::Flags;
      else if (hit.slope > .6f) reason = TerrainRejection::Slope;
      else if (hit.height < minimum_y) reason = TerrainRejection::MinimumHeight;
      else if (std::abs(hit.height-previous_y) > .15f) reason = TerrainRejection::HeightChange;
      if (reason != TerrainRejection::None) {
        if (rejection) *rejection = reason;
        return false;
      }
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

  // Five parallel rays cover the camera center and an offset cross around it.
  // This is a sampled clearance test against this mesh, not a sphere cast.
  float CameraClip(const TerrainPoint& from, const TerrainPoint& to) const {
    const float dx=to[0]-from[0], dy=to[1]-from[1], dz=to[2]-from[2];
    const float distance=std::hypot(dx,dy,dz), horizontal=std::hypot(dx,dz);
    if (!std::isfinite(distance) || distance < .0001f) return 0;
    const TerrainPoint right = horizontal > .0001f
        ? TerrainPoint{dz/horizontal,0,-dx/horizontal} : TerrainPoint{1,0,0};
    const TerrainPoint up{(dy*right[2])/distance,
        (dz*right[0]-dx*right[2])/distance,(-dy*right[0])/distance};
    float fraction=Clip(from,to);
    for (const auto& axis : {right,up}) {
      for (float sign : {-1.f,1.f}) {
        TerrainPoint a=from,b=to;
        for (int i=0;i<3;++i) { a[i]+=axis[i]*sign*.12f; b[i]+=axis[i]*sign*.12f; }
        fraction=std::min(fraction,Clip(a,b));
      }
    }
    return fraction;
  }
};

// Try the larger component first. Each fallback must pass the same terrain,
// native-height and radial checks as the full proposal. No extra travel is added.
template <typename Validate>
bool SlidePlanar(const TerrainPoint& previous, TerrainPoint& proposed, Validate validate) {
  const bool x_first=std::abs(proposed[0]-previous[0]) >= std::abs(proposed[2]-previous[2]);
  for (int axis : {x_first ? 0 : 2, x_first ? 2 : 0}) {
    if (std::abs(proposed[axis]-previous[axis]) < .000001f) continue;
    TerrainPoint candidate=previous;
    candidate[axis]=proposed[axis];
    if (validate(candidate)) { proposed=candidate; return true; }
  }
  proposed=previous;
  return false;
}
}  // namespace doax2::open_world
