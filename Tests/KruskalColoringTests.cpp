#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

#include "Graph/Graph.hpp"

namespace {
template <typename G>
concept HasKruskal = requires(const G& graph) { graph.Kruskal(); };
// These constraints keep unweighted operations usable for other payloads.
static_assert(HasKruskal<Graph<int, int, int>>);
static_assert(HasKruskal<Graph<int, int, double>>);
static_assert(!HasKruskal<Graph<int, int, std::string>>);
static_assert(!HasKruskal<Graph<int, int, bool>>);

TEST(Kruskal, SelectsCheapestParallelEdgesAndSkipsCyclesAndLoops) {
    Graph<std::string, int, int> graph(false);
    for (const std::string id : {"A", "B", "C", "D"}) graph.add_vertex(id, 0);
    graph.add_edge("A", "B", 8); graph.add_edge("B", "A", 1);
    graph.add_edge("B", "C", 2); graph.add_edge("A", "C", 3);
    graph.add_edge("C", "D", 4); graph.add_edge("B", "D", 6);
    graph.add_edge("A", "A", -100);
    const auto& view = graph;
    const auto forest = view.Kruskal();
    EXPECT_EQ(forest.component_count, 1u); EXPECT_EQ(forest.total_weight, 7);
    ASSERT_EQ(forest.edges.size(), 3u);
    EXPECT_EQ(forest.edges.get(0).from, "A"); EXPECT_EQ(forest.edges.get(0).to, "B"); EXPECT_EQ(forest.edges.get(0).weight, 1);
    EXPECT_EQ(forest.edges.get(1).weight, 2); EXPECT_EQ(forest.edges.get(2).weight, 4);
    EXPECT_EQ(graph.edge_count(), 7u); EXPECT_EQ(graph.get_edge_weight("A", "B", 0), 8);
}
TEST(Kruskal, ReturnsForestAndCountsIsolatedVertices) {
    Graph<int, int, int> graph(false);
    for (int id = 0; id < 6; ++id) graph.add_vertex(id, id);
    graph.add_edge(0, 1, -4); graph.add_edge(1, 2, 0); graph.add_edge(3, 4, -2);
    graph.add_edge(5, 5, -100);
    const auto result = graph.Kruskal();
    EXPECT_EQ(result.component_count, 3u); EXPECT_EQ(result.total_weight, -6); EXPECT_EQ(result.edges.size(), 3u);
}
TEST(Kruskal, EmptyIsolatedAndDirectedGraphs) {
    Graph<int, int, int> empty(false), directed(true);
    const auto result = empty.Kruskal();
    EXPECT_EQ(result.component_count, 0u); EXPECT_EQ(result.total_weight, 0); EXPECT_EQ(result.edges.size(), 0u);
    empty.add_vertex(0, 0);
    EXPECT_EQ(empty.Kruskal().component_count, 1u);
    EXPECT_THROW(directed.Kruskal(), std::invalid_argument);
    directed.add_vertex(0, 0); EXPECT_THROW(directed.Kruskal(), std::invalid_argument);
}
TEST(Kruskal, HandlesEqualWeightsZeroWeightsAndDeterministicTies) {
    Graph<int, int, int> graph(false);
    for (int id = 0; id < 4; ++id) graph.add_vertex(id, id);
    for (int from = 0; from < 4; ++from) for (int to = from + 1; to < 4; ++to) graph.add_edge(from, to, 0);
    const auto first = graph.Kruskal(), second = graph.Kruskal();
    ASSERT_EQ(first.edges.size(), 3u); EXPECT_EQ(first.total_weight, 0);
    for (std::size_t i = 0; i < first.edges.size(); ++i) {
        EXPECT_EQ(first.edges.get(i).from, second.edges.get(i).from);
        EXPECT_EQ(first.edges.get(i).to, second.edges.get(i).to);
    }
}
TEST(Kruskal, SupportsFractionalWeightsAndRejectsNonFiniteWeights) {
    Graph<int, int, double> graph(false);
    for (int id = 0; id < 3; ++id) graph.add_vertex(id, id);
    graph.add_edge(0, 1, -1.5); graph.add_edge(1, 2, 0.25); graph.add_edge(0, 2, 2.0);
    EXPECT_DOUBLE_EQ(graph.Kruskal().total_weight, -1.25);
    graph.add_edge(0, 0, std::numeric_limits<double>::infinity());
    EXPECT_THROW(graph.Kruskal(), std::invalid_argument);
    graph.remove_edges(0, 0); graph.add_edge(1, 1, std::numeric_limits<double>::quiet_NaN());
    EXPECT_THROW(graph.Kruskal(), std::invalid_argument);
}
TEST(Kruskal, ChecksSignedUnsignedAndFloatingSumBounds) {
    Graph<int, int, std::int64_t> signed_graph(false);
    for (int id = 0; id < 3; ++id) signed_graph.add_vertex(id, id);
    signed_graph.add_edge(0, 1, std::numeric_limits<std::int64_t>::max());
    EXPECT_EQ(signed_graph.Kruskal().total_weight, std::numeric_limits<std::int64_t>::max());
    signed_graph.add_edge(1, 2, 1); EXPECT_THROW(signed_graph.Kruskal(), std::overflow_error);
    signed_graph.remove_edges(0, 1); signed_graph.remove_edges(1, 2);
    signed_graph.add_edge(0, 1, std::numeric_limits<std::int64_t>::lowest()); signed_graph.add_edge(1, 2, -1);
    EXPECT_THROW(signed_graph.Kruskal(), std::overflow_error);
    Graph<int, int, unsigned> unsigned_graph(false);
    Graph<int, int, double> floating_graph(false);
    for (int id = 0; id < 3; ++id) { unsigned_graph.add_vertex(id, id); floating_graph.add_vertex(id, id); }
    unsigned_graph.add_edge(0, 1, std::numeric_limits<unsigned>::max()); unsigned_graph.add_edge(1, 2, 1);
    floating_graph.add_edge(0, 1, std::numeric_limits<double>::max()); floating_graph.add_edge(1, 2, std::numeric_limits<double>::max());
    EXPECT_THROW(unsigned_graph.Kruskal(), std::overflow_error);
    EXPECT_THROW(floating_graph.Kruskal(), std::overflow_error);
}
struct OrderedId {
    int value = 0;
    bool operator<(const OrderedId& other) const { return value < other.value; }
};
TEST(Kruskal, SupportsOrderingOnlyIdsAndIndependentResultCopies) {
    Graph<OrderedId, int, int> graph(false);
    graph.add_vertex({0}, 0); graph.add_vertex({9}, 0); graph.add_edge({9}, {0}, 3);
    auto result = graph.Kruskal(); auto copy = result;
    graph.remove_vertex({9}); result.edges.get(0).weight = 100;
    ASSERT_EQ(copy.edges.size(), 1u); EXPECT_EQ(copy.edges.get(0).weight, 3);
    EXPECT_EQ(copy.edges.get(0).from.value, 0); EXPECT_EQ(copy.edges.get(0).to.value, 9);
}
TEST(Kruskal, RandomMultigraphsAgreeWithIndependentPrimForest) {
    constexpr int kCount = 9, kInfinity = 1000000;
    std::mt19937 random(20261126);
    for (int trial = 0; trial < 24; ++trial) {
        Graph<int, int, int> graph(false);
        std::array<std::array<int, kCount>, kCount> costs;
        for (auto& row : costs) row.fill(kInfinity);
        for (int id = 0; id < kCount; ++id) graph.add_vertex(id, id);
        const int attempts = trial < 8 ? trial : 45;
        for (int edge = 0; edge < attempts; ++edge) {
            int from = random() % kCount, to = random() % kCount, weight = static_cast<int>(random() % 41) - 20;
            graph.add_edge(from, to, weight);
            if (from != to) costs[from][to] = costs[to][from] = std::min(costs[from][to], weight);
        }
        std::array<bool, kCount> included{};
        std::array<int, kCount> best; best.fill(kInfinity);
        int expected_weight = 0; std::size_t components = 0;
        for (int step = 0; step < kCount; ++step) {
            int current = -1;
            for (int id = 0; id < kCount; ++id)
                if (!included[id] && (current == -1 || best[id] < best[current])) current = id;
            if (best[current] == kInfinity) ++components;
            else expected_weight += best[current];
            included[current] = true;
            for (int to = 0; to < kCount; ++to) if (!included[to]) best[to] = std::min(best[to], costs[current][to]);
        }
        const auto forest = graph.Kruskal();
        EXPECT_EQ(forest.total_weight, expected_weight); EXPECT_EQ(forest.component_count, components);
        EXPECT_EQ(forest.edges.size(), kCount - components);
        // Reachability closure independently checks acyclicity and spanning.
        std::array<std::array<bool, kCount>, kCount> reachable{};
        int actual_weight = 0;
        for (std::size_t i = 0; i < forest.edges.size(); ++i) {
            const auto& edge = forest.edges.get(i);
            ASSERT_NE(edge.from, edge.to); EXPECT_EQ(edge.weight, costs[edge.from][edge.to]);
            reachable[edge.from][edge.to] = reachable[edge.to][edge.from] = true;
            actual_weight += edge.weight;
        }
        EXPECT_EQ(actual_weight, expected_weight);
        for (int k = 0; k < kCount; ++k) for (int i = 0; i < kCount; ++i) for (int j = 0; j < kCount; ++j)
            reachable[i][j] = reachable[i][j] || (reachable[i][k] && reachable[k][j]);
        for (int i = 0; i < kCount; ++i) for (int j = 0; j < kCount; ++j)
            if (costs[i][j] != kInfinity) { EXPECT_TRUE(reachable[i][j]); }
    }
}

TEST(GreedyColoring, TriangleAndIsolatedVertexUseSmallestFreeColor) {
    Graph<int, int, std::string> graph(false);
    for (int id : {0, 1, 2, 3}) graph.add_vertex(id, id);
    graph.add_edge(0, 1, "a"); graph.add_edge(1, 2, "b"); graph.add_edge(2, 0, "c"); graph.add_edge(0, 1, "parallel");
    const auto& view = graph;
    const auto result = view.GreedyColoring();
    EXPECT_EQ(result.color_count, 3u); EXPECT_EQ(result.colors.size(), 4u);
    EXPECT_EQ(result.colors.get(0), 0u); EXPECT_EQ(result.colors.get(1), 1u);
    EXPECT_EQ(result.colors.get(2), 2u); EXPECT_EQ(result.colors.get(3), 0u);
}
TEST(GreedyColoring, DirectedEdgesConstrainBothEndpointsAndIgnoreWeights) {
    Graph<int, int, double> graph(true);
    for (int id = 0; id < 3; ++id) graph.add_vertex(id, id);
    graph.add_edge(0, 1, std::numeric_limits<double>::quiet_NaN());
    graph.add_edge(1, 2, -10); graph.add_edge(2, 1, 42);
    const auto result = graph.GreedyColoring();
    EXPECT_EQ(result.color_count, 2u); EXPECT_EQ(result.colors.get(0), 0u);
    EXPECT_EQ(result.colors.get(1), 1u); EXPECT_EQ(result.colors.get(2), 0u);
}
TEST(GreedyColoring, EmptyAndEdgelessGraphs) {
    Graph<std::string, int, int> graph(false);
    const auto empty = graph.GreedyColoring(); EXPECT_EQ(empty.color_count, 0u); EXPECT_EQ(empty.colors.size(), 0u);
    graph.add_vertex("", 0); graph.add_vertex("z", 0);
    const auto isolated = graph.GreedyColoring();
    EXPECT_EQ(isolated.color_count, 1u); EXPECT_EQ(isolated.colors.get(""), 0u); EXPECT_EQ(isolated.colors.get("z"), 0u);
}
TEST(GreedyColoring, RejectsSelfLoopsIncludingDisconnectedLoop) {
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed); graph.add_vertex(0, 0); graph.add_vertex(1, 1);
        graph.add_edge(1, 1, 0);
        EXPECT_THROW(graph.GreedyColoring(), std::domain_error);
        EXPECT_EQ(graph.edge_count(), 1u); EXPECT_EQ(graph.vertex_count(), 2u);
    }
}
TEST(GreedyColoring, DoesNotClaimOptimalColorCountForBadVertexOrder) {
    // Path 0--2--3--1 is bipartite, but increasing IDs use 3 colors.
    Graph<int, int, int> graph(false);
    for (int id = 0; id < 4; ++id) graph.add_vertex(id, id);
    graph.add_edge(0, 2, 1); graph.add_edge(2, 3, 1); graph.add_edge(3, 1, 1);
    const auto result = graph.GreedyColoring();
    EXPECT_EQ(result.color_count, 3u); EXPECT_EQ(result.colors.get(0), 0u); EXPECT_EQ(result.colors.get(1), 0u);
    EXPECT_EQ(result.colors.get(2), 1u); EXPECT_EQ(result.colors.get(3), 2u);
}
TEST(GreedyColoring, SupportsOrderingOnlyIdsAndIndependentResults) {
    Graph<OrderedId, int, bool> graph(true);
    graph.add_vertex({0}, 0); graph.add_vertex({1}, 0); graph.add_edge({0}, {1}, true);
    auto result = graph.GreedyColoring(); auto copy = result;
    result.colors.get({0}) = 99; graph.clear();
    EXPECT_EQ(copy.color_count, 2u); EXPECT_EQ(copy.colors.get({0}), 0u); EXPECT_EQ(copy.colors.get({1}), 1u);
}
TEST(GreedyColoring, RandomGraphsHaveProperColorsAndGreedyMinimalChoice) {
    constexpr int kCount = 18;
    std::mt19937 random(20261127);
    for (bool directed : {false, true}) for (int trial = 0; trial < 8; ++trial) {
        Graph<int, int, int> graph(directed);
        std::array<std::array<bool, kCount>, kCount> adjacent{};
        for (int id = 0; id < kCount; ++id) graph.add_vertex(id, id);
        for (int edge = 0; edge < 90; ++edge) {
            int from = random() % kCount, to = random() % kCount;
            if (from == to) continue;
            graph.add_edge(from, to, edge); adjacent[from][to] = adjacent[to][from] = true;
        }
        const auto result = graph.GreedyColoring(); EXPECT_EQ(result.colors.size(), kCount);
        std::size_t max_color = 0, max_degree = 0;
        for (int id = 0; id < kCount; ++id) {
            const auto color = result.colors.get(id); ASSERT_LT(color, kCount); max_color = std::max(color, max_color);
            std::array<bool, kCount> blocked{}; std::size_t degree = 0;
            for (int neighbor = 0; neighbor < kCount; ++neighbor) if (adjacent[id][neighbor]) {
                ++degree; EXPECT_NE(color, result.colors.get(neighbor));
                if (neighbor < id) blocked[result.colors.get(neighbor)] = true;
            }
            for (std::size_t lower = 0; lower < color; ++lower) EXPECT_TRUE(blocked[lower]);
            max_degree = std::max(max_degree, degree);
        }
        EXPECT_EQ(result.color_count, max_color + 1); EXPECT_LE(result.color_count, max_degree + 1);
    }
}
}  // namespace
