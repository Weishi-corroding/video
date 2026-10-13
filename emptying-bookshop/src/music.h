// The score: synthesized music laid out on the script's timing, one section per chapter, with a
// few sounds tied to on-screen events (a clock ticking, a camera shutter, beads dropping into a jar).
#pragma once
#include <eng/eng.h>

namespace film {

// Stereo music for the whole film at `rate`. Call after script::layout().
eng::Audio composeScore(int rate = 48000);

}  // namespace film
