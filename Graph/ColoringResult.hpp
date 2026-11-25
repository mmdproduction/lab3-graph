#pragma once

#include <cstddef>

#include "GraphTypes.hpp"
#include "../Dictionary/AVLDictionary.hpp"

template <ComparableVertexId ID>
struct ColoringResult {
    AvlDictionary<ID, std::size_t> colors;
    std::size_t color_count = 0;
};
