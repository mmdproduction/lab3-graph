#pragma once

#include <cstddef>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include "GraphTypes.hpp"
#include "PathResult.hpp"
#include "../Dictionary/AVLDictionary.hpp"
#include "../Queue/Queue.hpp"
#include "../Stack/Stack.hpp"
#include "../Sequence/ArraySequence/ArraySequence.hpp"

template <ComparableVertexId VertexID, typename VertexData, typename Weight>
class Graph {
private:
    AvlDictionary<VertexID, VertexData> vertices_;
    AvlDictionary<EdgeKey<VertexID>, ArraySequence<Weight>,
                  EdgeKeyCompare<VertexID>> edges_;
    bool directed_;

    void validate_path_weights(bool non_negative) const requires NumericPathWeight<Weight> {
        edges_.for_each([&](const EdgeKey<VertexID>&, const ArraySequence<Weight>& weights) {
            for (std::size_t i = 0; i < weights.size(); ++i) {
                const Weight& weight = weights.get(i);
                if constexpr (std::floating_point<Weight>) {
                    if (!std::isfinite(weight)) throw std::invalid_argument("path weights must be finite");
                }
                if (non_negative && weight < Weight{}) {
                    throw std::invalid_argument("Dijkstra requires nonnegative weights");
                }
            }
        });
    }

    static Weight add_path_weights(Weight left, Weight right) requires NumericPathWeight<Weight> {
        if constexpr (std::signed_integral<Weight>) {
            if ((right > 0 && left > std::numeric_limits<Weight>::max() - right) ||
                (right < 0 && left < std::numeric_limits<Weight>::lowest() - right)) {
                throw std::overflow_error("path weight overflow");
            }
        } else if constexpr (std::unsigned_integral<Weight>) {
            if (left > std::numeric_limits<Weight>::max() - right) {
                throw std::overflow_error("path weight overflow");
            }
        }
        const Weight sum = static_cast<Weight>(left + right);
        if constexpr (std::floating_point<Weight>) {
            if (!std::isfinite(sum)) throw std::overflow_error("path weight overflow");
        }
        return sum;
    }

    bool can_relax_path_edge(const AvlDictionary<VertexID, Weight>& distances,
                             const VertexID& from, const VertexID& to,
                             const Weight& weight) const requires NumericPathWeight<Weight> {
        if (!distances.contains_key(from)) return false;
        const Weight candidate = add_path_weights(distances.get(from), weight);
        return !distances.contains_key(to) || candidate < distances.get(to);
    }

    bool relax_path_edge(AvlDictionary<VertexID, Weight>& distances,
                         AvlDictionary<VertexID, VertexID>& predecessors,
                         const VertexID& from, const VertexID& to,
                         const Weight& weight) const requires NumericPathWeight<Weight> {
        if (!distances.contains_key(from)) return false;
        const Weight candidate = add_path_weights(distances.get(from), weight);
        if (distances.contains_key(to)) {
            if (!(candidate < distances.get(to))) return false;
            distances.get(to) = candidate;
        } else {
            distances.add(to, candidate);
        }
        if (predecessors.contains_key(to)) predecessors.get(to) = from;
        else predecessors.add(to, from);
        return true;
    }

    template <typename Visitor>
    void for_each_path_edge(Visitor&& visitor) const {
        edges_.for_each([&](const EdgeKey<VertexID>& key, const ArraySequence<Weight>& weights) {
            for (std::size_t i = 0; i < weights.size(); ++i) {
                visitor(key.from, key.to, weights.get(i));
                if (!directed_ && !EquivalentVertexIds(key.from, key.to)) {
                    visitor(key.to, key.from, weights.get(i));
                }
            }
        });
    }

