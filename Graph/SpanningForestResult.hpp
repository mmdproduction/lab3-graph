#pragma once

#include <cstddef>

#include "GraphTypes.hpp"
#include "../Sequence/ArraySequence/ArraySequence.hpp"

template <ComparableVertexId ID, typename Weight>
struct WeightedEdge {
    ID from{};
    ID to{};
    Weight weight{};
    std::size_t edge_index = 0;  // Index in the source pair at the time of the snapshot.
};

template <ComparableVertexId ID, typename Weight>
struct SpanningForestResult {
    ArraySequence<WeightedEdge<ID, Weight>> edges;
    Weight total_weight{};
    std::size_t component_count = 0;
};
