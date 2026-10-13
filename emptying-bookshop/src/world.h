// The world: light by time of day, the campus diorama, its crowd, Jia, and the camera rig.
// Everything here is a pure function of master time t, so any frame can be rendered on its own.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "kit.h"

namespace film {

// Key places on the campus (x, z). North is -z.
namespace P {
inline const vec3 shop{17, 0, -25}, shopDoor{14.6f, 0, -21.6f}, library{0, 0, -16}, libraryFront{0, 0, -10.6f}, oldShop{7, 0, 4.2f},
    teach1Door{-18.6f, 0, 0}, teach2Door{-18.6f, 0, 7}, dorm1Door{-7, 0, 20.4f}, dorm2Door{11, 0, 20.4f}, cafeDoor{19.9f, 0, 2},
    quad{-6, 0, -4};
constexpr float shopFront = -22.25f;
}  // namespace P

// Side path from the library plaza to the shop door.
inline const std::vector<std::pair<float, float>> SIDE_PATH = {{4.8f, -10.4f}, {11, -10.4f}, {14.6f, -13.6f}, {14.6f, -21.6f}};

// Camera: arrive at pose (p looking at l) by time t, moving over d seconds. While a shot holds, the
// camera pushes in slightly (capped by `push`). `arc` lifts the move in the middle.
struct CamKey {
  float t;
  float d = 2;
  vec3 p, l;
  float fov = 40;
  float push = 0.035f;
  float arc = 0;
};
// Jia: be at (x, z) facing ry by time t, walking there from the previous key.
struct JiaKey {
  float t;
  float x = 0, z = 0, y = 0, ry = 0, look = 0, nod = 0;
  bool hide = false;
  bool walk = true;
  float dur = -1;
};
// Light: arrive at preset p by time t, blending over d seconds.
struct LightKey {
  float t;
  const char* p;
  float d;
};
// Crowd level (0..1) by time t, blending over d seconds.
struct CrowdKey {
  float t, v, d;
};
// Shop state from time t: the door sign, the shop sign (faded 0 → bright 1), the interior light (0..1).
struct ShopKey {
  float t;
  std::optional<std::string> door;
  std::optional<float> lit, sign;
};

// Per-frame screen effects. Chapters adjust these; applyFx() hands them to the renderer.
struct Fx {
  float sat = 1, fade = 0, flash = 0;
  std::optional<vec3> focus;
  float focusR = 0.2f, focusAmt = 0;
};

struct World {
  Scene scene;
  Camera camera;
  PostSettings post;
  Fx fx;
  float night = 0;

  std::vector<CamKey> camKeys;
  std::vector<JiaKey> jiaKeys;
  std::vector<LightKey> lightKeys;
  std::vector<CrowdKey> crowdKeys;
  std::vector<ShopKey> shopKeys;
  std::vector<std::tuple<float, float, float>> warps;
  std::vector<std::function<void(float)>> updaters;

  std::vector<Material*> nightMats, lamps;
  Material* starMat = nullptr;
  Node* campus = nullptr;
  Node* libraryGroup = nullptr;

  struct Shop {
    Node* group = nullptr;
    Sign* sign = nullptr;
    Sign* doorSign = nullptr;
    Material* glass = nullptr;
    PointLight* lamp = nullptr;
  } shop;

  struct Walker {
    int route;
    float phase, speed, side, gate, bob;
  };
  struct Crowd {
    InstancedMesh* bodies = nullptr;
    InstancedMesh* heads = nullptr;
    std::vector<Walker> walkers;
    std::vector<const Curve*> curves;
    std::vector<float> lens;
  } crowd;

  Figure jia;

  void addCam(std::initializer_list<CamKey> k) { camKeys.insert(camKeys.end(), k); }
  void addJia(std::initializer_list<JiaKey> k) { jiaKeys.insert(jiaKeys.end(), k); }
  void addLight(std::initializer_list<LightKey> k) { lightKeys.insert(lightKeys.end(), k); }
  void addCrowd(std::initializer_list<CrowdKey> k) { crowdKeys.insert(crowdKeys.end(), k); }
  void addShop(std::initializer_list<ShopKey> k) { shopKeys.insert(shopKeys.end(), k); }
  // Crowd time runs at k× between a and b (k = 0 freezes it, k > 1 is a time-lapse).
  void addWarp(float a, float b, float k) { warps.emplace_back(a, b, k); }
  void onUpdate(std::function<void(float)> f) { updaters.push_back(std::move(f)); }

  void finalize();
  void resize(int w, int h);
  void update(float t);
  void applyFx();

  struct ScreenPos {
    float x, y;
    bool behind;
  };
  // Screen position (0..1, y down) of a world point.
  ScreenPos toScreen(const vec3& p) const;
  void focus(const vec3& p, float r, float amt);
};

World* createWorld();

}  // namespace film
