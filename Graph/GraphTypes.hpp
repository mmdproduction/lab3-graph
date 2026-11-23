#pragma once

#include <cstdint>

using VertexID = std::uint32_t;
inline constexpr VertexID kInvalidVertexID = 0;
struct EdgeKey {
    VertexID from = kInvalidVertexID;
    VertexID to = kInvalidVertexID;
    bool operator==(const EdgeKey&) const = default;
};

class EdgeKeyCompare {
public:
    bool operator()(const EdgeKey& left, const EdgeKey& right) const noexcept {
        if (left.from != right.from) return left.from < right.from;
        return left.to < right.to;
    }
};

inline EdgeKey MakeEdgeKey(VertexID from, VertexID to, bool directed) noexcept {
    if (!directed && to < from) return {to, from};
    return {from, to};
}
