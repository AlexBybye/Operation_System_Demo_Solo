#pragma once

/**
 * @file semaphore.h
 * @brief 用户级线程信号量
 *
 * 通过计数器控制并发访问数。资源不足时，当前线程会进入阻塞状态，
 * 待其他线程 signal 后再继续竞争资源。
 */

#include <queue>

namespace uthread {

/**
 * @class Semaphore
 * @brief 协作式计数信号量
 */
class Semaphore {
public:
    /**
     * @brief 构造函数
     * @param initial_count 初始资源数
     */
    explicit Semaphore(int initial_count = 0);
    ~Semaphore() = default;

    /**
     * @brief 申请一个资源
     *
     * 若当前无可用资源，则当前线程阻塞等待。
     */
    void wait();

    /**
     * @brief 释放一个资源
     *
     * 若有等待线程，会唤醒一个阻塞线程重新竞争资源。
     */
    void signal();

    /**
     * @brief 获取当前资源计数
     */
    int getCount() const;

    /**
     * @brief 判断是否存在等待线程
     */
    bool hasWaiters() const;

private:
    int count_;
    std::queue<int> wait_queue_;
};

} // namespace uthread
