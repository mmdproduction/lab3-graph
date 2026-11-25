#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include "Graph/Graph.hpp"

namespace {
using Kind = GraphStepKind;
using Event = GraphStep<std::string, double>;
using TestGraph = Graph<std::string, int, double>;
bool Has(const ArraySequence<Event>& events, Kind kind) {
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events.get(i).kind == kind) return true;
    return false;
}
TEST(GraphTrace, BreadthAndDepthVisitsMatchReturnedOrder) {
    TestGraph graph(false);
    for (const std::string id : {"A", "B", "C"}) graph.add_vertex(id, 0);
    graph.add_edge("A", "B", 1);
    graph.add_edge("A", "C", 2);
    for (bool dfs : {false, true}) {
        ArraySequence<Event> events;
        const auto observer = [&](const Event& event) { events.append(event); };
        const auto order = dfs ? graph.DFS("A", observer) : graph.BFS("A", observer);
        std::size_t seen = 0;
        for (std::size_t i = 0; i < events.size(); ++i)
            if (events.get(i).kind == Kind::VisitVertex) {
                ASSERT_LT(seen, order.size());
                EXPECT_EQ(events.get(i).vertex, order.get(seen++));
            }
        EXPECT_EQ(seen, order.size());
        EXPECT_TRUE(Has(events, Kind::ExamineEdge));
        EXPECT_TRUE(Has(events, Kind::Complete));
    }
}
TEST(GraphTrace, DijkstraIdentifiesParallelEdgeAndRestoredPath) {
    TestGraph graph(true);
    graph.add_vertex("A", 0);
    graph.add_vertex("B", 0);
    graph.add_edge("A", "B", 9);
    graph.add_edge("A", "B", 2);
    ArraySequence<Event> events;
    const auto path = graph.Dijkstra("A", "B", [&](const Event& e) { events.append(e); });
    EXPECT_EQ(path.total_weight, 2);
    bool exact = false;
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events.get(i).kind == Kind::PathEdge) {
            const auto& e = events.get(i);
            EXPECT_EQ(e.from, "A");
            EXPECT_EQ(e.to, "B");
            EXPECT_EQ(e.edge_index, 1u);
            exact = true;
        }
    EXPECT_TRUE(exact);
    EXPECT_TRUE(Has(events, Kind::UpdateDistance));
}
TEST(GraphTrace, BellmanFordReportsPassesAndReachableNegativeCycle) {
    TestGraph graph(true);
    graph.add_vertex("A", 0);
    graph.add_vertex("B", 0);
    graph.add_edge("A", "B", 1);
    graph.add_edge("B", "B", -1);
    ArraySequence<Event> events;
    EXPECT_THROW(graph.BellmanFord("A", "B", [&](const Event& e) { events.append(e); }),
                 std::domain_error);
    EXPECT_TRUE(Has(events, Kind::BeginPass));
    EXPECT_TRUE(Has(events, Kind::Cycle));
    EXPECT_FALSE(Has(events, Kind::Complete));
}
TEST(GraphTrace, TopologicalVisitsAndCycleDetectionAreReported) {
    TestGraph graph(true);
    graph.add_vertex("A", 0);
    graph.add_vertex("B", 0);
    graph.add_edge("A", "B", 1);
    ArraySequence<Event> events;
    const auto order = graph.TopologicalSort([&](const Event& e) { events.append(e); });
    EXPECT_EQ(order.size(), 2u);
    EXPECT_TRUE(Has(events, Kind::VisitVertex));
    EXPECT_TRUE(Has(events, Kind::ExamineEdge));
    graph.add_edge("B", "A", 1);
    events.clear();
    EXPECT_THROW(graph.TopologicalSort([&](const Event& e) { events.append(e); }),
                 std::domain_error);
    EXPECT_TRUE(Has(events, Kind::Cycle));
    EXPECT_FALSE(Has(events, Kind::Complete));
}
TEST(GraphTrace, KruskalReportsAcceptedAndRejectedParallelEdges) {
    TestGraph graph(false);
    for (const std::string id : {"A", "B", "C"}) graph.add_vertex(id, 0);
    graph.add_edge("A", "B", 1);
    graph.add_edge("A", "B", 2);
    graph.add_edge("B", "C", 4);
    ArraySequence<Event> events;
    const auto forest = graph.Kruskal([&](const Event& e) { events.append(e); });
    EXPECT_EQ(forest.total_weight, 5);
    EXPECT_TRUE(Has(events, Kind::AcceptEdge));
    EXPECT_TRUE(Has(events, Kind::RejectEdge));
    std::size_t accepted = 0;
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events.get(i).kind == Kind::AcceptEdge) ++accepted;
    EXPECT_EQ(accepted, forest.edges.size());
}
TEST(GraphTrace, ColoringEventsHaveActualColorAndLoopFailure) {
    TestGraph graph(false);
    graph.add_vertex("A", 0);
    graph.add_vertex("B", 0);
    graph.add_edge("A", "B", 1);
    ArraySequence<Event> events;
    const auto result = graph.GreedyColoring([&](const Event& e) { events.append(e); });
    std::size_t assigned = 0;
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events.get(i).kind == Kind::AssignColor) {
            EXPECT_EQ(events.get(i).number, result.colors.get(events.get(i).vertex));
            ++assigned;
        }
    EXPECT_EQ(assigned, 2u);
    graph.add_edge("A", "A", 1);
    events.clear();
    EXPECT_THROW(graph.GreedyColoring([&](const Event& e) { events.append(e); }),
                 std::domain_error);
    EXPECT_TRUE(Has(events, Kind::Cycle));
}
TEST(GraphTrace, CallbackExceptionPropagatesWithoutMutatingGraph) {
    TestGraph graph(false);
    graph.add_vertex("A", 0);
    graph.add_vertex("B", 0);
    graph.add_edge("A", "B", 1);
    EXPECT_THROW(graph.BFS("A", [](const Event&) { throw std::runtime_error("stop"); }),
                 std::runtime_error);
    EXPECT_EQ(graph.vertex_count(), 2u);
    EXPECT_EQ(graph.edge_count(), 1u);
    EXPECT_EQ(graph.BFS("A").size(), 2u);
}
}  // namespace
