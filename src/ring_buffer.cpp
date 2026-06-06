#include "ring_buffer.h"
#include "scheduler.h"
#include <iostream>

namespace uthread {

RingBuffer::RingBuffer(int capacity)
    : buffer_(capacity, 0),
      capacity_(capacity),
      head_(0),
      tail_(0),
      count_(0) {}

void RingBuffer::produce(int item) {
    mutex_.lock();
    while (count_ == capacity_) {
        not_full_.wait(mutex_);
    }
    buffer_[tail_] = item;
    tail_ = (tail_ + 1) % capacity_;
    ++count_;
    int tid = -1;
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->getCurrentThread()) tid = sched->getCurrentThread()->getTid();
    std::cout << "  [生产 #" << tid << "] 写入 " << item
              << " (占用 " << count_ << "/" << capacity_ << ")\n";
    not_empty_.signal();
    mutex_.unlock();
}

int RingBuffer::consume() {
    mutex_.lock();
    while (count_ == 0) {
        not_empty_.wait(mutex_);
    }
    int item = buffer_[head_];
    head_ = (head_ + 1) % capacity_;
    --count_;
    int tid = -1;
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->getCurrentThread()) tid = sched->getCurrentThread()->getTid();
    std::cout << "  [消费 #" << tid << "] 读取 " << item
              << " (占用 " << count_ << "/" << capacity_ << ")\n";
    not_full_.signal();
    mutex_.unlock();
    return item;
}

int RingBuffer::size() const { return count_; }
int RingBuffer::capacity() const { return capacity_; }
bool RingBuffer::empty() const { return count_ == 0; }
bool RingBuffer::full() const { return count_ == capacity_; }

void RingBuffer::printState() const {
    std::cout << "  [RingBuffer] 容量:" << capacity_
              << " 当前:" << count_ << " head:" << head_ << " tail:" << tail_ << "\n";
}

} // namespace uthread
