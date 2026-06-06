#pragma once

#include <queue>

namespace uthread {

class Mutex {
public:
    Mutex();
    ~Mutex() = default;

    void lock();
    bool tryLock();
    void unlock();
    bool isLocked() const;
    int getOwnerTid() const;
    int getMutexId() const;

private:
    static int next_mutex_id_;
    int mutex_id_;
    bool locked_;
    int owner_tid_;
    std::queue<int> wait_queue_;
};

} // namespace uthread
