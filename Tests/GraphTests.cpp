#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Graph/Graph.hpp"

namespace {
struct OrderedId {
    int value = 0;
    std::string label;
    bool operator<(const OrderedId& other) const { return value < other.value; }
};
struct UnorderedId {};
struct EqualityOnly { bool operator==(const EqualityOnly&) const = default; };
static_assert(ComparableVertexId<int>);
static_assert(ComparableVertexId<std::string>);
static_assert(ComparableVertexId<OrderedId>);
static_assert(!ComparableVertexId<UnorderedId>);
static_assert(!ComparableVertexId<EqualityOnly>);

template <typename T>
std::vector<T> Values(const ArraySequence<T>& sequence) {
    std::vector<T> result;
    for (std::size_t i = 0; i < sequence.size(); ++i) result.push_back(sequence.get(i));
    return result;
}

TEST(GraphTypes, OrdersGenericPairsAndNormalizesUndirectedEdges) {
    const EdgeKey<std::string> ab{"a", "b"}, ba{"b", "a"};
    const EdgeKeyCompare<std::string> compare;
    EXPECT_TRUE(compare(ab, ba));
    EXPECT_FALSE(compare(ab, ab));
    EXPECT_EQ(MakeEdgeKey(std::string("b"), std::string("a"), false), ab);
    EXPECT_EQ(MakeEdgeKey(std::string("b"), std::string("a"), true), ba);
    EXPECT_EQ(MakeEdgeKey(OrderedId{1, "x"}, OrderedId{2, "y"}, false),
              (EdgeKey<OrderedId>{{1, "alias"}, {2, "other"}}));
}

TEST(GraphVertices, UsesExplicitStringAndPreviouslyReservedIds) {
    Graph<std::string, int, double> graph(false);
    graph.add_vertex("", 10); graph.add_vertex("assets", 20);
    EXPECT_EQ(graph.get_vertex(""), 10);
    EXPECT_TRUE(graph.contains_vertex("assets"));
    EXPECT_FALSE(graph.contains_vertex("missing"));
    EXPECT_THROW(graph.add_vertex("assets", 9), std::invalid_argument);
    EXPECT_THROW(graph.get_vertex("missing"), std::out_of_range);
    graph.set_vertex("assets", 30); EXPECT_EQ(graph.get_vertex("assets"), 30);
    EXPECT_EQ(Values(graph.vertex_ids()), (std::vector<std::string>{"", "assets"}));
    Graph<int, std::string, int> numbers(true);
    numbers.add_vertex(0, "zero"); numbers.add_vertex(-5, "negative");
    EXPECT_EQ(numbers.get_vertex(0), "zero");
    EXPECT_EQ(numbers.get_vertex(-5), "negative");
}

TEST(GraphVertices, AcceptsOrderingOnlyIdsAndUsesOrderingEquivalence) {
    Graph<OrderedId, int, int> graph(false);
    graph.add_vertex({1, "first"}, 10); graph.add_vertex({2, "second"}, 20);
    EXPECT_TRUE(graph.contains_vertex({1, "alias"}));
    EXPECT_THROW(graph.add_vertex(OrderedId{1, "duplicate"}, 7), std::invalid_argument);
    graph.add_edge({2, "alias"}, {1, "alias"}, 8);
    graph.add_edge({1, "other"}, {1, "loop"}, 3);
    EXPECT_EQ(graph.get_edge_weight({1, "x"}, {2, "y"}, 0), 8);
    EXPECT_EQ(graph.degree({1, "x"}), 3u);
    EXPECT_EQ(graph.get_neighbors({1, "x"}).size(), 2u);
    graph.remove_vertex({1, "alias"});
    EXPECT_EQ(graph.edge_count(), 0u);
}

TEST(GraphParallel, DirectedGroupsHaveIndependentDirectionsAndAllowEqualWeights) {
    Graph<int, int, int> graph(true); graph.add_vertex(1, 1); graph.add_vertex(2, 2);
    EXPECT_EQ(graph.add_edge(1, 2, 7), 0u);
    EXPECT_EQ(graph.add_edge(1, 2, 7), 1u);
    EXPECT_EQ(graph.add_edge(1, 2, -3), 2u);
    graph.add_edge(2, 1, 99);
    EXPECT_EQ(graph.edge_count(), 4u); EXPECT_EQ(graph.edge_count(1, 2), 3u);
    EXPECT_EQ(Values(graph.get_edge_weights(1, 2)), (std::vector<int>{7, 7, -3}));
    graph.set_edge_weight(1, 2, 1, 8);
    EXPECT_EQ(graph.get_edge_weight(1, 2, 0), 7);
    EXPECT_EQ(graph.get_edge_weight(1, 2, 1), 8);
    EXPECT_EQ(graph.get_edge_weight(2, 1, 0), 99);
    EXPECT_EQ(graph.edge_keys().size(), 2u);
    EXPECT_EQ(Values(graph.get_neighbors(1)), (std::vector<int>{2}));
}

TEST(GraphParallel, UndirectedReverseUsesOneGroupAndIndexesShiftAfterRemoval) {
    Graph<std::string, int, int> graph(false); graph.add_vertex("a", 1); graph.add_vertex("b", 2);
    graph.add_edge("a", "b", 0); graph.add_edge("b", "a", 10); graph.add_edge("a", "b", 20);
    EXPECT_EQ(graph.edge_count(), 3u); EXPECT_EQ(graph.edge_keys().size(), 1u);
    EXPECT_EQ(graph.edge_keys().get(0), (EdgeKey<std::string>{"a", "b"}));
    graph.remove_edge("b", "a", 1);
    EXPECT_EQ(Values(graph.get_edge_weights("a", "b")), (std::vector<int>{0, 20}));
    graph.remove_edge("a", "b", 0); graph.remove_edge("b", "a", 0);
    EXPECT_FALSE(graph.contains_edge("a", "b")); EXPECT_EQ(graph.edge_count(), 0u);
    EXPECT_EQ(graph.edge_keys().size(), 0u);
}

TEST(GraphLoops, DirectedLoopsCountOnceInEachDirection) {
    Graph<int, int, int> graph(true); graph.add_vertex(1, 1); graph.add_vertex(2, 2); graph.add_vertex(3, 3);
    graph.add_edge(1, 1, 4); graph.add_edge(1, 1, 5); graph.add_edge(1, 2, 6); graph.add_edge(2, 1, 7);
    EXPECT_TRUE(graph.contains_edge(1, 1)); EXPECT_EQ(graph.edge_count(), 4u);
    EXPECT_EQ(graph.in_degree(1), 3u); EXPECT_EQ(graph.out_degree(1), 3u); EXPECT_EQ(graph.degree(1), 6u);
    EXPECT_EQ(Values(graph.get_neighbors(1)), (std::vector<int>{1, 2}));
    EXPECT_EQ(Values(graph.get_predecessors(1)), (std::vector<int>{1, 2}));
    EXPECT_EQ(graph.degree(3), 0u); EXPECT_EQ(graph.get_neighbors(3).size(), 0u);
    graph.remove_edge(1, 1, 0); EXPECT_EQ(graph.degree(1), 4u);
}

TEST(GraphLoops, UndirectedLoopsContributeTwiceAndAppearAsOneNeighbor) {
    Graph<int, int, int> graph(false); graph.add_vertex(1, 1); graph.add_vertex(2, 2);
    graph.add_edge(1, 1, 4); graph.add_edge(1, 1, 5); graph.add_edge(1, 2, 6); graph.add_edge(2, 1, 7);
    EXPECT_EQ(graph.edge_count(), 4u); EXPECT_EQ(graph.degree(1), 6u); EXPECT_EQ(graph.degree(2), 2u);
    EXPECT_EQ(graph.in_degree(1), 6u); EXPECT_EQ(graph.out_degree(1), 6u);
    EXPECT_EQ(Values(graph.get_neighbors(1)), (std::vector<int>{1, 2}));
    EXPECT_EQ(Values(graph.get_predecessors(1)), (std::vector<int>{1, 2}));
    graph.remove_edges(1, 1); EXPECT_EQ(graph.degree(1), 2u);
}

TEST(GraphRemoval, VertexDeletionRemovesAllIncidentGroupsAndLoops) {
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        for (int id : {1, 2, 3}) graph.add_vertex(id, id);
        graph.add_edge(1, 1, 1); graph.add_edge(1, 1, 2);
        graph.add_edge(1, 2, 3); graph.add_edge(2, 1, 4);
        graph.add_edge(3, 1, 5); graph.add_edge(2, 3, 6);
        graph.remove_vertex(1);
        EXPECT_FALSE(graph.contains_vertex(1)); EXPECT_EQ(graph.vertex_count(), 2u);
        EXPECT_EQ(graph.edge_count(), 1u); EXPECT_EQ(graph.get_edge_weight(2, 3, 0), 6);
        graph.add_vertex(1, 9); EXPECT_EQ(graph.degree(1), 0u);
    }
}

