#include "uthread.h"
#include <iostream>
#include <vector>

using namespace uthread;

int pass_count = 0;
int fail_count = 0;
std::vector<int> g_trace;

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "❌ FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            ++fail_count; \
        } else { \
            std::cout << "✅ PASS: " << msg << std::endl; \
            ++pass_count; \
        } \
    } while(0)

int getCurrentTid() {
    Scheduler* sched = Scheduler::getInstance();
    if (!sched || !sched->getCurrentThread()) {
        return -1;
    }
    return sched->getCurrentThread()->getTid();
}

void resetTrace() {
    g_trace.clear();
}

bool traceEquals(const std::vector<int>& expected) {
    return g_trace == expected;
}

void cooperativeTwice() {
    for (int i = 0; i < 2; ++i) {
        g_trace.push_back(getCurrentTid());
        Scheduler::yield();
    }
}

void blockOnceThenExit() {
    g_trace.push_back(getCurrentTid());
    Scheduler::blockCurrent();
    g_trace.push_back(getCurrentTid());
    Scheduler::exitCurrent();
}

void runOnce() {
    g_trace.push_back(getCurrentTid());
}

void exitImmediately() {
    g_trace.push_back(getCurrentTid());
    Scheduler::exitCurrent();
    g_trace.push_back(-1);
}

void test_fifo() {
    resetTrace();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::FIFO);

    runtime.createThread(5, cooperativeTwice);
    runtime.createThread(1, cooperativeTwice);
    runtime.createThread(3, cooperativeTwice);

    runtime.run();
    ASSERT(traceEquals({1, 1, 2, 2, 3, 3}), "FIFO 按创建顺序调度");
}

void test_priority() {
    resetTrace();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::PRIORITY);

    runtime.createThread(5, cooperativeTwice);
    runtime.createThread(1, cooperativeTwice);
    runtime.createThread(3, cooperativeTwice);

    runtime.run();
    ASSERT(traceEquals({2, 2, 3, 3, 1, 1}), "Priority 按优先级调度");
}

void test_rr_quantum_one() {
    resetTrace();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);

    runtime.createThread(5, cooperativeTwice);
    runtime.createThread(1, cooperativeTwice);
    runtime.createThread(3, cooperativeTwice);

    runtime.run();
    ASSERT(traceEquals({1, 2, 3, 1, 2, 3}), "RR 在 quantum=1 时轮转公平");
}

void test_rr_quantum_two() {
    resetTrace();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(2);

    runtime.createThread(5, cooperativeTwice);
    runtime.createThread(1, cooperativeTwice);
    runtime.createThread(3, cooperativeTwice);

    runtime.run();
    ASSERT(traceEquals({1, 1, 2, 2, 3, 3}), "RR 在 quantum=2 时允许连续运行两个片段");
}

void test_block_wake_and_join() {
    resetTrace();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::FIFO);

    int blocked_tid = runtime.createThread(5, blockOnceThenExit);
    runtime.createThread(5, runOnce);

    ASSERT(runtime.step(), "阻塞线程首次被调度");
    ASSERT(runtime.getThread(blocked_tid)->getState() == ThreadState::BLOCKED, "线程可主动阻塞自己");
    ASSERT(runtime.step(), "其余就绪线程仍可继续执行");
    ASSERT(runtime.wakeThread(blocked_tid), "阻塞线程可被显式唤醒");
    ASSERT(runtime.joinThread(blocked_tid), "joinThread 可等待目标线程完成");
    ASSERT(traceEquals({1, 2, 1}), "阻塞与唤醒后的执行顺序正确");
}

void test_exit_current() {
    resetTrace();

    UThreadRuntime runtime;
    int tid = runtime.createThread(5, exitImmediately);

    ASSERT(runtime.joinThread(tid), "exitCurrent 后线程可正常结束");
    ASSERT(traceEquals({1}), "exitCurrent 后不会继续执行后续代码");
}

void test_empty() {
    UThreadRuntime runtime;
    ASSERT(!runtime.step(), "空线程池单步调度立即返回 false");
}

int main() {
    test_fifo();
    test_priority();
    test_rr_quantum_one();
    test_rr_quantum_two();
    test_block_wake_and_join();
    test_exit_current();
    test_empty();
    std::cout << "共 " << (pass_count + fail_count)
              << " 个测试, 通过 " << pass_count
              << " 个, 失败 " << fail_count << " 个" << std::endl;
    return fail_count == 0 ? 0 : 1;
}
