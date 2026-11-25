#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "App/Gui/GraphDocument.hpp"
#include "App/Gui/TraceRunner.hpp"

TEST(GraphDocument, EditingMaintainsTopologyAndPositions) {
    GraphDocument doc;
    doc.add_vertex("A", {10, 20});
    doc.add_vertex("B", {100, 200});
    doc.add_edge("A", "B", 2);
    doc.add_edge("A", "B", 3);
    doc.add_edge("A", "A", 4);
    doc.rename_vertex("A", "C");
    EXPECT_FALSE(doc.graph().contains_vertex("A"));
    EXPECT_EQ(doc.graph().edge_count("C", "B"), 2u);
    EXPECT_EQ(doc.position("C"), QPointF(10, 20));
    doc.set_position("C", {12, 30});
    EXPECT_EQ(doc.position("C"), QPointF(12, 30));
    doc.remove_vertex("C");
    EXPECT_EQ(doc.graph().edge_count(), 0u);
    EXPECT_EQ(doc.graph().vertex_count(), 1u);
}
TEST(GraphDocument, RejectsInvalidInputAndKeepsGraph) {
    GraphDocument doc;
    doc.add_vertex("A", {});
    EXPECT_THROW(doc.add_vertex("A", {}), std::invalid_argument);
    EXPECT_THROW(doc.add_vertex("", {}), std::invalid_argument);
    EXPECT_THROW(doc.add_edge("A", "A", std::numeric_limits<double>::infinity()),
                 std::invalid_argument);
    EXPECT_THROW(doc.set_position("A", {std::numeric_limits<double>::quiet_NaN(), 0}),
                 std::invalid_argument);
    EXPECT_EQ(doc.graph().edge_count(), 0u);
    EXPECT_EQ(doc.graph().vertex_count(), 1u);
}
TEST(GraphDocument, JsonRoundTripIncludesParallelLoopsAndPositions) {
    GraphDocument doc;
    doc.reset(true);
    doc.add_vertex("узел", {10, 20});
    doc.add_vertex("B", {30, 40});
    doc.add_edge("узел", "B", -1.5);
    doc.add_edge("узел", "B", 2);
    doc.add_edge("B", "B", 0);
    GraphDocument loaded;
    loaded.load_json(doc.to_json());
    EXPECT_TRUE(loaded.graph().is_directed());
    EXPECT_EQ(loaded.graph().vertex_count(), 2u);
    EXPECT_EQ(loaded.graph().edge_count(), 3u);
    EXPECT_EQ(loaded.position("узел"), QPointF(10, 20));
    EXPECT_EQ(loaded.graph().get_edge_weight("узел", "B", 0), -1.5);
    EXPECT_THROW(loaded.load_json("{\"vertices\":[{}]}"), std::invalid_argument);
    EXPECT_EQ(loaded.graph().edge_count(), 3u);
    EXPECT_THROW(loaded.load_json("not json"), std::invalid_argument);
    EXPECT_EQ(loaded.graph().vertex_count(), 2u);
}
TEST(GraphDocument, PresetsAreSmallAndCoverRequiredGraphTypes) {
    GraphDocument doc;
    for (int preset = 0; preset < 5; ++preset) {
        doc.load_example(preset);
        EXPECT_GE(doc.graph().vertex_count(), 4u);
        EXPECT_LE(doc.graph().vertex_count(), 12u);
        const auto ids = doc.graph().vertex_ids();
        for (std::size_t i = 0; i < ids.size(); ++i)
            EXPECT_TRUE(std::isfinite(doc.position(ids.get(i)).x()));
    }
    doc.load_example(1);
    EXPECT_TRUE(doc.graph().is_directed());
    EXPECT_NO_THROW(doc.graph().TopologicalSort());
    doc.load_example(2);
    EXPECT_TRUE(doc.graph().is_directed());
    EXPECT_NO_THROW(doc.graph().BellmanFord("A", "F"));
}
TEST(TraceRunner, EveryAlgorithmRecordsActualStepsAndRewinds) {
    GraphDocument doc;
    for (int a = 0; a < 7; ++a) {
        const auto algorithm = static_cast<Algorithm>(a);
        doc.load_example(algorithm == Algorithm::Topological   ? 1
                         : algorithm == Algorithm::BellmanFord ? 2
                                                               : 0);
        TraceRunner runner;
        runner.prepare(doc.graph(), algorithm, "A", "F");
        EXPECT_FALSE(runner.has_error());
        ASSERT_GT(runner.size(), 2u);
        EXPECT_EQ(runner.cursor(), 0u);
        EXPECT_FALSE(runner.state().current.has_value());
        runner.seek(runner.size());
        EXPECT_FALSE(runner.state().summary.isEmpty());
        runner.seek(1);
        EXPECT_EQ(runner.cursor(), 1u);
        runner.seek(0);
        const auto initial = runner.state();
        initial.vertices.for_each([](const std::string&, const VertexVisual& v) {
            EXPECT_EQ(v.tone, VertexTone::Idle);
            EXPECT_EQ(v.color, -1);
        });
        runner.seek(999999);
        EXPECT_EQ(runner.cursor(), runner.size());
    }
}
TEST(TraceRunner, ColoringSeekRestoresColorsWithoutFutureInformation) {
    GraphDocument doc;
    doc.load_example(4);
    TraceRunner runner;
    runner.prepare(doc.graph(), Algorithm::Coloring, "A", "F");
    runner.seek(runner.size());
    const auto final = runner.state();
    EXPECT_GE(final.color_count, 3u);
    const auto keys = doc.graph().edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i)
        EXPECT_NE(final.vertices.get(keys.get(i).from).color,
                  final.vertices.get(keys.get(i).to).color);
    runner.seek(0);
    EXPECT_EQ(runner.state().color_count, 0u);
    runner.seek(runner.size());
    EXPECT_EQ(runner.state().vertices.get("A").color, final.vertices.get("A").color);
}
TEST(TraceRunner, ErrorsAreReplayableAndNotReportedAsSuccessful) {
    GraphDocument doc;
    doc.reset(true);
    doc.add_vertex("A", {});
    doc.add_edge("A", "A", -1);
    TraceRunner runner;
    runner.prepare(doc.graph(), Algorithm::BellmanFord, "A", "A");
    EXPECT_TRUE(runner.has_error());
    runner.seek(runner.size());
    EXPECT_TRUE(runner.state().is_error);
    EXPECT_FALSE(runner.state().explanation.isEmpty());
    runner.seek(0);
    EXPECT_FALSE(runner.state().is_error);
    runner.prepare(doc.graph(), Algorithm::Kruskal, "A", "A");
    EXPECT_TRUE(runner.has_error());
    runner.prepare(doc.graph(), Algorithm::Coloring, "A", "A");
    EXPECT_TRUE(runner.has_error());
}
TEST(TraceRunner, ParallelShortestPathMarksOnlyChosenEdge) {
    GraphDocument doc;
    doc.add_vertex("A", {});
    doc.add_vertex("B", {100, 0});
    doc.add_edge("A", "B", 9);
    doc.add_edge("A", "B", 2);
    TraceRunner runner;
    runner.prepare(doc.graph(), Algorithm::Dijkstra, "A", "B");
    runner.seek(runner.size());
    const auto state = runner.state();
    std::size_t selected = 0;
    for (std::size_t i = 0; i < state.edges.size(); ++i)
        if (state.edges.get(i).tone == EdgeTone::Accepted) {
            EXPECT_EQ(state.edges.get(i).ref.index, 1u);
            ++selected;
        }
    EXPECT_EQ(selected, 1u);
}

