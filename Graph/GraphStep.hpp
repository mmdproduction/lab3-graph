#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include "GraphTypes.hpp"

enum class GraphStepKind {
    Begin,
    DiscoverVertex,
    VisitVertex,
    FinishVertex,
    ExamineEdge,
    UpdateDistance,
    BeginPass,
    AcceptEdge,
    RejectEdge,
    AssignColor,
    PathVertex,
    PathEdge,
    Unreachable,
    Cycle,
    Complete
};
inline constexpr std::size_t kAllParallelEdges = std::numeric_limits<std::size_t>::max();

template <ComparableVertexId ID, typename Weight>
struct GraphStep {
    GraphStepKind kind = GraphStepKind::Begin;
    ID vertex{};
    ID from{};
    ID to{};
    std::size_t edge_index = kAllParallelEdges;
    Weight value{};
    std::size_t number = 0;
};

template <ComparableVertexId ID, typename Weight>
using GraphObserver = std::function<void(const GraphStep<ID, Weight>&)>;