TEST(GraphErrors, RejectsMissingEndpointsAndInvalidIndexesWithoutChanges) {
    Graph<int, int, int> graph(false); graph.add_vertex(0, 1); graph.add_edge(0, 0, 7);
    EXPECT_THROW(graph.add_edge(0, 9, 1), std::out_of_range);
    EXPECT_THROW(graph.add_edge(9, 9, 1), std::out_of_range);
    EXPECT_FALSE(graph.contains_edge(0, 9)); EXPECT_EQ(graph.edge_count(0, 9), 0u);
    EXPECT_THROW(graph.get_edge_weights(0, 9), std::out_of_range);
    EXPECT_THROW(graph.get_edge_weight(0, 0, 1), std::out_of_range);
    EXPECT_THROW(graph.set_edge_weight(0, 0, 1, 2), std::out_of_range);
    EXPECT_THROW(graph.remove_edge(0, 0, 1), std::out_of_range);
    EXPECT_THROW(graph.remove_edges(0, 9), std::out_of_range);
    EXPECT_THROW(graph.remove_vertex(9), std::out_of_range);
    EXPECT_THROW(graph.get_neighbors(9), std::out_of_range);
    EXPECT_THROW(graph.get_predecessors(9), std::out_of_range);
    EXPECT_THROW(graph.degree(9), std::out_of_range);
    EXPECT_EQ(graph.edge_count(), 1u); EXPECT_EQ(graph.get_edge_weight(0, 0, 0), 7);
}