    PathResult<VertexID, Weight> recover_path(
        const VertexID& start, const VertexID& finish,
        const AvlDictionary<VertexID, Weight>& distances,
        const AvlDictionary<VertexID, VertexID>& predecessors) const requires NumericPathWeight<Weight> {
        PathResult<VertexID, Weight> result;
        if (!distances.contains_key(finish)) return result;
        result.total_weight = distances.get(finish);
        ArraySequence<VertexID> reversed;
        VertexID current = finish;
        while (!EquivalentVertexIds(current, start)) {
            if (reversed.size() >= vertex_count()) throw std::logic_error("invalid predecessor chain");
            reversed.append(current);
            current = predecessors.get(current);
        }
        reversed.append(start);
        for (std::size_t i = reversed.size(); i > 0; --i) result.vertices.append(reversed.get(i - 1));
        result.reachable = true;
        return result;
    }

    void require_vertex(const VertexID& id) const {
        if (!contains_vertex(id)) throw std::out_of_range("vertex not found");
    }

    ArraySequence<VertexID> collect_neighbors(const VertexID& id, bool outgoing) const {
        require_vertex(id);
        ArraySequence<VertexID> result;
        edges_.for_each([&](const Key& key, const Weights&) {
            const VertexID& source = outgoing ? key.from : key.to;
            const VertexID& target = outgoing ? key.to : key.from;
            if (EquivalentVertexIds(source, id)) result.append(target);
            else if (!directed_ && EquivalentVertexIds(target, id)) result.append(source);
        });
        return result;
    }

    std::size_t count_connections(const VertexID& id, bool outgoing) const {
        require_vertex(id);
        std::size_t result = 0;
        edges_.for_each([&](const Key& key, const Weights& weights) {
            if (directed_) {
                const VertexID& endpoint = outgoing ? key.from : key.to;
                if (EquivalentVertexIds(endpoint, id)) result += weights.size();
            } else {
                if (EquivalentVertexIds(key.from, id)) result += weights.size();
                if (EquivalentVertexIds(key.to, id)) result += weights.size();
            }
        });
        return result;
    }

public:
    using Key = EdgeKey<VertexID>;
    using Weights = ArraySequence<Weight>;

    explicit Graph(bool directed) : directed_(directed) {}
    Graph(const Graph&) = default;

    Graph& operator=(const Graph& other) {
        if (this != &other) {
            Graph copy(other);
            swap(copy);
        }
        return *this;
    }

    void swap(Graph& other) noexcept {
        if (this == &other) return;
        vertices_.swap(other.vertices_);
        edges_.swap(other.edges_);
        std::swap(directed_, other.directed_);
    }

    bool is_directed() const noexcept { return directed_; }
    bool empty() const noexcept { return vertices_.size() == 0; }
    std::size_t vertex_count() const noexcept { return vertices_.size(); }

    std::size_t edge_count() const {
        std::size_t result = 0;
        edges_.for_each([&](const Key&, const Weights& weights) {
            result += weights.size();
        });
        return result;
    }

    std::size_t edge_count(const VertexID& from, const VertexID& to) const {
        const Key key = MakeEdgeKey(from, to, directed_);
        return edges_.contains_key(key) ? edges_.get(key).size() : 0;
    }

    void add_vertex(const VertexID& id, const VertexData& data) {
        vertices_.add(id, data);
    }

    bool contains_vertex(const VertexID& id) const {
        return vertices_.contains_key(id);
    }

    const VertexData& get_vertex(const VertexID& id) const {
        return vertices_.get(id);
    }

    void set_vertex(const VertexID& id, const VertexData& data) {
        require_vertex(id);
        auto replacement = vertices_;
        replacement.get(id) = data;
        vertices_.swap(replacement);
    }

    void remove_vertex(const VertexID& id) {
        require_vertex(id);
        ArraySequence<Key> incident;
        edges_.for_each([&](const Key& key, const Weights&) {
            if (EquivalentVertexIds(key.from, id) || EquivalentVertexIds(key.to, id)) {
                incident.append(key);
            }
        });

        auto replacement = edges_;
        for (std::size_t i = 0; i < incident.size(); ++i) {
            replacement.remove(incident.get(i));
        }
        vertices_.remove(id);
        edges_.swap(replacement);
    }

