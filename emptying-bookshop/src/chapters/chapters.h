// One function per chapter: each builds its props, registers keys with the world and adds labels,
// then returns an update function that animates everything from master time t.
#pragma once
#include <functional>

#include "../overlay.h"
#include "../script.h"
#include "../world.h"

namespace film {

struct Ctx {
  World& W;
  Labels& L;
};
using Update = std::function<void(float)>;

// Time relative to a cue's start.
inline float T(const char* id, float dt = 0) { return c(id).s + dt; }

Update buildC1(Ctx& ctx);
Update buildC2(Ctx& ctx);
Update buildC3(Ctx& ctx);
Update buildC4(Ctx& ctx);
Update buildC5(Ctx& ctx);
Update buildC6(Ctx& ctx);
Update buildC7(Ctx& ctx);

}  // namespace film