TEST(GraphValueSemantics, CopiesBucketsIndependentlyAndPreservesModes) {
    Graph<int, int, int> source(false); source.add_vertex(1, 1); source.add_edge(1, 1, 2); source.add_edge(1, 1, 3);
    auto copy = source; copy.set_vertex(1, 9); copy.set_edge_weight(1, 1, 0, 8); copy.remove_edge(1, 1, 1);
    EXPECT_EQ(source.get_vertex(1), 1); EXPECT_EQ(source.edge_count(), 2u); EXPECT_EQ(source.get_edge_weight(1, 1, 0), 2);
    Graph<int, int, int> assigned(true); assigned = source;
    EXPECT_FALSE(assigned.is_directed()); EXPECT_EQ(assigned.edge_count(), 2u);
    const auto& self = assigned; assigned = self; assigned.swap(assigned);
    EXPECT_EQ(assigned.edge_count(), 2u);
    auto ids = source.vertex_ids(); ids.get(0) = 100; EXPECT_TRUE(source.contains_vertex(1));
    assigned.clear_edges(); EXPECT_EQ(assigned.edge_count(), 0u); EXPECT_EQ(assigned.vertex_count(), 1u);
    assigned.clear(); EXPECT_TRUE(assigned.empty()); EXPECT_FALSE(assigned.is_directed());
    assigned.add_vertex(1, 5); assigned.swap(copy); EXPECT_EQ(assigned.get_vertex(1), 9);
}

