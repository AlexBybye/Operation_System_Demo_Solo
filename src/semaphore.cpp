#include "semaphore.h"
#include "scheduler.h"

namespace uthread {

namespace {

int getWaitingTid() {
    Scheduler* sched = Scheduler::getInstance();
    if (!sched || !sched->getCurrentThread()) {
        return -1;
    }
    return sched->getCurrentThread()->getTid();
}

} // namespace

Semaphore::Semaphore(int initial_count)
    : count_(initial_count < 0 ? 0 : initial_count) {}

void Semaphore::wait() {
    int current_tid = getWaitingTid();
    if (current_tid == -1) {
        return;
    }

    while (count_ == 0) {
        wait_queue_.push(current_tid);
        Scheduler::blockCurrent();
    }

    --count_;
}

void Semaphore::signal() {
    ++count_;

    if (!wait_queue_.empty()) {
        int next_tid = wait_queue_.front();
        wait_queue_.pop();
        Scheduler* sched = Scheduler::getInstance();
        if (sched) {
            sched->wakeThread(next_tid);
        }
    }
}

int Semaphore::getCount() const {
    return count_;
}

bool Semaphore::hasWaiters() const {
    return !wait_queue_.empty();
}

} // namespace uthread
