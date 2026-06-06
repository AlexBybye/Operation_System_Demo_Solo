#pragma once

#include "mutex.h"
#include "condition_variable.h"
#include "semaphore.h"
#include <vector>

namespace uthread {

class RingBuffer {
public:
    explicit RingBuffer(int capacity);
    ~RingBuffer() = default;

    // 阻塞式生产
    void produce(int item);
    // 阻塞式消费
    int consume();

    int size() const;
    int capacity() const;
    bool empty() const;
    bool full() const;
    void printState() const;

private:
    std::vector<int> buffer_;
    int capacity_;
    int head_;
    int tail_;
    int count_;

    Mutex mutex_;
    ConditionVariable not_full_;
    ConditionVariable not_empty_;
};

} // namespace uthread
