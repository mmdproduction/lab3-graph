#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "GraphTypes.hpp"
#include "../Dictionary/AVLDictionary.hpp"
#include "../Sequence/ArraySequence/ArraySequence.hpp"

template <ComparableVertexId VertexID, typename VertexData, typename Weight>
class Graph {
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

private:
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

    AvlDictionary<VertexID, VertexData> vertices_;
    AvlDictionary<Key, Weights, EdgeKeyCompare<VertexID>> edges_;
    bool directed_;
};
