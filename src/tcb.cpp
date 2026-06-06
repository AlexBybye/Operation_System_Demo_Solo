#include "tcb.h"
#include "context.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unistd.h>

namespace uthread {

TCB::TCB(int tid, int priority, ThreadFunc func, size_t stack_size)
    : tid_(tid),
      priority_(std::clamp(priority, 1, 10)),
      pid_(getpid()),
      create_time_(time(nullptr)),
      first_run_time_(-1),
      end_time_(-1),
      state_(ThreadState::READY),
      finished_(false),
      burst_time_(1),
      arrival_tick_(0),
      first_run_tick_(-1),
      finish_tick_(-1),
      entry_func_(func),
      stack_size_(stack_size) {
    stack_ = allocateStack(stack_size_);
}

TCB::~TCB() {
    if (stack_ != nullptr) {
        freeStack(stack_);
    }
}

ThreadState TCB::getState() const {
    return state_;
}

bool TCB::setState(ThreadState new_state) {
    switch (state_) {
        case ThreadState::READY:
            if (new_state == ThreadState::RUNNING) {
                state_ = new_state;
                return true;
            }
            break;
        case ThreadState::RUNNING:
            if (new_state == ThreadState::READY ||
                new_state == ThreadState::BLOCKED ||
                new_state == ThreadState::FINISHED) {
                state_ = new_state;
                return true;
            }
            break;
        case ThreadState::BLOCKED:
            if (new_state == ThreadState::READY) {
                state_ = new_state;
                return true;
            }
            break;
        case ThreadState::FINISHED:
            return false;
    }
    return false;
}

int TCB::getTid() const {
    return tid_;
}

int TCB::getPriority() const {
    return priority_;
}

void TCB::setPriority(int new_priority) {
    priority_ = std::clamp(new_priority, 1, 10);
}

pid_t TCB::getPid() const {
    return pid_;
}

time_t TCB::getCreateTime() const {
    return create_time_;
}

time_t TCB::getFirstRunTime() const {
    return first_run_time_;
}

time_t TCB::getEndTime() const {
    return end_time_;
}

bool TCB::isFinished() const {
    return finished_;
}

void TCB::markFinished() {
    if (setState(ThreadState::FINISHED)) {
        finished_ = true;
        end_time_ = time(nullptr);
    }
}

void TCB::recordFirstRun() {
    if (first_run_time_ == -1) {
        first_run_time_ = time(nullptr);
    }
}

void TCB::recordEnd() {
    end_time_ = time(nullptr);
}

void TCB::setBurstTime(int burst) {
    burst_time_ = burst > 0 ? burst : 1;
}

int TCB::getBurstTime() const {
    return burst_time_;
}

void TCB::setArrivalTick(int tick) {
    arrival_tick_ = tick;
}

int TCB::getArrivalTick() const {
    return arrival_tick_;
}

void TCB::setFirstRunTick(int tick) {
    first_run_tick_ = tick;
}

int TCB::getFirstRunTick() const {
    return first_run_tick_;
}

void TCB::setFinishTick(int tick) {
    finish_tick_ = tick;
}

int TCB::getFinishTick() const {
    return finish_tick_;
}

std::string TCB::getStateStr() const {
    switch (state_) {
        case ThreadState::READY: return "就绪";
        case ThreadState::RUNNING: return "运行";
        case ThreadState::BLOCKED: return "阻塞";
        case ThreadState::FINISHED: return "完成";
        default: return "未知";
    }
}

ThreadFunc TCB::getEntryFunc() const {
    return entry_func_;
}

void TCB::printInfo() const {
    char time_buf[64];
    struct tm* tm_info = localtime(&create_time_);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    std::cout << "[线程 #" << tid_ << "] "
              << "状态:" << getStateStr() << " | "
              << "优先级:" << priority_ << " | "
              << "PID:" << pid_ << " | "
              << "创建时间:" << time_buf << std::endl;
}

ucontext_t& TCB::getContext() {
    return context_;
}

void TCB::initContext(ucontext_t* main_ctx) {
    extern void threadWrapper();
    uthread::initContext(context_, stack_, stack_size_, main_ctx, threadWrapper);
}

} // namespace uthread
