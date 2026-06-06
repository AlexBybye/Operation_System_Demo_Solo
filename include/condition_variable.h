#pragma once

/**
 * @file condition_variable.h
 * @brief 用户级线程条件变量
 *
 * 条件变量与互斥锁配合使用。线程在 wait 时会先释放互斥锁，
 * 再进入阻塞状态；被 signal 或 broadcast 唤醒后，会重新获取互斥锁。
 */

#include "mutex.h"
#include <queue>

namespace uthread {

/**
 * @class ConditionVariable
 * @brief 协作式条件变量
 */
class ConditionVariable {
public:
    ConditionVariable() = default;
    ~ConditionVariable() = default;

    /**
     * @brief 等待条件满足
     * @param mutex 与条件变量配套使用的互斥锁
     *
     * 当前线程会先加入等待队列，释放互斥锁，然后进入阻塞。
     * 被唤醒后会重新获取互斥锁。
     */
    void wait(Mutex& mutex);

    /**
     * @brief 唤醒一个等待线程
     */
    void signal();

    /**
     * @brief 唤醒所有等待线程
     */
    void broadcast();

    /**
     * @brief 判断是否存在等待线程
     */
    bool hasWaiters() const;

private:
    std::queue<int> wait_queue_;
};

} // namespace uthread
