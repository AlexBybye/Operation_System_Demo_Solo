#include "thread_pool.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace uthread {

ThreadPool::ThreadPool() : next_tid_(1) {}

int ThreadPool::createThread(int priority, ThreadFunc func) {
    auto tcb = std::make_shared<TCB>(next_tid_, priority, func);
    threads_.push_back(tcb);
    return next_tid_++;
}

bool ThreadPool::removeThread(int tid) {
    auto it = std::remove_if(threads_.begin(), threads_.end(),
                             [tid](const std::shared_ptr<TCB>& tcb) { return tcb->getTid() == tid; });
    if (it != threads_.end()) {
        threads_.erase(it, threads_.end());
        return true;
    }
    return false;
}

std::shared_ptr<TCB> ThreadPool::getThread(int tid) {
    auto it = std::find_if(threads_.begin(), threads_.end(),
                           [tid](const std::shared_ptr<TCB>& tcb) { return tcb->getTid() == tid; });
    if (it != threads_.end()) {
        return *it;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<TCB>>& ThreadPool::getAllThreads() const {
    return threads_;
}

std::vector<std::shared_ptr<TCB>> ThreadPool::getThreadsByState(ThreadState state) const {
    std::vector<std::shared_ptr<TCB>> result;
    for (const auto& tcb : threads_) {
        if (tcb->getState() == state) {
            result.push_back(tcb);
        }
    }
    return result;
}

size_t ThreadPool::size() const {
    return threads_.size();
}

void ThreadPool::printAll() const {
    std::cout << "  ID    状态    优先级    PID    创建时间\n";
    std::cout << "  ─────────────────────────────────────\n";
    if (threads_.empty()) {
        std::cout << "（线程池为空）\n";
        return;
    }
    for (const auto& tcb : threads_) {
        tcb->printInfo();
    }
}

void ThreadPool::clear() {
    threads_.clear();
}

} // namespace uthread
