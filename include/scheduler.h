#pragma once

/**
 * @file scheduler.h
 * @brief 线程调度器
 *
 * 实现三种调度算法：FIFO、RR、Priority。
 * 调度器在主上下文中运行，通过 swapcontext 切换到线程上下文。
 */

#include "thread_pool.h"
#include "context.h"
#include <functional>

namespace uthread {

void threadWrapper();

/**
 * @class Scheduler
 * @brief 线程调度器
 *
 * 管理调度策略的选择和执行。调度器维护主上下文，
 * 线程执行完毕或让步后都会返回到主上下文。
 */
class Scheduler {
public:
    /**
     * @brief 构造函数
     * @param pool 线程池引用
     */
    explicit Scheduler(ThreadPool& pool);
    ~Scheduler() = default;

    /**
     * @brief 设置调度策略
     * @param policy 调度算法类型
     */
    void setPolicy(SchedulePolicy policy);

    /**
     * @brief 获取当前调度策略
     */
    SchedulePolicy getPolicy() const;

    /**
     * @brief 获取调度策略的字符串名称
     */
    std::string getPolicyStr() const;

    /**
     * @brief 设置 RR 时间片大小
     * @param quantum 时间片（正整数）
     */
    void setQuantum(int quantum);

    /**
     * @brief 运行调度
     *
     * 持续调度直到所有线程都执行完毕或被阻塞。
     * 每一轮调度：选择线程 → 切换执行 → 更新状态。
     * 打印每一步的调度决策。
     */
    void run();

    /**
     * @brief 执行一步调度（选择并运行一个线程的一个时间片）
     * @return true 如果还有可调度的线程
     */
    bool step();

    /**
     * @brief 获取主上下文指针（供线程初始化时设置 uc_link）
     */
    ucontext_t* getMainContext();

    /**
     * @brief 当前运行线程主动让步
     *
     * 将当前线程状态设为 READY，切换回主上下文。
     * 供线程函数内部调用。
     */
    static void yield();

    /**
     * @brief 当前运行线程主动阻塞
     *
     * 将当前线程状态设为 BLOCKED，切换回主上下文。
     * 供线程函数内部模拟等待事件时调用。
     */
    static void blockCurrent();

    /**
     * @brief 当前运行线程主动退出
     *
     * 将当前线程标记为 FINISHED，切换回主上下文。
     * 供线程函数提前结束执行时调用。
     */
    static void exitCurrent();

    /**
     * @brief 获取全局调度器实例（单例，供 yield 等静态函数使用）
     */
    static Scheduler* getInstance();

    /**
     * @brief 获取当前正在运行的线程
     */
    std::shared_ptr<TCB> getCurrentThread();

    /**
     * @brief 唤醒一个阻塞线程
     * @param tid 目标线程ID
     * @return true 如果线程存在且唤醒成功
     */
    bool wakeThread(int tid);

    /**
     * @brief 线程移除后调整 RR 轮转索引
     * @param removed_index 被移除线程在数组中的位置
     */
    void adjustRRIndex(int removed_index);

    /**
     * @brief 获取当前逻辑时钟值（每个调度时间片 +1）
     *
     * 逻辑时钟与墙钟无关，用于精确量化 FCFS/SJF/RR/Priority 的
     * 等待时间与周转时间，避免 time_t 秒级精度不足的问题。
     */
    int getLogicalClock() const;

    /**
     * @brief 复位逻辑时钟（在每轮算法对比演示开始前调用）
     */
    void resetLogicalClock();

private:
    enum class SwitchReason {
        NONE,
        YIELD,
        BLOCK,
        EXIT
    };

    /**
     * @brief 根据 FIFO 策略选择下一个线程
     * @return 被选中的线程，无可用线程返回 nullptr
     */
    std::shared_ptr<TCB> selectFIFO();

    /**
     * @brief 根据 RR 策略选择下一个线程
     */
    std::shared_ptr<TCB> selectRR();

    /**
     * @brief 根据 SJF 策略选择下一个线程（选预估服务时间最短者）
     */
    std::shared_ptr<TCB> selectSJF();

    /**
     * @brief 根据 Priority 策略选择下一个线程
     */
    std::shared_ptr<TCB> selectPriority();

    /**
     * @brief 选择下一个要运行的线程（根据当前策略分发）
     */
    std::shared_ptr<TCB> selectNext();

    /**
     * @brief 执行一次上下文切换
     * @param selected 被调度的线程
     */
    void dispatchThread(const std::shared_ptr<TCB>& selected);

    /**
     * @brief 处理 RR 的协作式时间片
     * @param selected 被调度的线程
     */
    void runRoundRobinSlice(const std::shared_ptr<TCB>& selected);

    ThreadPool& pool_;                 // 线程池引用
    SchedulePolicy policy_;            // 当前调度策略
    int quantum_;                      // RR 协作式时间片片段数
    int rr_index_;                     // RR 轮转索引
    int logical_clock_;                // 逻辑时钟（每个时间片自增）
    ucontext_t main_context_;          // 主上下文
    std::shared_ptr<TCB> current_;     // 当前运行的线程
    SwitchReason last_switch_reason_;  // 最近一次切回主上下文的原因

    static Scheduler* instance_;       // 全局实例（供静态函数使用）

    friend void threadWrapper();
};

} // namespace uthread
