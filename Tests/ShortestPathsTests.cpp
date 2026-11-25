#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "Graph/Graph.hpp"

namespace {
template <typename T>
std::vector<T> Values(const ArraySequence<T>& values) {
    std::vector<T> result;
    for (std::size_t i = 0; i < values.size(); ++i) result.push_back(values.get(i));
    return result;
}
template <typename G, typename Id>
concept HasDijkstra = requires(const G& graph, const Id& id) { graph.Dijkstra(id, id); };
template <typename G, typename Id>
concept HasBellmanFord = requires(const G& graph, const Id& id) { graph.BellmanFord(id, id); };
static_assert(HasDijkstra<Graph<int, int, int>, int>);
static_assert(HasBellmanFord<Graph<int, int, double>, int>);
static_assert(!HasDijkstra<Graph<int, int, std::string>, int>);
static_assert(!HasBellmanFord<Graph<int, int, std::string>, int>);
static_assert(!HasDijkstra<Graph<int, int, bool>, int>);

template <typename Id, typename Data, typename Weight>
auto Solve(const Graph<Id, Data, Weight>& graph, const std::type_identity_t<Id>& start,
           const std::type_identity_t<Id>& finish, bool bellman) {
    return bellman ? graph.BellmanFord(start, finish) : graph.Dijkstra(start, finish);
}

TEST(ShortestPaths, ReturnsPathStructAndChoosesMinimumParallelWeights) {
    for (bool bellman : {false, true}) {
        Graph<std::string, int, int> graph(true);
        for (const std::string id : {"S", "A", "B", "T", "U"}) graph.add_vertex(id, 1);
        graph.add_edge("S", "A", 9); graph.add_edge("S", "A", 2); graph.add_edge("S", "A", 2);
        graph.add_edge("S", "B", 5); graph.add_edge("A", "B", 0); graph.add_edge("B", "A", 0);
        graph.add_edge("B", "T", 3); graph.add_edge("A", "T", 9); graph.add_edge("S", "S", 0);
        const auto& view = graph;
        const auto path = Solve(view, "S", "T", bellman);
        EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 5);
        EXPECT_EQ(Values(path.vertices), (std::vector<std::string>{"S", "A", "B", "T"}));
        const auto same = Solve(graph, "S", "S", bellman);
        EXPECT_TRUE(same.reachable); EXPECT_EQ(same.total_weight, 0);
        EXPECT_EQ(Values(same.vertices), (std::vector<std::string>{"S"}));
        const auto absent = Solve(graph, "S", "U", bellman);
        EXPECT_FALSE(absent.reachable); EXPECT_EQ(absent.vertices.size(), 0u);
        EXPECT_EQ(graph.edge_count(), 9u); EXPECT_EQ(graph.get_edge_weight("S", "A", 1), 2);
    }
}

TEST(ShortestPaths, HandlesUndirectedEdgesLoopsAndZeroIds) {
    for (bool bellman : {false, true}) {
        Graph<int, int, int> graph(false);
        for (int id : {0, 1, 2, 3}) graph.add_vertex(id, id);
        graph.add_edge(2, 1, 8); graph.add_edge(1, 2, 3); graph.add_edge(1, 0, 2);
        graph.add_edge(1, 1, 0); graph.add_edge(2, 0, 20);
        const auto path = Solve(graph, 2, 0, bellman);
        EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 5);
        EXPECT_EQ(Values(path.vertices), (std::vector<int>{2, 1, 0}));
        EXPECT_FALSE(Solve(graph, 2, 3, bellman).reachable);
    }
}

TEST(ShortestPaths, RejectsAbsentEndpointsAndSupportsIsolatedSource) {
    for (bool bellman : {false, true}) {
        Graph<int, int, int> graph(true);
        EXPECT_THROW(Solve(graph, 0, 0, bellman), std::out_of_range);
        graph.add_vertex(0, 7);
        EXPECT_THROW(Solve(graph, 1, 0, bellman), std::out_of_range);
        EXPECT_THROW(Solve(graph, 0, 1, bellman), std::out_of_range);
        const auto path = Solve(graph, 0, 0, bellman);
        EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 0);
        EXPECT_EQ(Values(path.vertices), (std::vector<int>{0}));
    }
}

