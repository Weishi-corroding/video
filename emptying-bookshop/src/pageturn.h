// A page turn between chapters: the last picture of a chapter is a page that curls up from the
// bottom-right corner and turns away to the left, showing the next chapter underneath.
#pragma once
#include <cstdint>

namespace film {

// Composite one frame of a page turn. `page` is the picture on the turning page, `under` what the turn
// reveals; both and `out` are RGBA8 frames, w × h, top row first. `progress` runs from 0 (the page
// lies flat) to 1 (it has turned away).
void pageTurn(const uint8_t* page, const uint8_t* under, uint8_t* out, int w, int h, float progress);

}  // namespace film
