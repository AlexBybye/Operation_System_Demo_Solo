#pragma once

/**
 * @file uthread.h
 * @brief 用户级线程库 — 统一对外 API
 *
 * 使用方只需 #include "uthread.h" 即可访问全部功能。
 */

#include "tcb.h"
#include "thread_pool.h"
#include "scheduler.h"
#include "context.h"
#include "mutex.h"
#include "semaphore.h"
#include "condition_variable.h"
#include "deadlock_detector.h"
#include "ring_buffer.h"
#include "virtual_memory.h"
#include "partition_allocator.h"
#include "virtual_fs.h"

namespace uthread {

/**
 * @class UThreadRuntime
 * @brief 用户级线程运行时封装
 *
 * 封装线程池与调度器，提供更接近线程库的统一入口。
 * 创建线程时会自动初始化上下文，避免调用方遗漏步骤。
 */
class UThreadRuntime {
public:
    UThreadRuntime();
    ~UThreadRuntime() = default;

    /**
     * @brief 创建线程并加入运行时
     * @param priority 优先级（1-10）
     * @param func 线程入口函数
     * @return 新线程的 tid
     */
    int createThread(int priority, ThreadFunc func);

    /**
     * @brief 删除指定线程
     * @param tid 线程ID
     * @return true 如果删除成功
     */
    bool removeThread(int tid);

    /**
     * @brief 唤醒阻塞线程
     * @param tid 线程ID
     * @return true 如果唤醒成功
     */
    bool wakeThread(int tid);

    /**
     * @brief 等待指定线程执行结束
     * @param tid 线程ID
     * @return true 如果线程成功执行结束
     *
     * 当前实现为协作式 join：运行调度器直到目标线程完成。
     */
    bool joinThread(int tid);

    void setPolicy(SchedulePolicy policy);
    SchedulePolicy getPolicy() const;
    void setQuantum(int quantum);
    void run();
    bool step();

    std::shared_ptr<TCB> getThread(int tid);
    const std::vector<std::shared_ptr<TCB>>& getAllThreads() const;
    void printAll() const;
    void clear();

    ThreadPool& getThreadPool();
    Scheduler& getScheduler();

private:
    ThreadPool pool_;
    Scheduler scheduler_;
};

} // namespace uthread
