#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "Queue/Queue.hpp"
#include "Stack/Stack.hpp"
#include "Sequence/LinkedList/LinkedList.hpp"
#include "Sequence/ListSequence/ListSequence.hpp"

namespace {
TEST(LinkedList, EmptyBoundsAndAppend) {
    LinkedList<int> list;
    EXPECT_TRUE(list.empty()); EXPECT_EQ(list.size(), 0u);
    EXPECT_THROW(list.get(0), std::out_of_range);
    EXPECT_THROW(list.remove_at(0), std::out_of_range);
    list.append(10); list.append(20); list.append(30);
    EXPECT_FALSE(list.empty()); ASSERT_EQ(list.size(), 3u);
    list.get(1) = 99;
    const auto& view = list;
    EXPECT_EQ(view.get(0), 10); EXPECT_EQ(view.get(1), 99); EXPECT_EQ(view.get(2), 30);
    EXPECT_THROW(view.get(3), std::out_of_range);
}

TEST(LinkedList, RemovesHeadMiddleTailAndReusesEmptyList) {
    LinkedList<int> list;
    for (int value : {1, 2, 3, 4}) list.append(value);
    list.remove_at(1); EXPECT_EQ(list.get(1), 3);
    list.remove_at(2); list.append(5); EXPECT_EQ(list.get(2), 5);
    list.remove_at(0); EXPECT_EQ(list.get(0), 3);
    list.remove_at(0); list.remove_at(0);
    EXPECT_TRUE(list.empty()); EXPECT_EQ(list.size(), 0u);
    list.append(7); EXPECT_EQ(list.get(0), 7);
    list.clear(); list.clear(); list.append(8); EXPECT_EQ(list.get(0), 8);
}

TEST(LinkedList, CopiesOwnTheirNodesAndMovesLeaveReusableSources) {
    LinkedList<std::string> source; source.append("a"); source.append("b");
    auto copy = source; copy.get(0) = "changed"; copy.remove_at(1);
    EXPECT_EQ(source.get(0), "a"); EXPECT_EQ(source.size(), 2u);
    LinkedList<std::string> assigned; assigned.append("old"); assigned = source;
    const auto& same = assigned; assigned = same;
    EXPECT_EQ(assigned.get(1), "b");
    LinkedList<std::string> moved(std::move(copy));
    EXPECT_TRUE(copy.empty()); EXPECT_EQ(moved.get(0), "changed"); copy.append("again");
    assigned = std::move(moved); EXPECT_TRUE(moved.empty()); moved.append("reused");
    auto& alias = assigned; assigned = std::move(alias);
    EXPECT_EQ(assigned.get(0), "changed");
    assigned.swap(source); EXPECT_EQ(assigned.size(), 2u); EXPECT_EQ(source.get(0), "changed");
}

struct CopyOnly {
    explicit CopyOnly(int number) : value(number) {}
    CopyOnly(const CopyOnly&) = default;
    CopyOnly& operator=(const CopyOnly&) = delete;
    int value;
};

TEST(LinkedList, PayloadNeedsNeitherDefaultConstructionNorAssignment) {
    LinkedList<CopyOnly> list; list.append(CopyOnly(5));
    auto copy = list; copy.append(copy.get(0));
    copy.remove_at(0); EXPECT_EQ(copy.get(0).value, 5); EXPECT_EQ(list.size(), 1u);
}

TEST(LinkedList, ClearsLongChainsWithoutRecursiveDestruction) {
    LinkedList<int> list;
    for (int i = 0; i < 30000; ++i) list.append(i);
    ASSERT_EQ(list.size(), 30000u);
    list.clear(); EXPECT_TRUE(list.empty()); list.append(1); EXPECT_EQ(list.get(0), 1);
}

struct Counted {
    static inline int alive = 0;
    static inline int copy_budget = -1;
    int value;
    explicit Counted(int number) : value(number) { ++alive; }
    Counted(const Counted& other) : value(other.value) {
        if (copy_budget == 0) throw std::runtime_error("injected copy failure");
        if (copy_budget > 0) --copy_budget;
        ++alive;
    }
    Counted& operator=(const Counted&) = delete;
    ~Counted() { --alive; }
};

class LinkedOwnership : public ::testing::Test {
    void TearDown() override { Counted::copy_budget = -1; EXPECT_EQ(Counted::alive, 0); }
};

TEST_F(LinkedOwnership, FailedCopyConstructionCleansAlreadyCopiedNodes) {
    LinkedList<Counted> source;
    source.append(Counted(1)); source.append(Counted(2)); source.append(Counted(3));
    const int before = Counted::alive;
    Counted::copy_budget = 1;
    EXPECT_THROW({ LinkedList<Counted> copy(source); }, std::runtime_error);
    Counted::copy_budget = -1;
    EXPECT_EQ(Counted::alive, before); EXPECT_EQ(source.size(), 3u);
}

TEST_F(LinkedOwnership, FailedAssignmentPreservesDestinationAndAppendPreservesTail) {
    LinkedList<Counted> source; source.append(Counted(1)); source.append(Counted(2));
    LinkedList<Counted> target; target.append(Counted(9));
    Counted::copy_budget = 1;
    EXPECT_THROW(target = source, std::runtime_error); Counted::copy_budget = -1;
    EXPECT_EQ(target.size(), 1u); EXPECT_EQ(target.get(0).value, 9);
    Counted value(7); Counted::copy_budget = 0;
    EXPECT_THROW(target.append(value), std::runtime_error); Counted::copy_budget = -1;
    EXPECT_EQ(target.size(), 1u); target.append(value); EXPECT_EQ(target.get(1).value, 7);
}

TEST(ListSequence, ImplementsSequenceInterfaceAndOwnsCopy) {
    ListSequence<int> sequence;
    Sequence<int>& interface = sequence;
    interface.append(1); interface.append(2); interface.append(3);
    interface.remove_at(1); EXPECT_EQ(interface.size(), 2u); EXPECT_EQ(interface.get(1), 3);
    const Sequence<int>& view = sequence; EXPECT_EQ(view.get(0), 1);
    auto copy = sequence; copy.get(0) = 8; EXPECT_EQ(sequence.get(0), 1);
    ListSequence<int> assigned; assigned = sequence; assigned.remove_at(0);
    EXPECT_EQ(sequence.size(), 2u); EXPECT_EQ(assigned.get(0), 3);
    ListSequence<int> moved(std::move(copy)); EXPECT_TRUE(copy.empty());
    copy.append(9); assigned = std::move(moved); EXPECT_TRUE(moved.empty());
    assigned.swap(sequence); EXPECT_EQ(sequence.get(0), 8);
    interface.clear(); EXPECT_TRUE(sequence.empty());
}

TEST(Queue, PreservesFifoAndRejectsEmptyFrontAndDequeue) {
    Queue<std::string> queue;
    EXPECT_TRUE(queue.empty()); EXPECT_THROW(queue.front(), std::out_of_range);
    EXPECT_THROW(queue.dequeue(), std::out_of_range);
    queue.enqueue("first"); queue.enqueue("second"); queue.enqueue("third");
    EXPECT_EQ(queue.size(), 3u); EXPECT_EQ(queue.front(), "first");
    queue.dequeue(); queue.enqueue("fourth"); EXPECT_EQ(queue.front(), "second");
    queue.front() = "changed"; const auto& view = queue; EXPECT_EQ(view.front(), "changed");
    queue.dequeue(); EXPECT_EQ(queue.front(), "third"); queue.dequeue(); EXPECT_EQ(queue.front(), "fourth");
    queue.dequeue(); EXPECT_TRUE(queue.empty()); queue.enqueue("again"); EXPECT_EQ(queue.front(), "again");
    queue.clear(); EXPECT_TRUE(queue.empty());
}

TEST(Queue, CopiesAndMovesKeepIndependentFifoState) {
    Queue<int> source; source.enqueue(1); source.enqueue(2);
    auto copy = source; copy.dequeue(); EXPECT_EQ(source.front(), 1); EXPECT_EQ(copy.front(), 2);
    Queue<int> assigned; assigned = source; assigned.front() = 9; EXPECT_EQ(source.front(), 1);
    Queue<int> moved(std::move(copy)); EXPECT_TRUE(copy.empty()); EXPECT_EQ(moved.front(), 2);
    assigned = std::move(moved); EXPECT_TRUE(moved.empty()); moved.enqueue(5);
    assigned.swap(source); EXPECT_EQ(assigned.front(), 1); EXPECT_EQ(source.front(), 2);
}

TEST_F(LinkedOwnership, FailedEnqueuePreservesQueueAndDequeueDoesNotCopyPayload) {
    Queue<Counted> queue; queue.enqueue(Counted(1)); Counted value(2);
    Counted::copy_budget = 0;
    EXPECT_THROW(queue.enqueue(value), std::runtime_error);
    EXPECT_EQ(queue.size(), 1u); EXPECT_EQ(queue.front().value, 1);
    EXPECT_NO_THROW(queue.dequeue()); EXPECT_TRUE(queue.empty()); Counted::copy_budget = -1;
}
TEST(LinkedList, PrependMaintainsHeadTailAndSupportsAliasing) {
    LinkedList<int> list;
    list.prepend(2); list.append(3); list.prepend(1);
    EXPECT_EQ(list.size(), 3u); EXPECT_EQ(list.get(0), 1); EXPECT_EQ(list.get(2), 3);
    list.prepend(list.get(2)); EXPECT_EQ(list.get(0), 3); EXPECT_EQ(list.size(), 4u);
    while (!list.empty()) list.remove_at(0);
    list.prepend(4); list.append(5); EXPECT_EQ(list.get(1), 5);
}

TEST(ListSequence, PrependDelegatesWithoutChangingSequenceInterface) {
    ListSequence<int> sequence;
    sequence.prepend(2); sequence.prepend(1); sequence.append(3);
    Sequence<int>& view = sequence;
    EXPECT_EQ(view.get(0), 1); EXPECT_EQ(view.get(2), 3);
    view.remove_at(0); EXPECT_EQ(view.get(0), 2);
}

TEST(Stack, PreservesLifoAndRejectsEmptyAccess) {
    Stack<int> stack;
    EXPECT_TRUE(stack.empty()); EXPECT_THROW(stack.top(), std::out_of_range);
    EXPECT_THROW(stack.pop(), std::out_of_range);
    stack.push(1); stack.push(2); stack.push(3);
    EXPECT_EQ(stack.size(), 3u); EXPECT_EQ(stack.top(), 3);
    stack.pop(); EXPECT_EQ(stack.top(), 2); stack.top() = 8;
    const auto& view = stack; EXPECT_EQ(view.top(), 8);
    stack.pop(); EXPECT_EQ(stack.top(), 1); stack.pop(); EXPECT_TRUE(stack.empty());
    stack.push(9); stack.clear(); stack.push(10); EXPECT_EQ(stack.top(), 10);
}

TEST(Stack, CopiesMovesAndNonDefaultPayloadsWorkIndependently) {
    Stack<CopyOnly> source; source.push(CopyOnly(1)); source.push(CopyOnly(2));
    auto copy = source; copy.pop(); EXPECT_EQ(source.top().value, 2); EXPECT_EQ(copy.top().value, 1);
    Stack<CopyOnly> assigned; assigned = source;
    Stack<CopyOnly> moved(std::move(copy)); EXPECT_TRUE(copy.empty()); copy.push(CopyOnly(3));
    assigned = std::move(moved); EXPECT_TRUE(moved.empty()); EXPECT_EQ(assigned.top().value, 1);
    assigned.swap(source); EXPECT_EQ(assigned.top().value, 2); EXPECT_EQ(source.top().value, 1);
}

TEST_F(LinkedOwnership, FailedStackPushPreservesHeadAndTailAndPopDoesNotCopy) {
    LinkedList<Counted> list; list.append(Counted(7)); Counted value(2);
    Counted::copy_budget = 0;
    EXPECT_THROW(list.prepend(value), std::runtime_error); Counted::copy_budget = -1;
    EXPECT_EQ(list.size(), 1u); EXPECT_EQ(list.get(0).value, 7);
    list.append(Counted(8)); EXPECT_EQ(list.get(1).value, 8);
    Stack<Counted> stack; stack.push(Counted(1));
    Counted::copy_budget = 0;
    EXPECT_THROW(stack.push(value), std::runtime_error);
    EXPECT_EQ(stack.size(), 1u); EXPECT_EQ(stack.top().value, 1);
    EXPECT_NO_THROW(stack.pop()); EXPECT_TRUE(stack.empty()); Counted::copy_budget = -1;
}
}  // namespace