TEST(Dijkstra, RejectsNegativeEdgesIncludingUnreachableOnes) {
    Graph<int, int, int> graph(true);
    for (int id : {0, 1, 2}) graph.add_vertex(id, id);
    graph.add_edge(1, 2, -1);
    EXPECT_THROW(graph.Dijkstra(0, 0), std::invalid_argument);
    graph.add_edge(0, 0, -1); EXPECT_THROW(graph.Dijkstra(0, 2), std::invalid_argument);
}

TEST(BellmanFord, SupportsNegativeEdgesAndEnoughRelaxationPasses) {
    Graph<int, int, int> graph(true);
    for (int id : {0, 1, 2, 3}) graph.add_vertex(id, id);
    graph.add_edge(3, 2, -1); graph.add_edge(2, 1, -1); graph.add_edge(1, 0, -1);
    const auto path = graph.BellmanFord(3, 0);
    EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, -3);
    EXPECT_EQ(Values(path.vertices), (std::vector<int>{3, 2, 1, 0}));
}

TEST(BellmanFord, RejectsReachableNegativeCyclesAndNegativeLoops) {
    Graph<int, int, int> graph(true);
    for (int id : {0, 1, 2}) graph.add_vertex(id, id);
    graph.add_edge(0, 1, 0); graph.add_edge(1, 2, -2); graph.add_edge(2, 1, 1);
    EXPECT_THROW(graph.BellmanFord(0, 2), std::domain_error);
    EXPECT_THROW(graph.BellmanFord(0, 0), std::domain_error);
    Graph<int, int, int> loop(true); loop.add_vertex(0, 1); loop.add_edge(0, 0, -1);
    EXPECT_THROW(loop.BellmanFord(0, 0), std::domain_error);
}

TEST(BellmanFord, IgnoresUnreachableNegativeCycles) {
    Graph<int, int, int> graph(true);
    for (int id : {0, 1, 2}) graph.add_vertex(id, id);
    graph.add_edge(1, 2, -2); graph.add_edge(2, 1, 1);
    const auto path = graph.BellmanFord(0, 0);
    EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 0);
    EXPECT_FALSE(graph.BellmanFord(0, 1).reachable);
}

TEST(BellmanFord, UndirectedReachableNegativeEdgeIsANegativeCycle) {
    Graph<int, int, int> graph(false); graph.add_vertex(0, 1); graph.add_vertex(1, 1);
    graph.add_edge(0, 1, -1); EXPECT_THROW(graph.BellmanFord(0, 1), std::domain_error);
}

TEST(ShortestPaths, SupportsFloatingPointAndRejectsNonFiniteWeights) {
    for (bool bellman : {false, true}) {
        Graph<int, int, double> graph(true);
        for (int id : {0, 1, 2}) graph.add_vertex(id, id);
        graph.add_edge(0, 1, 1.25); graph.add_edge(1, 2, 0.5); graph.add_edge(0, 2, 4.0);
        EXPECT_DOUBLE_EQ(Solve(graph, 0, 2, bellman).total_weight, 1.75);
        graph.add_edge(2, 2, std::numeric_limits<double>::infinity());
        EXPECT_THROW(Solve(graph, 0, 2, bellman), std::invalid_argument);
        graph.set_edge_weight(2, 2, 0, std::numeric_limits<double>::quiet_NaN());
        EXPECT_THROW(Solve(graph, 0, 2, bellman), std::invalid_argument);
    }
    Graph<int, int, double> negative(true); negative.add_vertex(0, 0); negative.add_vertex(1, 1);
    negative.add_edge(0, 1, -0.5); EXPECT_DOUBLE_EQ(negative.BellmanFord(0, 1).total_weight, -0.5);
}

TEST(ShortestPaths, AllowsMaximumDistanceWithoutInfinitySentinel) {
    for (bool bellman : {false, true}) {
        Graph<int, int, std::int64_t> graph(true); graph.add_vertex(0, 0); graph.add_vertex(1, 1);
        graph.add_edge(0, 1, std::numeric_limits<std::int64_t>::max());
        const auto path = Solve(graph, 0, 1, bellman);
        EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, std::numeric_limits<std::int64_t>::max());
    }
}

