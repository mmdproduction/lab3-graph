#pragma once

#include <cstddef>

#include "../Sequence/ListSequence/ListSequence.hpp"

template <typename T>
class Queue {
private:
    ListSequence<T> items_;

public:
    bool empty() const noexcept { return items_.empty(); }
    std::size_t size() const noexcept { return items_.size(); }

    void enqueue(const T& value) { items_.append(value); }
    void dequeue() { items_.remove_at(0); }

    T& front() { return items_.get(0); }
    const T& front() const { return items_.get(0); }

    void clear() noexcept { items_.clear(); }
    void swap(Queue& other) noexcept { items_.swap(other.items_); }
};
