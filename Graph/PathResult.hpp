#pragma once

#include <concepts>
#include <type_traits>

#include "GraphTypes.hpp"
#include "../Sequence/ArraySequence/ArraySequence.hpp"

template <typename T>
concept NumericPathWeight =
    (std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>) || std::floating_point<T>;

template <ComparableVertexId ID, typename Weight>
struct PathResult {
    ArraySequence<ID> vertices;
    Weight total_weight{};
    bool reachable = false;
};
