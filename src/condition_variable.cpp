#include "condition_variable.h"
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

void ConditionVariable::wait(Mutex& mutex) {
    int current_tid = getWaitingTid();
    if (current_tid == -1 || mutex.getOwnerTid() != current_tid) {
        return;
    }

    wait_queue_.push(current_tid);
    mutex.unlock();
    Scheduler::blockCurrent();
    mutex.lock();
}

void ConditionVariable::signal() {
    if (wait_queue_.empty()) {
        return;
    }

    int next_tid = wait_queue_.front();
    wait_queue_.pop();
    Scheduler* sched = Scheduler::getInstance();
    if (sched) {
        sched->wakeThread(next_tid);
    }
}

void ConditionVariable::broadcast() {
    Scheduler* sched = Scheduler::getInstance();
    if (!sched) {
        return;
    }

    while (!wait_queue_.empty()) {
        int next_tid = wait_queue_.front();
        wait_queue_.pop();
        sched->wakeThread(next_tid);
    }
}

bool ConditionVariable::hasWaiters() const {
    return !wait_queue_.empty();
}

} // namespace uthread