TEST(ShortestPaths, DetectsSignedUnsignedAndFloatingOverflow) {
    for (bool bellman : {false, true}) {
        Graph<int, int, std::int64_t> signed_graph(true);
        Graph<int, int, std::uint32_t> unsigned_graph(true); Graph<int, int, double> floating_graph(true);
        for (int id : {0, 1, 2}) {
            signed_graph.add_vertex(id, id); unsigned_graph.add_vertex(id, id); floating_graph.add_vertex(id, id);
        }
        signed_graph.add_edge(0, 1, std::numeric_limits<std::int64_t>::max()); signed_graph.add_edge(1, 2, 1);
        unsigned_graph.add_edge(0, 1, std::numeric_limits<std::uint32_t>::max()); unsigned_graph.add_edge(1, 2, 1);
        floating_graph.add_edge(0, 1, std::numeric_limits<double>::max()); floating_graph.add_edge(1, 2, std::numeric_limits<double>::max());
        EXPECT_THROW(Solve(signed_graph, 0, 2, bellman), std::overflow_error);
        EXPECT_THROW(Solve(unsigned_graph, 0, 2, bellman), std::overflow_error);
        EXPECT_THROW(Solve(floating_graph, 0, 2, bellman), std::overflow_error);
    }
}

TEST(BellmanFord, DetectsSignedUnderflow) {
    Graph<int, int, std::int64_t> graph(true); for (int id : {0, 1, 2}) graph.add_vertex(id, id);
    graph.add_edge(0, 1, std::numeric_limits<std::int64_t>::lowest()); graph.add_edge(1, 2, -1);
    EXPECT_THROW(graph.BellmanFord(0, 2), std::overflow_error);
}

struct OrderedId { int value = 0; bool operator<(const OrderedId& other) const { return value < other.value; } };
TEST(ShortestPaths, SupportsOrderingOnlyCustomIds) {
    for (bool bellman : {false, true}) {
        Graph<OrderedId, int, int> graph(true); graph.add_vertex({0}, 0); graph.add_vertex({1}, 1); graph.add_edge({0}, {1}, 4);
        const auto path = Solve(graph, OrderedId{0}, OrderedId{1}, bellman);
        EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 4); ASSERT_EQ(path.vertices.size(), 2u);
        EXPECT_EQ(path.vertices.get(0).value, 0);
    }
}

TEST(ShortestPaths, ResultsAreIndependentValueSnapshots) {
    for (bool bellman : {false, true}) {
        Graph<int, int, int> graph(true); graph.add_vertex(0, 0); graph.add_vertex(1, 1); graph.add_edge(0, 1, 4);
        const auto path = Solve(graph, 0, 1, bellman); auto copy = path;
        auto assigned = Solve(graph, 1, 1, bellman); assigned = path; copy.vertices.get(0) = 99;
        graph.set_edge_weight(0, 1, 0, 99); graph.remove_vertex(0);
        EXPECT_EQ(path.total_weight, 4); EXPECT_EQ(path.vertices.get(0), 0);
        EXPECT_EQ(Values(assigned.vertices), (std::vector<int>{0, 1}));
    }
}

