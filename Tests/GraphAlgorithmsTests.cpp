#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <functional>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "Graph/Graph.hpp"

namespace {
template <typename T>
std::vector<T> Values(const ArraySequence<T>& sequence) {
    std::vector<T> result;
    for (std::size_t i = 0; i < sequence.size(); ++i) result.push_back(sequence.get(i));
    return result;
}

TEST(BreadthFirstSearch, FollowsLayersAndVisitsSharedSuccessorOnce) {
    Graph<int, int, int> graph(true);
    for (int id = 1; id <= 7; ++id) graph.add_vertex(id, id);
    graph.add_edge(1, 2, 1); graph.add_edge(1, 3, 1);
    graph.add_edge(2, 4, 1); graph.add_edge(2, 5, 1); graph.add_edge(3, 4, 1);
    graph.add_edge(3, 6, 1); graph.add_edge(6, 1, 1);
    const auto& view = graph;
    EXPECT_EQ(Values(view.BFS(1)), (std::vector<int>{1, 2, 3, 4, 5, 6}));
    EXPECT_EQ(graph.vertex_count(), 7u); EXPECT_EQ(graph.edge_count(), 7u);
}

TEST(BreadthFirstSearch, HandlesLoopsParallelEdgesAndBothDirections) {
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        for (int id : {0, 1, 2}) graph.add_vertex(id, id);
        graph.add_edge(0, 0, 1); graph.add_edge(0, 0, 2);
        graph.add_edge(0, 1, 3); graph.add_edge(0, 1, 4); graph.add_edge(1, 2, 5);
        EXPECT_EQ(Values(graph.BFS(0)), (std::vector<int>{0, 1, 2}));
        const auto reverse = Values(graph.BFS(2));
        EXPECT_EQ(reverse, directed ? std::vector<int>{2} : (std::vector<int>{2, 1, 0}));
    }
}

TEST(BreadthFirstSearch, IgnoresWeightsAndAcceptsStringLiteralStart) {
    Graph<std::string, int, std::string> graph(false);
    graph.add_vertex("assets", 1); graph.add_vertex("textures", 2); graph.add_vertex("", 3);
    graph.add_edge("assets", "textures", "heavy"); graph.add_edge("textures", "", "light");
    EXPECT_EQ(Values(graph.BFS("assets")),
              (std::vector<std::string>{"assets", "textures", ""}));
}

struct OrderedId {
    int value = 0;
    bool operator<(const OrderedId& other) const { return value < other.value; }
};

TEST(BreadthFirstSearch, SupportsOrderingOnlyCustomIds) {
    Graph<OrderedId, int, int> graph(true);
    graph.add_vertex({1}, 1); graph.add_vertex({2}, 2); graph.add_edge({1}, {2}, 7);
    const auto result = graph.BFS(OrderedId{1});
    ASSERT_EQ(result.size(), 2u); EXPECT_EQ(result.get(0).value, 1); EXPECT_EQ(result.get(1).value, 2);
}

TEST(BreadthFirstSearch, HandlesIsolatedVerticesAndRejectsAbsentStart) {
    Graph<int, int, int> graph(false);
    EXPECT_THROW(graph.BFS(0), std::out_of_range);
    graph.add_vertex(0, 9);
    EXPECT_EQ(Values(graph.BFS(0)), (std::vector<int>{0}));
    EXPECT_THROW(graph.BFS(1), std::out_of_range);
    EXPECT_EQ(graph.vertex_count(), 1u); EXPECT_EQ(graph.edge_count(), 0u);
}

struct ThrowingId {
    static inline int copy_budget = -1;
    static inline int alive = 0;
    int value = 0;
    ThrowingId() { ++alive; }
    explicit ThrowingId(int number) : value(number) { ++alive; }
    ThrowingId(const ThrowingId& other) : value(other.value) {
        if (copy_budget == 0) throw std::runtime_error("injected ID copy failure");
        if (copy_budget > 0) --copy_budget;
        ++alive;
    }
    ThrowingId& operator=(const ThrowingId&) = default;
    ~ThrowingId() { --alive; }
    bool operator<(const ThrowingId& other) const { return value < other.value; }
};