struct Fragile {
    static inline int copy_budget = -1;
    static inline int assign_budget = -1;
    static inline int default_budget = -1;
    int value = 0;
    static void Spend(int& budget) {
        if (budget == 0) throw std::runtime_error("injected failure");
        if (budget > 0) --budget;
    }
    Fragile() { Spend(default_budget); }
    explicit Fragile(int number) : value(number) {}
    Fragile(const Fragile& other) : value(other.value) { Spend(copy_budget); }
    Fragile& operator=(const Fragile& other) {
        value = -1; Spend(assign_budget); value = other.value; return *this;
    }
    static void Reset() { copy_budget = assign_budget = default_budget = -1; }
};

TEST(GraphExceptions, FailedWeightOperationsPreserveOriginalBucket) {
    Graph<int, int, Fragile> graph(false); graph.add_vertex(1, 1); graph.add_vertex(2, 2);
    graph.add_edge(1, 1, Fragile(7)); graph.add_edge(1, 1, Fragile(8));
    Fragile::assign_budget = 0;
    EXPECT_THROW(graph.set_edge_weight(1, 1, 0, Fragile(99)), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.get_edge_weight(1, 1, 0).value, 7);
    Fragile::copy_budget = 0;
    EXPECT_THROW(graph.add_edge(1, 1, Fragile(9)), std::runtime_error); Fragile::Reset();
    Fragile::copy_budget = 0;
    EXPECT_THROW(graph.add_edge(1, 2, Fragile(9)), std::runtime_error); Fragile::Reset();
    Fragile::assign_budget = 0;
    EXPECT_THROW(graph.remove_edge(1, 1, 0), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.edge_count(), 2u); EXPECT_FALSE(graph.contains_edge(1, 2));
    EXPECT_EQ(graph.get_edge_weight(1, 1, 1).value, 8);
    // Two assignments finish the bucket copy; the edit itself then throws.
    Fragile::assign_budget = 2;
    EXPECT_THROW(graph.set_edge_weight(1, 1, 0, Fragile(99)), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.get_edge_weight(1, 1, 0).value, 7);
    Fragile::assign_budget = 2;
    EXPECT_THROW(graph.remove_edge(1, 1, 0), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.edge_count(1, 1), 2u);
    EXPECT_EQ(graph.get_edge_weight(1, 1, 0).value, 7);
    graph.add_edge(1, 1, graph.get_edge_weight(1, 1, 0));
    EXPECT_EQ(graph.get_edge_weight(1, 1, 2).value, 7);
}

TEST(GraphExceptions, FailedVertexDeletionOrCopyAssignmentPreservesGraph) {
    Graph<int, Fragile, Fragile> graph(false);
    graph.add_vertex(1, Fragile(1)); graph.add_vertex(2, Fragile(2)); graph.add_edge(1, 1, Fragile(3)); graph.add_edge(1, 2, Fragile(4));
    // ArraySequence copies its elements by assignment inside DynamicArray.
    Fragile::assign_budget = 0;
    EXPECT_THROW(graph.remove_vertex(1), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.vertex_count(), 2u); EXPECT_EQ(graph.edge_count(), 2u);
    Fragile::default_budget = 1;
    EXPECT_THROW(graph.remove_vertex(1), std::runtime_error); Fragile::Reset();
    EXPECT_TRUE(graph.contains_vertex(1)); EXPECT_EQ(graph.edge_count(), 2u);
    Fragile::assign_budget = 0;
    EXPECT_THROW(graph.set_vertex(1, Fragile(9)), std::runtime_error); Fragile::Reset();
    EXPECT_EQ(graph.get_vertex(1).value, 1);
    Graph<int, Fragile, Fragile> target(true); target.add_vertex(5, Fragile(5));
    Fragile::copy_budget = 0;
    EXPECT_THROW(target = graph, std::runtime_error); Fragile::Reset();
    EXPECT_TRUE(target.is_directed()); EXPECT_EQ(target.vertex_count(), 1u); EXPECT_EQ(target.get_vertex(5).value, 5);

    Graph<int, Fragile, int> late_failure(false);
    late_failure.add_vertex(1, Fragile(1)); late_failure.add_vertex(2, Fragile(2));
    late_failure.add_edge(1, 1, 3); late_failure.add_edge(1, 2, 4);
    Fragile::default_budget = 1; // require_vertex succeeds; actual vertex erase fails.
    EXPECT_THROW(late_failure.remove_vertex(1), std::runtime_error); Fragile::Reset();
    EXPECT_TRUE(late_failure.contains_vertex(1)); EXPECT_EQ(late_failure.edge_count(), 2u);
}