TEST(GraphGenerator, ExactCountsSimpleTopologyAndPositiveWeights) {
    for (bool directed : {false, true}) {
        GraphDocument doc;
        doc.generate(12, 25, directed, 20261125);
        EXPECT_EQ(doc.graph().vertex_count(), 12u);
        EXPECT_EQ(doc.graph().edge_count(), 25u);
        EXPECT_EQ(doc.graph().is_directed(), directed);
        const auto keys = doc.graph().edge_keys();
        EXPECT_EQ(keys.size(), 25u);
        for (std::size_t i = 0; i < keys.size(); ++i) {
            const auto& key = keys.get(i);
            EXPECT_NE(key.from, key.to);
            const auto& weights = doc.graph().get_edge_weights(key.from, key.to);
            ASSERT_EQ(weights.size(), 1u);
            EXPECT_GE(weights.get(0), 1);
            EXPECT_LE(weights.get(0), 9);
        }
        const auto ids = doc.graph().vertex_ids();
        for (std::size_t i = 0; i < ids.size(); ++i) {
            EXPECT_TRUE(std::isfinite(doc.position(ids.get(i)).x()));
            EXPECT_TRUE(std::isfinite(doc.position(ids.get(i)).y()));
        }
    }
}
TEST(GraphGenerator, EmptySingletonCompleteAndMaximumGraphs) {
    GraphDocument doc;
    doc.generate(0, 0, true, 1);
    EXPECT_EQ(doc.graph().vertex_count(), 0u);
    EXPECT_EQ(doc.graph().edge_count(), 0u);
    doc.generate(1, 0, false, 1);
    EXPECT_EQ(doc.graph().vertex_count(), 1u);
    doc.generate(3, 3, false, 1);
    EXPECT_EQ(doc.graph().edge_count(), 3u);
    doc.generate(3, 6, true, 1);
    EXPECT_EQ(doc.graph().edge_count(), 6u);
    doc.generate(50, 150, false, 1);
    EXPECT_EQ(doc.graph().vertex_count(), 50u);
    EXPECT_EQ(doc.graph().edge_count(), 150u);
}
TEST(GraphGenerator, RejectsImpossibleParametersWithoutReplacingDocument) {
    GraphDocument doc;
    doc.load_example(0);
    const auto before = doc.to_json();
    EXPECT_THROW(doc.generate(0, 1, false, 1), std::invalid_argument);
    EXPECT_THROW(doc.generate(1, 1, true, 1), std::invalid_argument);
    EXPECT_THROW(doc.generate(2, 2, false, 1), std::invalid_argument);
    EXPECT_THROW(doc.generate(51, 0, false, 1), std::invalid_argument);
    EXPECT_THROW(doc.generate(50, 151, true, 1), std::invalid_argument);
    EXPECT_EQ(doc.to_json(), before);
}
TEST(GraphGenerator, SeedReproducesGraphAndGeneratedGraphCanBeSaved) {
    GraphDocument first, second, loaded;
    first.generate(9, 18, true, 42);
    second.generate(9, 18, true, 42);
    EXPECT_EQ(first.to_json(), second.to_json());
    loaded.load_json(first.to_json());
    EXPECT_EQ(loaded.to_json(), first.to_json());
    EXPECT_EQ(GraphDocument::max_generated_edges(2, false), 1u);
    EXPECT_EQ(GraphDocument::max_generated_edges(2, true), 2u);
    EXPECT_EQ(GraphDocument::max_generated_edges(0, true), 0u);
}
