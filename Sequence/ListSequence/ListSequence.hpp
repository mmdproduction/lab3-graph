#pragma once

#include <cstddef>

#include "../LinkedList/LinkedList.hpp"
#include "../Sequence.hpp"

template <typename T>
class ListSequence : public Sequence<T> {
private:
    LinkedList<T> storage_;
public:
    ListSequence() = default;
    ListSequence(const ListSequence&) = default;
    ListSequence& operator=(const ListSequence&) = default;
    ListSequence(ListSequence&&) noexcept = default;
    ListSequence& operator=(ListSequence&&) noexcept = default;
    ~ListSequence() override = default;

    std::size_t size() const noexcept override { return storage_.size(); }
    bool empty() const noexcept { return storage_.empty(); }

    T& get(std::size_t index) override { return storage_.get(index); }
    const T& get(std::size_t index) const override { return storage_.get(index); }

    void prepend(const T& value) { storage_.prepend(value); }
    void append(const T& value) override { storage_.append(value); }
    void remove_at(std::size_t index) override { storage_.remove_at(index); }
    void clear() noexcept override { storage_.clear(); }
    void swap(ListSequence& other) noexcept { storage_.swap(other.storage_); }
};
