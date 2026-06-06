#include "uthread.h"

namespace uthread {

UThreadRuntime::UThreadRuntime()
    : pool_(),
      scheduler_(pool_) {}

int UThreadRuntime::createThread(int priority, ThreadFunc func) {
    int tid = pool_.createThread(priority, func);
    auto thread = pool_.getThread(tid);
    if (thread) {
        thread->initContext(scheduler_.getMainContext());
    }
    return tid;
}

bool UThreadRuntime::removeThread(int tid) {
    auto& threads = pool_.getAllThreads();
    for (int i = 0; i < static_cast<int>(threads.size()); ++i) {
        if (threads[i]->getTid() == tid) {
            pool_.removeThread(tid);
            scheduler_.adjustRRIndex(i);
            return true;
        }
    }
    return false;
}

bool UThreadRuntime::wakeThread(int tid) {
    return scheduler_.wakeThread(tid);
}

bool UThreadRuntime::joinThread(int tid) {
    auto target = pool_.getThread(tid);
    if (!target) {
        return false;
    }

    while (!target->isFinished()) {
        if (!scheduler_.step()) {
            return false;
        }
    }
    return true;
}

void UThreadRuntime::setPolicy(SchedulePolicy policy) {
    scheduler_.setPolicy(policy);
}

SchedulePolicy UThreadRuntime::getPolicy() const {
    return scheduler_.getPolicy();
}

void UThreadRuntime::setQuantum(int quantum) {
    scheduler_.setQuantum(quantum);
}

void UThreadRuntime::run() {
    scheduler_.run();
}

bool UThreadRuntime::step() {
    return scheduler_.step();
}

std::shared_ptr<TCB> UThreadRuntime::getThread(int tid) {
    return pool_.getThread(tid);
}

const std::vector<std::shared_ptr<TCB>>& UThreadRuntime::getAllThreads() const {
    return pool_.getAllThreads();
}

void UThreadRuntime::printAll() const {
    pool_.printAll();
}

void UThreadRuntime::clear() {
    pool_.clear();
}

ThreadPool& UThreadRuntime::getThreadPool() {
    return pool_;
}

Scheduler& UThreadRuntime::getScheduler() {
    return scheduler_;
}

} // namespace uthread