    std::size_t add_edge(const VertexID& from, const VertexID& to, const Weight& weight) {
        require_vertex(from);
        require_vertex(to);
        const Key key = MakeEdgeKey(from, to, directed_);
        if (!edges_.contains_key(key)) {
            Weights weights;
            weights.append(weight);
            edges_.add(key, weights);
            return 0;
        }

        auto& weights = edges_.get(key);
        const std::size_t index = weights.size();
        Weights replacement(weights);
        replacement.append(weight);
        weights.swap(replacement);
        return index;
    }

    bool contains_edge(const VertexID& from, const VertexID& to) const {
        return edges_.contains_key(MakeEdgeKey(from, to, directed_));
    }

    const Weights& get_edge_weights(const VertexID& from, const VertexID& to) const {
        return edges_.get(MakeEdgeKey(from, to, directed_));
    }

    const Weight& get_edge_weight(const VertexID& from, const VertexID& to,
                                  std::size_t index) const {
        return get_edge_weights(from, to).get(index);
    }

    void set_edge_weight(const VertexID& from, const VertexID& to,
                         std::size_t index, const Weight& weight) {
        auto& weights = edges_.get(MakeEdgeKey(from, to, directed_));
        weights.get(index);  // Validate before copying the bucket.
        Weights replacement(weights);
        replacement.get(index) = weight;
        weights.swap(replacement);
    }

    void remove_edge(const VertexID& from, const VertexID& to, std::size_t index) {
        const Key key = MakeEdgeKey(from, to, directed_);
        auto& weights = edges_.get(key);
        weights.get(index);
        if (weights.size() == 1) {
            edges_.remove(key);
            return;
        }
        Weights replacement(weights);
        replacement.remove_at(index);
        weights.swap(replacement);
    }

    void remove_edges(const VertexID& from, const VertexID& to) {
        edges_.remove(MakeEdgeKey(from, to, directed_));
    }

    // Neighbor lists contain each ID once, regardless of parallel edges.
    ArraySequence<VertexID> get_neighbors(const VertexID& id) const {
        return collect_neighbors(id, true);
    }

    ArraySequence<VertexID> get_predecessors(const VertexID& id) const {
        return collect_neighbors(id, false);
    }

    std::size_t out_degree(const VertexID& id) const { return count_connections(id, true); }
    std::size_t in_degree(const VertexID& id) const { return count_connections(id, false); }

    std::size_t degree(const VertexID& id) const {
        return directed_ ? out_degree(id) + in_degree(id) : out_degree(id);
    }

    ArraySequence<VertexID> vertex_ids() const {
        ArraySequence<VertexID> result;
        vertices_.for_each([&](const VertexID& id, const VertexData&) { result.append(id); });
        return result;
    }

    ArraySequence<Key> edge_keys() const {
        ArraySequence<Key> result;
        edges_.for_each([&](const Key& key, const Weights&) { result.append(key); });
        return result;
    }

    void clear_edges() noexcept { edges_.clear(); }
    void clear() noexcept {
        edges_.clear();
        vertices_.clear();
    }


    //algoritms (((((CODEX))))) P.S. GEMINI

