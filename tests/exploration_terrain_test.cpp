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
    Check(!t.Supported(std::numeric_limits<float>::quiet_NaN(),0,0,y));
    t.admitted_flags = 2;
    Check(!t.Supported(0,0,0,y));
    t.admitted_flags = 1;
    Check(t.Clip({0,1,0},{0,-1,0}) == .5f);
    Check(t.Clip({0,-1,0},{0,1,0}) == .5f); // both face orientations
    Check(t.Clip({0,1,0},{1,1,0}) == 1);
    t.triangles.push_back({{1,0,-1},{1,2,-1},{1,0,1},1});
    Check(t.Clip({0,.5f,0},{2,.5f,0}) == .5f); // vertical obstruction
    t.triangles = {{{-2,-2,-2},{2,2,-2},{2,2,2},1},{{-2,-2,-2},{2,2,2},{-2,-2,2},1}};
    Check(!t.Supported(0,0,0,y)); // steep slope
    t.triangles = {{{-2,-.4f,-2},{2,.4f,-2},{2,.4f,2},1},{{-2,-.4f,-2},{2,.4f,2},{-2,-.4f,2},1}};
    Check(t.Supported(.5f,0,.1f,y) && std::abs(y-.1f)<.0001f);
    t.minimum_y = .2f;
    Check(!t.Supported(.5f,0,.1f,y));
    std::cout << "Exploration terrain checks passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
