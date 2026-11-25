#pragma once

#include <concepts>


template <typename Id>
concept ComparableVertexId = std::default_initializable<Id> &&
    std::copyable<Id> && requires(const Id& left, const Id& right) {
        { left < right } -> std::convertible_to<bool>;
    };

template <ComparableVertexId Id>
bool EquivalentVertexIds(const Id& left, const Id& right) {
    return !(left < right) && !(right < left);
}

template <ComparableVertexId Id>
struct EdgeKey {
    Id from{};
    Id to{};

    bool operator==(const EdgeKey& other) const {
        return EquivalentVertexIds(from, other.from) &&
               EquivalentVertexIds(to, other.to);
    }
};

template <ComparableVertexId Id>
class EdgeKeyCompare {
public:
    bool operator()(const EdgeKey<Id>& left, const EdgeKey<Id>& right) const {
        if (left.from < right.from) return true;
        if (right.from < left.from) return false;
        return left.to < right.to;
    }
};

template <ComparableVertexId Id>
EdgeKey<Id> MakeEdgeKey(const Id& from, const Id& to, bool directed) {
    if (!directed && to < from) return {to, from};
    return {from, to};
}
