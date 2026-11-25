#pragma once

#include <cstddef>
#include <utility>

#include "../Sequence/DynamicArray/DynamicArray.hpp"

namespace graph_detail {
class DisjointSet {
private:
    DynamicArray<std::size_t> parents_;
    DynamicArray<std::size_t> ranks_;
    std::size_t component_count_;

public:
    explicit DisjointSet(std::size_t size)
        : parents_(size), ranks_(size), component_count_(size) {
        for (std::size_t i = 0; i < size; ++i) parents_.get(i) = i;
    }

    std::size_t find(std::size_t vertex) {
        std::size_t root = vertex;
        while (parents_.get(root) != root) root = parents_.get(root);
        while (vertex != root) {
            const std::size_t parent = parents_.get(vertex);
            parents_.get(vertex) = root;
            vertex = parent;
        }
        return root;
    }

    bool unite(std::size_t first, std::size_t second) {
        first = find(first);
        second = find(second);
        if (first == second) return false;
        if (ranks_.get(first) < ranks_.get(second)) std::swap(first, second);
        parents_.get(second) = first;
        if (ranks_.get(first) == ranks_.get(second)) ++ranks_.get(first);
        --component_count_;
        return true;
    }

    std::size_t component_count() const noexcept { return component_count_; }
};
}  // namespace graph_detail
