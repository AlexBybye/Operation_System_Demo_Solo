#pragma once

/**
 * @file thread_pool.h
 * @brief 线程池管理器
 *
 * 负责管理所有线程的集合，提供增删查改操作。
 * 线程ID自动分配（自增），保证唯一性。
 */

#include "tcb.h"
#include <vector>
#include <memory>

namespace uthread {

/**
 * @class ThreadPool
 * @brief 线程池 — 管理所有 TCB 实例
 *
 * 提供线程的创建、删除、查询和列表展示功能。
 * 使用 shared_ptr 管理 TCB 生命周期。
 */
class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool() = default;

    /**
     * @brief 创建并添加一个新线程
     * @param priority 优先级（1-10）
     * @param func 线程入口函数
     * @return 新线程的 tid
     */
    int createThread(int priority, ThreadFunc func);

    /**
     * @brief 根据 tid 删除线程
     * @param tid 要删除的线程ID
     * @return true 如果删除成功
     */
    bool removeThread(int tid);

    /**
     * @brief 根据 tid 查找线程
     * @param tid 线程ID
     * @return 线程指针（shared_ptr），未找到返回 nullptr
     */
    std::shared_ptr<TCB> getThread(int tid);

    /**
     * @brief 获取所有线程列表
     */
    const std::vector<std::shared_ptr<TCB>>& getAllThreads() const;

    /**
     * @brief 获取所有处于指定状态的线程
     * @param state 目标状态
     * @return 符合条件的线程列表
     */
    std::vector<std::shared_ptr<TCB>> getThreadsByState(ThreadState state) const;

    /**
     * @brief 获取线程总数
     */
    size_t size() const;

    /**
     * @brief 打印所有线程的信息表格
     */
    void printAll() const;

    /**
     * @brief 清空所有线程
     */
    void clear();

private:
    std::vector<std::shared_ptr<TCB>> threads_;   // 线程集合
    int next_tid_;                                 // 下一个可用的线程ID
};

} // namespace uthread
