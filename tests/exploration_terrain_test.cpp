#include "../src/open_world/exploration_terrain.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace doax2::open_world;
void Check(bool v) { if (!v) throw std::runtime_error("Terrain guard check failed"); }
int main() {
  try {
    ExplorationTerrain t;
    t.triangles = {{{-2,0,-2},{2,0,-2},{2,0,2},1},{{-2,0,-2},{2,0,2},{-2,0,2},1}};
    t.admitted_flags = 1; t.minimum_y = -.5f;
    float y{};
    Check(t.Supported(0,0,0,y) && y == 0);
    Check(!t.Supported(1.95f,0,0,y)); // footprint over an edge
    Check(!t.Supported(3,0,0,y));
    Check(!t.Supported(0,0,.2f,y)); // discontinuous layer
    TerrainRejection reason{};
    Check(!t.Supported(0,0,.2f,y,&reason) && reason == TerrainRejection::HeightChange);
    Check(!t.Supported(3,0,0,y,&reason) && reason == TerrainRejection::Missing);
    Check(!t.Supported(std::numeric_limits<float>::quiet_NaN(),0,0,y));
    t.admitted_flags = 2;
    Check(!t.Supported(0,0,0,y,&reason) && reason == TerrainRejection::Flags);
    t.match_flags = false;
    Check(t.Supported(0,0,0,y,&reason)); // crossing a material seam is allowed
    Check(!t.Supported(3,0,0,y,&reason)); // missing terrain still rejects travel
    t.match_flags = true;
    t.admitted_flags = 1;
    Check(t.Clip({0,1,0},{0,-1,0}) == .5f);
    Check(t.Clip({0,-1,0},{0,1,0}) == .5f); // both face orientations
    Check(t.Clip({0,1,0},{1,1,0}) == 1);
    t.triangles.push_back({{1,0,-1},{1,2,-1},{1,0,1},1});
    Check(t.Clip({0,.5f,0},{2,.5f,0}) == .5f); // vertical obstruction
    t.triangles = {{{-2,-2,-2},{2,2,-2},{2,2,2},1},{{-2,-2,-2},{2,2,2},{-2,-2,2},1}};
    Check(!t.Supported(0,0,0,y,&reason) && reason == TerrainRejection::Slope);
    t.triangles = {{{-2,-.4f,-2},{2,.4f,-2},{2,.4f,2},1},{{-2,-.4f,-2},{2,.4f,2},{-2,-.4f,2},1}};
    Check(t.Supported(.5f,0,.1f,y) && std::abs(y-.1f)<.0001f);
    t.minimum_y = .2f;
    Check(!t.Supported(.5f,0,.1f,y,&reason) && reason == TerrainRejection::MinimumHeight);

    // A thin obstacle beside the center ray must still retract the camera.
    t.triangles = {{{.08f,.8f,1},{.2f,.8f,1},{.12f,1.2f,1},1}};
    Check(t.Clip({0,1,0},{0,1,2}) == 1);
    Check(std::abs(t.CameraClip({0,1,0},{0,1,2})-.5f)<.0001f);
    t.triangles = {{{-.2f,1.08f,1},{.2f,1.08f,1},{0,1.2f,1},1}};
    Check(t.Clip({0,1,0},{0,1,2}) == 1);
    Check(std::abs(t.CameraClip({0,1,0},{0,1,2})-.5f)<.0001f);
    Check(t.CameraClip({0,1,0},{0,1,0}) == 0);

    TerrainPoint previous{0,0,0}, proposed{1,0,1};
    Check(SlidePlanar(previous,proposed,[](TerrainPoint& p) {
      if (p[2]>.2f) return false;
      p[1]=.1f; return true;
    }));
    Check(proposed == TerrainPoint({1,.1f,0}));
    proposed={.5f,0,1};
    Check(SlidePlanar(previous,proposed,[](TerrainPoint&) { return true; }));
    Check(proposed == TerrainPoint({0,0,1})); // preserve larger valid component
    proposed={1,0,1};
    Check(!SlidePlanar(previous,proposed,[](TerrainPoint&) { return false; }));
    Check(proposed == previous); // both unsupported: stay at owned pose
    proposed=previous;
    Check(!SlidePlanar(previous,proposed,[](TerrainPoint&) { throw std::runtime_error("zero step queried"); return true; }));
    std::cout << "Exploration terrain checks passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
