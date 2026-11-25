#pragma once

#include <cstddef>

#include "../Sequence/ListSequence/ListSequence.hpp"

template <typename T>
class Stack {
private:
    ListSequence<T> items_;

public:
    bool empty() const noexcept { return items_.empty(); }
    std::size_t size() const noexcept { return items_.size(); }

    void push(const T& value) { items_.prepend(value); }
    void pop() { items_.remove_at(0); }

    T& top() { return items_.get(0); }
    const T& top() const { return items_.get(0); }

    void clear() noexcept { items_.clear(); }
    void swap(Stack& other) noexcept { items_.swap(other.items_); }
};
