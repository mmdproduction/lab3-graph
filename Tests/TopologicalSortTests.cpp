#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "Graph/Graph.hpp"

namespace {
TEST(TopologicalSort, IncludesDisconnectedIsolatedAndParallelDependencies) {
    Graph<int, int, std::string> graph(true);
    for (int id : {0, 1, 2, 3, 4, 5}) graph.add_vertex(id, id);
    graph.add_edge(0, 2, "one"); graph.add_edge(0, 2, "two"); graph.add_edge(1, 2, "three"); graph.add_edge(2, 3, "four");
    const auto& view = graph;
    const auto order = view.TopologicalSort(); ASSERT_EQ(order.size(), 6u);
    std::array<std::size_t, 6> position{}; std::array<bool, 6> seen{};
    for (std::size_t i = 0; i < order.size(); ++i) { ASSERT_FALSE(seen[order.get(i)]); seen[order.get(i)] = true; position[order.get(i)] = i; }
    EXPECT_LT(position[0], position[2]); EXPECT_LT(position[1], position[2]); EXPECT_LT(position[2], position[3]);
    EXPECT_EQ(graph.edge_count(), 4u);
}
TEST(TopologicalSort, SupportsEmptyGraphAndRejectsUndirectedGraph) {
    Graph<int, int, int> directed(true), undirected(false);
    EXPECT_EQ(directed.TopologicalSort().size(), 0u);
    EXPECT_THROW(undirected.TopologicalSort(), std::invalid_argument);
    undirected.add_vertex(0, 0); EXPECT_THROW(undirected.TopologicalSort(), std::invalid_argument);
}
TEST(TopologicalSort, RejectsCyclesIncludingDisconnectedCyclesAndLoops) {
    Graph<int, int, int> graph(true); for (int id : {0, 1, 2}) graph.add_vertex(id, id);
    graph.add_edge(1, 2, 1); graph.add_edge(2, 1, 1);
    EXPECT_THROW(graph.TopologicalSort(), std::domain_error);
    graph.remove_edges(2, 1); graph.add_edge(0, 0, 1);
    EXPECT_THROW(graph.TopologicalSort(), std::domain_error);
}
struct OrderedId { int value = 0; bool operator<(const OrderedId& other) const { return value < other.value; } };
TEST(TopologicalSort, SupportsStringAndOrderingOnlyIds) {
    Graph<std::string, int, int> strings(true); strings.add_vertex("", 0); strings.add_vertex("a", 1); strings.add_edge("", "a", 0);
    const auto order = strings.TopologicalSort(); ASSERT_EQ(order.size(), 2u); EXPECT_EQ(order.get(0), ""); EXPECT_EQ(order.get(1), "a");
    Graph<OrderedId, int, int> custom(true); custom.add_vertex({1}, 1); custom.add_vertex({2}, 2); custom.add_edge({2}, {1}, 0);
    const auto ids = custom.TopologicalSort(); ASSERT_EQ(ids.size(), 2u); EXPECT_EQ(ids.get(0).value, 2); EXPECT_EQ(ids.get(1).value, 1);
}
TEST(TopologicalSort, RandomDagRespectsEveryDependency) {
    constexpr int kVertices = 12;
    Graph<int, int, int> graph(true); for (int id = 0; id < kVertices; ++id) graph.add_vertex(id, id);
    std::mt19937 random(20261125);
    for (int edge = 0; edge < 45; ++edge) {
        int from = static_cast<int>(random() % kVertices), to = static_cast<int>(random() % kVertices);
        if (from == to) continue;
        if (from > to) std::swap(from, to);
        graph.add_edge(from, to, edge);
    }
    const auto order = graph.TopologicalSort(); ASSERT_EQ(order.size(), kVertices);
    std::array<std::size_t, kVertices> position{}; std::array<bool, kVertices> seen{};
    for (std::size_t i = 0; i < order.size(); ++i) { ASSERT_FALSE(seen[order.get(i)]); seen[order.get(i)] = true; position[order.get(i)] = i; }
    const auto keys = graph.edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i) EXPECT_LT(position[keys.get(i).from], position[keys.get(i).to]);
}
}  // namespace
