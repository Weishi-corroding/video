// The score: synthesized music laid out on the script's timing, one section per chapter, with a
// few sounds tied to on-screen events (a clock ticking, a camera shutter, beads dropping into a jar).
#pragma once
#include <eng/eng.h>

#include <utility>
#include <vector>

namespace film {

// Sounds tied to what happens on screen, from the world: page turns and pieces of card popping up.
struct ScoreEvents {
  std::vector<double> pageTurns;                    // start of each turn
  std::vector<std::pair<double, float>> flaps;      // time and loudness
};

// Stereo music for the whole film at `rate`. Call after script::layout().
eng::Audio composeScore(const ScoreEvents& events, int rate = 48000);

}  // namespace film