TEST(ShortestPaths, AgreesWithIndependentFloydWarshallModel) {
    constexpr int kVertices = 6, kInfinity = 100000;
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        std::array<std::array<int, kVertices>, kVertices> distances;
        for (int i = 0; i < kVertices; ++i) { graph.add_vertex(i, i); distances[i].fill(kInfinity); distances[i][i] = 0; }
        std::mt19937 random(20261125);
        for (int edge = 0; edge < 28; ++edge) {
            const int from = static_cast<int>(random() % kVertices), to = static_cast<int>(random() % kVertices);
            const int weight = static_cast<int>(random() % 10); graph.add_edge(from, to, weight);
            distances[from][to] = std::min(distances[from][to], weight);
            if (!directed) distances[to][from] = std::min(distances[to][from], weight);
        }
        for (int k = 0; k < kVertices; ++k) for (int i = 0; i < kVertices; ++i) for (int j = 0; j < kVertices; ++j) {
            if (distances[i][k] != kInfinity && distances[k][j] != kInfinity)
                distances[i][j] = std::min(distances[i][j], distances[i][k] + distances[k][j]);
        }
        for (int start = 0; start < kVertices; ++start) for (int target = 0; target < kVertices; ++target) for (bool bellman : {false, true}) {
            const auto path = Solve(graph, start, target, bellman);
            ASSERT_EQ(path.reachable, distances[start][target] != kInfinity);
            if (!path.reachable) { EXPECT_EQ(path.vertices.size(), 0u); continue; }
            EXPECT_EQ(path.total_weight, distances[start][target]); ASSERT_GT(path.vertices.size(), 0u);
            EXPECT_EQ(path.vertices.get(0), start); EXPECT_EQ(path.vertices.get(path.vertices.size() - 1), target);
            int total = 0;
            for (std::size_t i = 1; i < path.vertices.size(); ++i) {
                const auto& weights = graph.get_edge_weights(path.vertices.get(i - 1), path.vertices.get(i));
                int best = weights.get(0);
                for (std::size_t j = 1; j < weights.size(); ++j) best = std::min(best, weights.get(j));
                total += best;
            }
            EXPECT_EQ(total, path.total_weight);
        }
    }
}
TEST(BellmanFord, NegativeDagAgreesWithIndependentDynamicProgramming) {
    constexpr int kVertices = 7, kInfinity = 100000;
    Graph<int, int, int> graph(true); for (int i = 0; i < kVertices; ++i) graph.add_vertex(i, i);
    std::array<std::array<int, kVertices>, kVertices> costs;
    for (auto& row : costs) row.fill(kInfinity);
    std::mt19937 random(20261125);
    for (int edge = 0; edge < 30; ++edge) {
        int from = static_cast<int>(random() % kVertices), to = static_cast<int>(random() % kVertices);
        if (from == to) continue;
        if (from > to) std::swap(from, to);
        const int weight = static_cast<int>(random() % 11) - 5;
        graph.add_edge(from, to, weight); costs[from][to] = std::min(costs[from][to], weight);
    }
    for (int start = 0; start < kVertices; ++start) {
        std::array<int, kVertices> expected; expected.fill(kInfinity); expected[start] = 0;
        for (int from = 0; from < kVertices; ++from) for (int to = from + 1; to < kVertices; ++to) {
            if (expected[from] != kInfinity && costs[from][to] != kInfinity)
                expected[to] = std::min(expected[to], expected[from] + costs[from][to]);
        }
        for (int target = 0; target < kVertices; ++target) {
            const auto path = graph.BellmanFord(start, target);
            EXPECT_EQ(path.reachable, expected[target] != kInfinity);
            if (path.reachable) {
                EXPECT_EQ(path.total_weight, expected[target]);
            }
        }
    }
}

struct ThrowingId {
    static inline int budget = -1;
    static inline int alive = 0;
    int value = 0;
    ThrowingId() { ++alive; }
    explicit ThrowingId(int number) : value(number) { ++alive; }
    ThrowingId(const ThrowingId& other) : value(other.value) {
        if (budget == 0) throw std::runtime_error("injected ID failure");
        if (budget > 0) --budget;
        ++alive;
    }
    ThrowingId& operator=(const ThrowingId&) = default;
    ~ThrowingId() { --alive; }
    bool operator<(const ThrowingId& other) const { return value < other.value; }
};
TEST(ShortestPaths, CopyFailuresCleanLocalStateAndPreserveGraph) {
    for (bool bellman : {false, true}) {
        Graph<ThrowingId, int, int> graph(true);
        const ThrowingId start(0), finish(1);
        graph.add_vertex(start, 0); graph.add_vertex(finish, 1); graph.add_edge(start, finish, 4);
        const int before = ThrowingId::alive;
        bool completed = false; int failures = 0;
        for (int budget = 0; budget < 256; ++budget) {
            ThrowingId::budget = budget;
            try {
                const auto path = Solve(graph, start, finish, bellman);
                EXPECT_TRUE(path.reachable); EXPECT_EQ(path.total_weight, 4); completed = true;
            } catch (const std::runtime_error&) { ++failures; }
            ThrowingId::budget = -1;
            EXPECT_EQ(ThrowingId::alive, before);
            if (completed) break;
        }
        EXPECT_TRUE(completed); EXPECT_GT(failures, 3);
        EXPECT_EQ(graph.vertex_count(), 2u); EXPECT_EQ(graph.get_edge_weight(start, finish, 0), 4);
    }
    EXPECT_EQ(ThrowingId::alive, 0);
}
}  // namespace