    ArraySequence<VertexID> BFS(const VertexID& start) const {
        if (!contains_vertex(start)) throw std::out_of_range("start vertex not found");

        ArraySequence<VertexID> order;
        AvlDictionary<VertexID, bool> visited;
        Queue<VertexID> pending;
        visited.add(start, true);
        pending.enqueue(start);

        while (!pending.empty()) {
            const VertexID current = pending.front();
            pending.dequeue();
            order.append(current);
            const auto neighbors = get_neighbors(current);
            for (std::size_t i = 0; i < neighbors.size(); ++i) {
                const VertexID& next = neighbors.get(i);
                if (!visited.contains_key(next)) {
                    visited.add(next, true);
                    pending.enqueue(next);
                }
            }
        }
        return order;
    }
    ArraySequence<VertexID> DFS(const VertexID& start) const {
        if (!contains_vertex(start)) throw std::out_of_range("start vertex not found");

        ArraySequence<VertexID> order;
        AvlDictionary<VertexID, bool> visited;
        Stack<VertexID> pending;
        pending.push(start);

        while (!pending.empty()) {
            const VertexID current = pending.top();
            pending.pop();
            if (visited.contains_key(current)) continue;
            visited.add(current, true);
            order.append(current);

            const auto neighbors = get_neighbors(current);
            for (std::size_t i = neighbors.size(); i > 0; --i) {
                const VertexID& next = neighbors.get(i - 1);
                if (!visited.contains_key(next)) pending.push(next);
            }
        }
        return order;
    }
    PathResult<VertexID, Weight> Dijkstra(const VertexID& start, const VertexID& finish) const
        requires NumericPathWeight<Weight> {
        require_vertex(start);
        require_vertex(finish);
        validate_path_weights(true);
        AvlDictionary<VertexID, Weight> distances;
        AvlDictionary<VertexID, VertexID> predecessors;
        AvlDictionary<VertexID, bool> settled;
        distances.add(start, Weight{});

        while (true) {
            bool found = false;
            VertexID current{};
            Weight nearest{};
            distances.for_each([&](const VertexID& id, const Weight& distance) {
                if (!settled.contains_key(id) && (!found || distance < nearest)) {
                    current = id;
                    nearest = distance;
                    found = true;
                }
            });
            if (!found) break;
            settled.add(current, true);
            if (EquivalentVertexIds(current, finish)) break;
            edges_.for_each([&](const Key& key, const Weights& weights) {
                const VertexID* next = nullptr;
                if (EquivalentVertexIds(key.from, current)) next = &key.to;
                else if (!directed_ && EquivalentVertexIds(key.to, current)) next = &key.from;
                if (!next || settled.contains_key(*next)) return;
                for (std::size_t i = 0; i < weights.size(); ++i) {
                    relax_path_edge(distances, predecessors, current, *next, weights.get(i));
                }
            });
        }
        return recover_path(start, finish, distances, predecessors);
    }

    PathResult<VertexID, Weight> BellmanFord(const VertexID& start, const VertexID& finish) const
        requires NumericPathWeight<Weight> {
        require_vertex(start);
        require_vertex(finish);
        validate_path_weights(false);
        AvlDictionary<VertexID, Weight> distances;
        AvlDictionary<VertexID, VertexID> predecessors;
        distances.add(start, Weight{});
        for (std::size_t pass = 1; pass < vertex_count(); ++pass) {
            bool changed = false;
            for_each_path_edge([&](const VertexID& from, const VertexID& to, const Weight& weight) {
                if (relax_path_edge(distances, predecessors, from, to, weight)) changed = true;
            });
            if (!changed) break;
        }
        for_each_path_edge([&](const VertexID& from, const VertexID& to, const Weight& weight) {
            if (can_relax_path_edge(distances, from, to, weight)) {
                throw std::domain_error("reachable negative cycle");
            }
        });
        return recover_path(start, finish, distances, predecessors);
    }

    ArraySequence<VertexID> TopologicalSort() const {
        if (!directed_) throw std::invalid_argument("topological sort requires a directed graph");

        AvlDictionary<VertexID, std::size_t> incoming;
        vertices_.for_each([&](const VertexID& id, const VertexData&) { incoming.add(id, 0); });
        edges_.for_each([&](const Key& key, const Weights& weights) {
            incoming.get(key.to) += weights.size();
        });

        Queue<VertexID> ready;
        vertices_.for_each([&](const VertexID& id, const VertexData&) {
            if (incoming.get(id) == 0) ready.enqueue(id);
        });
        ArraySequence<VertexID> order;
        while (!ready.empty()) {
            const VertexID current = ready.front();
            ready.dequeue();
            order.append(current);
            edges_.for_each([&](const Key& key, const Weights& weights) {
                if (EquivalentVertexIds(key.from, current)) {
                    auto& count = incoming.get(key.to);
                    count -= weights.size();
                    if (count == 0) ready.enqueue(key.to);
                }
            });
        }
        if (order.size() != vertex_count()) throw std::domain_error("graph contains a directed cycle");
        return order;
    }
};