TEST(BreadthFirstSearch, FailedIdCopyLeavesInputGraphUnchanged) {
    Graph<ThrowingId, int, int> graph(false);
    const ThrowingId first(1), second(2);
    graph.add_vertex(first, 10); graph.add_vertex(second, 20); graph.add_edge(first, second, 7);
    const int alive_before = ThrowingId::alive;
    int failures = 0;
    bool completed = false;
    // Fail successive copy operations, including after traversal state exists.
    for (int budget = 0; budget < 64; ++budget) {
        ThrowingId::copy_budget = budget;
        try {
            const auto order = graph.BFS(first);
            EXPECT_EQ(order.size(), 2u);
            completed = true;
        } catch (const std::runtime_error&) {
            ++failures;
        }
        ThrowingId::copy_budget = -1;
        EXPECT_EQ(ThrowingId::alive, alive_before);
        if (completed) break;
    }
    EXPECT_TRUE(completed); EXPECT_GT(failures, 3);
    EXPECT_EQ(graph.vertex_count(), 2u); EXPECT_EQ(graph.edge_count(), 1u);
    EXPECT_EQ(graph.get_vertex(first), 10); EXPECT_EQ(graph.get_edge_weight(first, second, 0), 7);
}

TEST(BreadthFirstSearch, AgreesWithIndependentQueueModelOnRandomMultigraphs) {
    constexpr int kVertices = 8;
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        std::array<std::array<bool, kVertices>, kVertices> adjacency{};
        for (int id = 0; id < kVertices; ++id) graph.add_vertex(id, id);
        std::mt19937 random(20261125);
        for (int edge = 0; edge < 45; ++edge) {
            const auto from = random() % kVertices, to = random() % kVertices;
            graph.add_edge(static_cast<int>(from), static_cast<int>(to), edge);
            adjacency[from][to] = true;
            if (!directed) adjacency[to][from] = true;
        }
        for (int start = 0; start < kVertices; ++start) {
            std::queue<int> pending; std::array<bool, kVertices> visited{}; std::vector<int> expected;
            pending.push(start); visited[start] = true;
            while (!pending.empty()) {
                const auto current = pending.front(); pending.pop(); expected.push_back(current);
                for (int next = 0; next < kVertices; ++next) {
                    if (adjacency[current][next] && !visited[next]) { visited[next] = true; pending.push(next); }
                }
            }
            EXPECT_EQ(Values(graph.BFS(start)), expected);
        }
    }
}
TEST(DepthFirstSearch, MatchesRecursivePreorderWithSharedScheduledSuccessors) {
    Graph<int, int, int> graph(true);
    for (int id = 1; id <= 7; ++id) graph.add_vertex(id, id);
    graph.add_edge(1, 2, 1); graph.add_edge(1, 3, 1);
    graph.add_edge(2, 3, 1); graph.add_edge(2, 4, 1);
    graph.add_edge(3, 5, 1); graph.add_edge(4, 6, 1); graph.add_edge(5, 1, 1);
    const auto& view = graph;
    EXPECT_EQ(Values(view.DFS(1)), (std::vector<int>{1, 2, 3, 5, 4, 6}));
    EXPECT_EQ(graph.vertex_count(), 7u); EXPECT_EQ(graph.edge_count(), 7u);
    EXPECT_EQ(graph.get_edge_weight(1, 2, 0), 1);
}

TEST(DepthFirstSearch, HandlesLoopsParallelEdgesAndDirectedReachability) {
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        for (int id : {0, 1, 2}) graph.add_vertex(id, id);
        graph.add_edge(0, 0, 1); graph.add_edge(0, 0, 2);
        graph.add_edge(0, 1, 3); graph.add_edge(0, 1, 4); graph.add_edge(1, 2, 5);
        EXPECT_EQ(Values(graph.DFS(0)), (std::vector<int>{0, 1, 2}));
        EXPECT_EQ(Values(graph.DFS(2)), directed ? std::vector<int>{2} : (std::vector<int>{2, 1, 0}));
    }
}

