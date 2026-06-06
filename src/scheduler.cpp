#include "scheduler.h"
#include <iostream>

namespace uthread {

Scheduler* Scheduler::instance_ = nullptr;

void threadWrapper() {
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->current_) {
        auto current = sched->current_;
        current->getEntryFunc()();
        Scheduler::exitCurrent();
    }
}

Scheduler::Scheduler(ThreadPool& pool)
    : pool_(pool),
      policy_(SchedulePolicy::FIFO),
      quantum_(DEFAULT_QUANTUM),
      rr_index_(0),
      logical_clock_(0),
      current_(nullptr),
      last_switch_reason_(SwitchReason::NONE) {
    instance_ = this;
}

void Scheduler::setPolicy(SchedulePolicy policy) {
    policy_ = policy;
    rr_index_ = 0; // Reset RR index on policy change
}

SchedulePolicy Scheduler::getPolicy() const {
    return policy_;
}

std::string Scheduler::getPolicyStr() const {
    switch (policy_) {
        case SchedulePolicy::FIFO: return "FIFO";
        case SchedulePolicy::SJF: return "SJF";
        case SchedulePolicy::RR: return "RR";
        case SchedulePolicy::PRIORITY: return "Priority";
        default: return "Unknown";
    }
}

void Scheduler::setQuantum(int quantum) {
    if (quantum > 0) {
        quantum_ = quantum;
    }
}

std::shared_ptr<TCB> Scheduler::selectFIFO() {
    auto& threads = pool_.getAllThreads();
    for (auto& t : threads) {
        if (t->getState() == ThreadState::READY) {
            return t;
        }
    }
    return nullptr;
}

std::shared_ptr<TCB> Scheduler::selectRR() {
    auto& threads = pool_.getAllThreads();
    if (threads.empty()) return nullptr;
    
    size_t size = threads.size();
    for (size_t i = 0; i < size; ++i) {
        size_t idx = (rr_index_ + i) % size;
        if (threads[idx]->getState() == ThreadState::READY) {
            rr_index_ = (idx + 1) % size;
            return threads[idx];
        }
    }
    return nullptr;
}

std::shared_ptr<TCB> Scheduler::selectPriority() {
    auto& threads = pool_.getAllThreads();
    std::shared_ptr<TCB> best = nullptr;
    
    for (auto& t : threads) {
        if (t->getState() == ThreadState::READY) {
            if (!best) {
                best = t;
            } else if (t->getPriority() < best->getPriority()) {
                best = t;
            } else if (t->getPriority() == best->getPriority() && t->getTid() < best->getTid()) {
                best = t;
            }
        }
    }
    return best;
}

std::shared_ptr<TCB> Scheduler::selectSJF() {
    auto& threads = pool_.getAllThreads();
    std::shared_ptr<TCB> best = nullptr;

    for (auto& t : threads) {
        if (t->getState() == ThreadState::READY) {
            if (!best) {
                best = t;
            } else if (t->getBurstTime() < best->getBurstTime()) {
                best = t;
            } else if (t->getBurstTime() == best->getBurstTime() && t->getTid() < best->getTid()) {
                best = t;
            }
        }
    }
    return best;
}

std::shared_ptr<TCB> Scheduler::selectNext() {
    switch (policy_) {
        case SchedulePolicy::FIFO: return selectFIFO();
        case SchedulePolicy::SJF: return selectSJF();
        case SchedulePolicy::RR: return selectRR();
        case SchedulePolicy::PRIORITY: return selectPriority();
        default: return nullptr;
    }
}

bool Scheduler::step() {
    auto selected = selectNext();
    if (!selected) return false;

    if (policy_ == SchedulePolicy::RR) {
        runRoundRobinSlice(selected);
    } else {
        dispatchThread(selected);
    }

    current_ = nullptr;
    last_switch_reason_ = SwitchReason::NONE;
    return true;
}

void Scheduler::run() {
    std::cout << "▶ 开始调度 (算法: " << getPolicyStr() << ")\n";
    while (step()) {}
    std::cout << "■ 调度完成\n";
}

ucontext_t* Scheduler::getMainContext() {
    return &main_context_;
}

void Scheduler::yield() {
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->current_) {
        sched->last_switch_reason_ = SwitchReason::YIELD;
        sched->current_->setState(ThreadState::READY);
        switchContext(sched->current_->getContext(), sched->main_context_);
    }
}

void Scheduler::blockCurrent() {
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->current_) {
        sched->last_switch_reason_ = SwitchReason::BLOCK;
        sched->current_->setState(ThreadState::BLOCKED);
        switchContext(sched->current_->getContext(), sched->main_context_);
    }
}

void Scheduler::exitCurrent() {
    Scheduler* sched = Scheduler::getInstance();
    if (sched && sched->current_) {
        sched->last_switch_reason_ = SwitchReason::EXIT;
        sched->current_->markFinished();
        std::cout << "✅ 线程 #" << sched->current_->getTid() << " 执行完毕\n";
        switchContext(sched->current_->getContext(), sched->main_context_);
    }
}

Scheduler* Scheduler::getInstance() {
    return instance_;
}

std::shared_ptr<TCB> Scheduler::getCurrentThread() {
    return current_;
}

bool Scheduler::wakeThread(int tid) {
    auto target = pool_.getThread(tid);
    if (!target) {
        return false;
    }
    return target->setState(ThreadState::READY);
}

void Scheduler::adjustRRIndex(int removed_index) {
    auto& threads = pool_.getAllThreads();
    int size = static_cast<int>(threads.size());
    if (size == 0) {
        rr_index_ = 0;
        return;
    }
    if (rr_index_ > removed_index) {
        --rr_index_;
    }
    if (rr_index_ >= size) {
        rr_index_ = 0;
    }
}

int Scheduler::getLogicalClock() const {
    return logical_clock_;
}

void Scheduler::resetLogicalClock() {
    logical_clock_ = 0;
}

void Scheduler::dispatchThread(const std::shared_ptr<TCB>& selected) {
    std::cout << "📌 调度: 线程 #" << selected->getTid()
              << " (优先级:" << selected->getPriority()
              << ", 算法:" << getPolicyStr() << ")\n";

    selected->setState(ThreadState::RUNNING);
    selected->recordFirstRun();
    // 逻辑时钟：记录首次运行时刻，用于精确计算等待时间
    if (selected->getFirstRunTick() == -1) {
        selected->setFirstRunTick(logical_clock_);
    }
    current_ = selected;
    last_switch_reason_ = SwitchReason::NONE;

    switchContext(main_context_, selected->getContext());

    // 本次时间片消耗，逻辑时钟前进一格
    ++logical_clock_;

    if (selected->isFinished()) {
        selected->setState(ThreadState::FINISHED);
        selected->setFinishTick(logical_clock_);
    }
}

void Scheduler::runRoundRobinSlice(const std::shared_ptr<TCB>& selected) {
    int remaining_slice = quantum_;

    while (remaining_slice > 0) {
        dispatchThread(selected);

        if (selected->isFinished() || selected->getState() == ThreadState::BLOCKED) {
            return;
        }

        if (last_switch_reason_ != SwitchReason::YIELD) {
            return;
        }

        --remaining_slice;
        if (remaining_slice == 0) {
            return;
        }
    }
}

} // namespace uthread
