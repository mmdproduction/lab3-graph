#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class LinkedList {
private:
    struct Node {
        T value;
        Node* next = nullptr;
        explicit Node(const T& value) : value(value) {}
    };

    Node* node_at(std::size_t index) const {
        if (index >= size_) throw std::out_of_range("list index out of range");
        Node* node = head_;
        for (std::size_t i = 0; i < index; ++i) node = node->next;
        return node;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
public:
    LinkedList() = default;

    LinkedList(const LinkedList& other) {
        try {
            for (const Node* node = other.head_; node; node = node->next) {
                append(node->value);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList copy(other);
            swap(copy);
        }
        return *this;
    }

    LinkedList(LinkedList&& other) noexcept { swap(other); }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    T& get(std::size_t index) { return node_at(index)->value; }
    const T& get(std::size_t index) const { return node_at(index)->value; }

    void append(const T& value) {
        // Construct the payload before changing any links.
        Node* node = new Node(value);
        if (tail_) tail_->next = node;
        else head_ = node;
        tail_ = node;
        ++size_;
    }

    void remove_at(std::size_t index) {
        if (index >= size_) throw std::out_of_range("list index out of range");
        Node* previous = index == 0 ? nullptr : node_at(index - 1);
        Node* removed = previous ? previous->next : head_;
        if (previous) previous->next = removed->next;
        else head_ = removed->next;
        if (removed == tail_) tail_ = previous;
        --size_;
        delete removed;
    }

    void clear() noexcept {
        // Iterative deletion avoids recursive destruction of a long chain.
        while (head_) {
            Node* removed = head_;
            head_ = head_->next;
            delete removed;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    void swap(LinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }
};
