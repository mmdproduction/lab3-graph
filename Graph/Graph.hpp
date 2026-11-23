#pragma once

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

#include "GraphTypes.hpp"
#include "../Dictionary/AVLDictionary.hpp"
#include "../Sequence/ArraySequence/ArraySequence.hpp"

template <typename VertexID, typename VertexData, typename Weight>
class Graph {
private:
    AvlDictionary<VertexID, VertexData> vertices_;
    AvlDictionary<EdgeKey, ArraySequence<Weight>, EdgeKeyCompare> edges_;
    bool directed_;
    VertexID next_id_ = 1;

    void require_vertex(VertexID id) const {
        if (!contains_vertex(id)) throw std::out_of_range("vertex not found");
    }

    void advance_id(VertexID id) noexcept {
        if (next_id_ != kInvalidVertexID && id >= next_id_) {
            next_id_ = id == std::numeric_limits<VertexID>::max()
                ? kInvalidVertexID : static_cast<VertexID>(id + 1);
        }
    }

    ArraySequence<VertexID> collect_neighbors(VertexID id, bool outgoing) const {
        require_vertex(id);
        ArraySequence<VertexID> result;
        edges_.for_each([&](const EdgeKey& key, const Weight&) {
            const VertexID source = outgoing ? key.from : key.to;
            const VertexID target = outgoing ? key.to : key.from;
            if (source == id) result.append(target);
            else if (!directed_ && target == id) result.append(source);
        });
        return result;
    }

    std::size_t count_connections(VertexID id, bool outgoing) const {
        require_vertex(id);
        std::size_t result = 0;
        edges_.for_each([&](const EdgeKey& key, const Weight&) {
            if (directed_ ? (outgoing ? key.from == id : key.to == id)
                          : key.from == id || key.to == id) ++result;
        });
        return result;
    }
public:
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
        std::swap(next_id_, other.next_id_);
    }

    bool is_directed() const noexcept { return directed_; }
    bool empty() const noexcept { return vertices_.size() == 0; }
    std::size_t vertex_count() const noexcept { return vertices_.size(); }
    std::size_t edge_count() const noexcept { return edges_.size(); }

    VertexID add_vertex(const VertexData& data) {
        if (next_id_ == kInvalidVertexID) {
            throw std::overflow_error("vertex IDs exhausted");
        }
        const VertexID id = next_id_;
        vertices_.add(id, data);
        advance_id(id);
        return id;
    }

    // Explicit IDs are useful when restoring a graph from external data.
    void add_vertex(VertexID id, const VertexData& data) {
        if (id == kInvalidVertexID) {
            throw std::invalid_argument("vertex ID zero is reserved");
        }
        vertices_.add(id, data);
        advance_id(id);
    }

    bool contains_vertex(VertexID id) const {
        return id != kInvalidVertexID && vertices_.contains_key(id);
    }

    const VertexData& get_vertex(VertexID id) const {
        if (id == kInvalidVertexID) throw std::out_of_range("vertex not found");
        return vertices_.get(id);
    }

    void set_vertex(VertexID id, const VertexData& data) {
        require_vertex(id);
        auto replacement = vertices_;
        replacement.get(id) = data;
        vertices_.swap(replacement);
    }

    void remove_vertex(VertexID id) {
        require_vertex(id);
        ArraySequence<EdgeKey> incident;
        edges_.for_each([&](const EdgeKey& key, const Weight&) {
            if (key.from == id || key.to == id) incident.append(key);
        });

        // Stage all edge removals before committing a change to the graph.
        auto replacement = edges_;
        for (std::size_t i = 0; i < incident.size(); ++i) {
            replacement.remove(incident.get(i));
        }
        vertices_.remove(id);
        edges_.swap(replacement);
    }

    void add_edge(VertexID from, VertexID to, const Weight& weight) {
        require_vertex(from);
        require_vertex(to);
        if (from == to) throw std::invalid_argument("self loops are not supported");
        edges_.add(MakeEdgeKey(from, to, directed_), weight);
    }

    bool contains_edge(VertexID from, VertexID to) const {
        if (from == kInvalidVertexID || to == kInvalidVertexID || from == to) return false;
        return edges_.contains_key(MakeEdgeKey(from, to, directed_));
    }

    const Weight& get_edge_weight(VertexID from, VertexID to) const {
        return edges_.get(MakeEdgeKey(from, to, directed_));
    }

    void set_edge_weight(VertexID from, VertexID to, const Weight& weight) {
        const EdgeKey key = MakeEdgeKey(from, to, directed_);
        edges_.get(key);
        auto replacement = edges_;
        replacement.get(key) = weight;
        edges_.swap(replacement);
    }

    void remove_edge(VertexID from, VertexID to) {
        edges_.remove(MakeEdgeKey(from, to, directed_));
    }

    // In a directed graph neighbors are outgoing, predecessors are incoming.
    ArraySequence<VertexID> get_neighbors(VertexID id) const {
        return collect_neighbors(id, true);
    }

    ArraySequence<VertexID> get_predecessors(VertexID id) const {
        return collect_neighbors(id, false);
    }

    std::size_t out_degree(VertexID id) const { return count_connections(id, true); }
    std::size_t in_degree(VertexID id) const { return count_connections(id, false); }

    std::size_t degree(VertexID id) const {
        return directed_ ? out_degree(id) + in_degree(id) : out_degree(id);
    }

    ArraySequence<VertexID> vertex_ids() const {
        ArraySequence<VertexID> result;
        vertices_.for_each([&](VertexID id, const VertexData&) { result.append(id); });
        return result;
    }

    ArraySequence<EdgeKey> edge_keys() const {
        ArraySequence<EdgeKey> result;
        edges_.for_each([&](const EdgeKey& key, const Weight&) { result.append(key); });
        return result;
    }

    void clear_edges() noexcept { edges_.clear(); }

    void clear() noexcept {
        edges_.clear();
        vertices_.clear();
    }
};