TEST(GraphModel, RandomOperationsAgreeWithIndependentMultigraphModel) {
    for (bool directed : {false, true}) {
        Graph<int, int, int> graph(directed);
        std::map<std::pair<int, int>, std::vector<int>> model;
        for (int id = -2; id <= 2; ++id) graph.add_vertex(id, id);
        std::mt19937 random(20261125);
        for (int step = 0; step < 400; ++step) {
            int from = static_cast<int>(random() % 5) - 2, to = static_cast<int>(random() % 5) - 2;
            const auto key = !directed && to < from ? std::make_pair(to, from) : std::make_pair(from, to);
            const auto action = random() % 4;
            auto found = model.find(key);
            if (action == 0 || found == model.end()) {
                const int weight = static_cast<int>(random() % 9) - 4;
                const auto index = model[key].size(); model[key].push_back(weight);
                EXPECT_EQ(graph.add_edge(from, to, weight), index);
            } else if (action == 1) {
                const auto index = random() % found->second.size(); graph.remove_edge(from, to, index);
                found->second.erase(found->second.begin() + static_cast<std::ptrdiff_t>(index));
                if (found->second.empty()) model.erase(found);
            } else if (action == 2) {
                graph.remove_edges(from, to); model.erase(found);
            } else {
                const auto index = random() % found->second.size(); const int weight = static_cast<int>(random() % 7);
                graph.set_edge_weight(from, to, index, weight); found->second[index] = weight;
            }
            std::size_t count = 0;
            for (const auto& [pair, weights] : model) {
                count += weights.size(); EXPECT_EQ(Values(graph.get_edge_weights(pair.first, pair.second)), weights);
            }
            ASSERT_EQ(graph.edge_count(), count); ASSERT_EQ(graph.edge_keys().size(), model.size());
            for (int id = -2; id <= 2; ++id) {
                std::size_t incoming = 0, outgoing = 0; std::set<int> neighbors, predecessors;
                for (const auto& [pair, weights] : model) {
                    if (pair.first == id) { outgoing += weights.size(); neighbors.insert(pair.second); }
                    if (pair.second == id) { incoming += weights.size(); predecessors.insert(pair.first); }
                    if (!directed) {
                        if (pair.second == id) { outgoing += weights.size(); neighbors.insert(pair.first); }
                        if (pair.first == id) { incoming += weights.size(); predecessors.insert(pair.second); }
                    }
                }
                EXPECT_EQ(graph.out_degree(id), outgoing); EXPECT_EQ(graph.in_degree(id), incoming);
                EXPECT_EQ(graph.degree(id), directed ? incoming + outgoing : outgoing);
                auto actual = Values(graph.get_neighbors(id)); auto actual_pre = Values(graph.get_predecessors(id));
                EXPECT_EQ(actual.size(), neighbors.size()); EXPECT_EQ(actual_pre.size(), predecessors.size());
                EXPECT_EQ(std::set<int>(actual.begin(), actual.end()), neighbors);
                EXPECT_EQ(std::set<int>(actual_pre.begin(), actual_pre.end()), predecessors);
            }
        }
    }
}
}  // namespace
