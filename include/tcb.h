#pragma once

#define _XOPEN_SOURCE 600

/**
 * @file tcb.h
 * @brief 线程控制块（Thread Control Block）定义
 *
 * TCB 是线程的核心数据结构，存储线程的全部元信息。
 * 包括线程标识、状态、优先级、上下文和栈空间。
 */

#include <ctime>
#include <string>
#include <ucontext.h>
#include <sys/types.h>

namespace uthread {

/// 线程状态枚举
enum class ThreadState {
    READY,      // 就绪：等待被调度
    RUNNING,    // 运行：正在占用 CPU
    BLOCKED,    // 阻塞：等待外部事件
    FINISHED    // 完成：线程函数已执行结束
};

/// 调度策略枚举
enum class SchedulePolicy {
    FIFO,       // 先进先出 (FCFS)
    SJF,        // 短作业优先
    RR,         // 时间片轮转
    PRIORITY    // 优先级调度
};

/// 默认栈大小：64KB
constexpr size_t DEFAULT_STACK_SIZE = 64 * 1024;

/// 默认时间片大小（RR 调度用）
constexpr int DEFAULT_QUANTUM = 2;

/// 线程入口函数类型
using ThreadFunc = void (*)();

/**
 * @class TCB
 * @brief 线程控制块
 *
 * 存储单个线程的所有信息，包括标识、状态、上下文等。
 * 通过智能指针管理生命周期。
 */
class TCB {
public:
    /**
     * @brief 构造函数
     * @param tid 线程ID
     * @param priority 优先级（1-10，数字越小优先级越高）
     * @param func 线程入口函数
     * @param stack_size 栈空间大小（字节）
     */
    TCB(int tid, int priority, ThreadFunc func, size_t stack_size = DEFAULT_STACK_SIZE);

    /// 析构函数（释放栈空间）
    ~TCB();

    // 禁止拷贝
    TCB(const TCB&) = delete;
    TCB& operator=(const TCB&) = delete;

    // ===== 状态管理 =====

    /**
     * @brief 获取当前状态
     */
    ThreadState getState() const;

    /**
     * @brief 设置线程状态
     * @param new_state 目标状态
     * @return true 如果状态转换合法并成功
     *
     * 合法转换：
     *   READY → RUNNING, RUNNING → READY,
     *   RUNNING → BLOCKED, BLOCKED → READY,
     *   RUNNING → FINISHED
     */
    bool setState(ThreadState new_state);

    // ===== 属性访问 =====

    int getTid() const;
    int getPriority() const;
    void setPriority(int new_priority);
    pid_t getPid() const;
    time_t getCreateTime() const;
    time_t getFirstRunTime() const;
    time_t getEndTime() const;
    bool isFinished() const;
    void markFinished();
    void recordFirstRun();
    void recordEnd();

    // ===== 逻辑时钟调度统计（用于 FCFS/SJF/RR/Priority 量化对比）=====

    /// 设置/获取预估 CPU 服务时间（逻辑时间片单位，供 SJF 选择与统计使用）
    void setBurstTime(int burst);
    int getBurstTime() const;

    /// 设置/获取到达时间（逻辑时钟，批处理模型下默认为 0）
    void setArrivalTick(int tick);
    int getArrivalTick() const;

    /// 首次运行 / 完成的逻辑时钟（-1 表示尚未发生）
    void setFirstRunTick(int tick);
    int getFirstRunTick() const;
    void setFinishTick(int tick);
    int getFinishTick() const;

    /**
     * @brief 获取状态的字符串表示
     */
    std::string getStateStr() const;

    /**
     * @brief 获取入口函数
     */
    ThreadFunc getEntryFunc() const;

    /**
     * @brief 打印线程信息
     */
    void printInfo() const;

    // ===== 上下文相关 =====

    /**
     * @brief 获取 ucontext 引用（调度器需要直接操作）
     */
    ucontext_t& getContext();

    /**
     * @brief 初始化线程上下文
     * @param main_ctx 主上下文指针（线程结束后返回此处）
     *
     * 调用 getcontext → 设置栈 → makecontext 绑定入口函数
     */
    void initContext(ucontext_t* main_ctx);

private:
    int tid_;                   // 线程ID
    int priority_;              // 优先级 (1-10)
    pid_t pid_;                 // 所属进程ID
    time_t create_time_;        // 创建时间
    time_t first_run_time_;     // 首次被调度运行的时间戳 (-1 表示未运行)
    time_t end_time_;           // 线程运行结束完成的时间戳 (-1 表示未结束)
    ThreadState state_;         // 当前状态
    bool finished_;             // 完成标志

    // 逻辑时钟调度统计字段（与墙钟时间无关，用于精确量化对比）
    int burst_time_;            // 预估 CPU 服务时间（逻辑片段数）
    int arrival_tick_;          // 到达时刻（逻辑时钟）
    int first_run_tick_;        // 首次运行时刻（逻辑时钟，-1 表示未运行）
    int finish_tick_;           // 完成时刻（逻辑时钟，-1 表示未完成）

    ThreadFunc entry_func_;     // 入口函数指针
    ucontext_t context_;        // 线程上下文
    char* stack_;               // 栈空间（malloc 分配）
    size_t stack_size_;         // 栈大小
};

} // namespace uthread