TEST(DepthFirstSearch, AcceptsStringLiteralStartAndIgnoresWeights) {
    Graph<std::string, int, std::string> graph(false);
    graph.add_vertex("assets", 1); graph.add_vertex("textures", 2); graph.add_vertex("", 3);
    graph.add_edge("assets", "textures", "heavy"); graph.add_edge("textures", "", "light");
    EXPECT_EQ(Values(graph.DFS("assets")), (std::vector<std::string>{"assets", "textures", ""}));
}

TEST(DepthFirstSearch, SupportsOrderingOnlyCustomIds) {
    Graph<OrderedId, int, int> graph(true);
    graph.add_vertex({1}, 1); graph.add_vertex({2}, 2); graph.add_edge({1}, {2}, 7);
    const auto order = graph.DFS(OrderedId{1});
    ASSERT_EQ(order.size(), 2u); EXPECT_EQ(order.get(0).value, 1); EXPECT_EQ(order.get(1).value, 2);
}

TEST(DepthFirstSearch, HandlesIsolatedVerticesAndRejectsAbsentStart) {
    Graph<int, int, int> graph(false);
    EXPECT_THROW(graph.DFS(0), std::out_of_range);
    graph.add_vertex(0, 9);
    EXPECT_EQ(Values(graph.DFS(0)), (std::vector<int>{0}));
    EXPECT_THROW(graph.DFS(1), std::out_of_range);
    EXPECT_EQ(graph.vertex_count(), 1u); EXPECT_EQ(graph.edge_count(), 0u);
}

TEST(DepthFirstSearch, CopyFailuresCleanTraversalStateAndPreserveGraph) {
    Graph<ThrowingId, int, int> graph(false);
    const ThrowingId first(1), second(2), third(3);
    graph.add_vertex(first, 10); graph.add_vertex(second, 20); graph.add_vertex(third, 30);
    graph.add_edge(first, second, 7); graph.add_edge(first, third, 8); graph.add_edge(second, third, 9);
    const int alive_before = ThrowingId::alive;
    bool completed = false; int failures = 0;
    for (int budget = 0; budget < 128; ++budget) {
        ThrowingId::copy_budget = budget;
        try {
            const auto order = graph.DFS(first);
            EXPECT_EQ(order.size(), 3u); completed = true;
        } catch (const std::runtime_error&) { ++failures; }
        ThrowingId::copy_budget = -1;
        EXPECT_EQ(ThrowingId::alive, alive_before);
        if (completed) break;
    }
    EXPECT_TRUE(completed); EXPECT_GT(failures, 3);
    EXPECT_EQ(graph.vertex_count(), 3u); EXPECT_EQ(graph.edge_count(), 3u);
    EXPECT_EQ(graph.get_vertex(first), 10); EXPECT_EQ(graph.get_edge_weight(first, second, 0), 7);
}

TEST(DepthFirstSearch, AgreesWithIndependentRecursiveModelOnRandomMultigraphs) {
    constexpr int kVertices = 8;
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        std::array<std::array<bool, kVertices>, kVertices> adjacency{};
        for (int id = 0; id < kVertices; ++id) graph.add_vertex(id, id);
        std::mt19937 random(20261125);
        for (int edge = 0; edge < 45; ++edge) {
            const auto from = random() % kVertices, to = random() % kVertices;
            graph.add_edge(static_cast<int>(from), static_cast<int>(to), edge);
            adjacency[from][to] = true;
            if (!directed) adjacency[to][from] = true;
        }
        for (int start = 0; start < kVertices; ++start) {
            std::array<bool, kVertices> visited{}; std::vector<int> expected;
            const std::function<void(int)> visit = [&](int id) {
                visited[id] = true; expected.push_back(id);
                for (int next = 0; next < kVertices; ++next) {
                    if (adjacency[id][next] && !visited[next]) visit(next);
                }
            };
            visit(start);
            EXPECT_EQ(Values(graph.DFS(start)), expected);
        }
    }
}
}  // namespace
