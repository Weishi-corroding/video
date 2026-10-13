// A procedural, tileable sheet of paper for the renderer's paper look.
#pragma once
#include <cstdint>
#include <vector>

namespace eng {

// RGBA8, `size` × `size`, tiling seamlessly. R: colour variation (128 = none), G: height of the fibre
// relief, B: fine grain, A: 255.
std::vector<uint8_t> paperTexture(int size, uint32_t seed = 7);

}  // namespace eng
