#include "mutex.h"
#include "scheduler.h"
#include "deadlock_detector.h"
#include <iostream>

namespace uthread {

int Mutex::next_mutex_id_ = 0;

namespace {

int getRunningTid() {
    Scheduler* sched = Scheduler::getInstance();
    if (!sched || !sched->getCurrentThread()) {
        return -1;
    }
    return sched->getCurrentThread()->getTid();
}

} // namespace

Mutex::Mutex()
    : mutex_id_(next_mutex_id_++),
      locked_(false),
      owner_tid_(-1) {}

void Mutex::lock() {
    int current_tid = getRunningTid();
    if (current_tid == -1 || owner_tid_ == current_tid) {
        return;
    }

    while (locked_) {
        auto& detector = DeadlockDetector::getInstance();
        detector.addWaitEdge(current_tid, mutex_id_);
        if (detector.detect()) {
            std::cout << detector.getLastCycleInfo() << "\n";
        }
        wait_queue_.push(current_tid);
        Scheduler::blockCurrent();
        detector.removeWaitEdge(current_tid, mutex_id_);
    }

    locked_ = true;
    owner_tid_ = current_tid;
    DeadlockDetector::getInstance().addHoldEdge(mutex_id_, current_tid);
}

bool Mutex::tryLock() {
    int current_tid = getRunningTid();
    if (current_tid == -1) {
        return false;
    }
    if (owner_tid_ == current_tid) {
        return true;
    }
    if (locked_) {
        return false;
    }

    locked_ = true;
    owner_tid_ = current_tid;
    DeadlockDetector::getInstance().addHoldEdge(mutex_id_, current_tid);
    return true;
}

void Mutex::unlock() {
    int current_tid = getRunningTid();
    if (!locked_ || owner_tid_ != current_tid) {
        return;
    }

    DeadlockDetector::getInstance().removeHoldEdge(mutex_id_, current_tid);
    locked_ = false;
    owner_tid_ = -1;

    if (!wait_queue_.empty()) {
        int next_tid = wait_queue_.front();
        wait_queue_.pop();
        Scheduler* sched = Scheduler::getInstance();
        if (sched) {
            sched->wakeThread(next_tid);
        }
    }
}

bool Mutex::isLocked() const {
    return locked_;
}

int Mutex::getOwnerTid() const {
    return owner_tid_;
}

int Mutex::getMutexId() const {
    return mutex_id_;
}

} // namespace uthread
