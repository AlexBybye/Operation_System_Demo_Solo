#include "uthread.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace uthread;

int pass_count = 0;
int fail_count = 0;
std::vector<int> g_trace;
std::unique_ptr<Mutex> g_mutex;
std::unique_ptr<Semaphore> g_semaphore;
std::unique_ptr<ConditionVariable> g_condition_variable;
std::unique_ptr<ConditionVariable> g_not_empty_cv;
std::unique_ptr<ConditionVariable> g_not_full_cv;
bool g_condition_ready = false;
bool g_condition_lock_reacquired = false;
std::vector<int> g_buffer;
std::vector<int> g_consumed_items;

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

void mutexWorker() {
    g_mutex->lock();
    g_trace.push_back(getCurrentTid());
    Scheduler::yield();
    g_trace.push_back(getCurrentTid());
    g_mutex->unlock();
}

void mutexTryLockWorker() {
    if (g_mutex->tryLock()) {
        g_trace.push_back(getCurrentTid());
        g_mutex->unlock();
        return;
    }
    g_trace.push_back(-1);
}

void semaphoreWorker() {
    g_semaphore->wait();
    g_trace.push_back(getCurrentTid());
    Scheduler::yield();
    g_trace.push_back(getCurrentTid());
    g_semaphore->signal();
}

void waitingWorker() {
    g_semaphore->wait();
    g_trace.push_back(getCurrentTid());
}

void signalingWorker() {
    g_trace.push_back(getCurrentTid());
    g_semaphore->signal();
}

void conditionWaitWorker() {
    g_mutex->lock();
    while (!g_condition_ready) {
        g_condition_variable->wait(*g_mutex);
    }
    g_condition_lock_reacquired = (g_mutex->getOwnerTid() == getCurrentTid());
    g_trace.push_back(getCurrentTid());
    g_mutex->unlock();
}

void conditionSignalWorker() {
    g_mutex->lock();
    g_trace.push_back(getCurrentTid());
    g_condition_ready = true;
    g_condition_variable->signal();
    g_mutex->unlock();
}

void conditionBroadcastWorker() {
    g_mutex->lock();
    g_trace.push_back(getCurrentTid());
    g_condition_ready = true;
    g_condition_variable->broadcast();
    g_mutex->unlock();
}

void producerWorker() {
    for (int item = 1; item <= 2; ++item) {
        g_mutex->lock();
        while (!g_buffer.empty()) {
            g_not_full_cv->wait(*g_mutex);
        }
        g_buffer.push_back(item);
        g_trace.push_back(100 + item);
        g_not_empty_cv->signal();
        g_mutex->unlock();
        Scheduler::yield();
    }
}

void consumerWorker() {
    for (int i = 0; i < 2; ++i) {
        g_mutex->lock();
        while (g_buffer.empty()) {
            g_not_empty_cv->wait(*g_mutex);
        }
        int item = g_buffer.front();
        g_buffer.erase(g_buffer.begin());
        g_consumed_items.push_back(item);
        g_trace.push_back(200 + item);
        g_not_full_cv->signal();
        g_mutex->unlock();
        Scheduler::yield();
    }
}

void test_mutex_serializes_critical_section() {
    resetTrace();
    g_mutex = std::make_unique<Mutex>();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);

    runtime.createThread(5, mutexWorker);
    runtime.createThread(5, mutexWorker);
    runtime.run();

    ASSERT(traceEquals({1, 1, 2, 2}), "Mutex 可串行化临界区");
    ASSERT(!g_mutex->isLocked(), "全部线程结束后 Mutex 已释放");
}

void test_mutex_try_lock() {
    resetTrace();
    g_mutex = std::make_unique<Mutex>();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);

    runtime.createThread(5, mutexWorker);
    runtime.createThread(5, mutexTryLockWorker);
    runtime.run();

    ASSERT(traceEquals({1, -1, 1}), "tryLock 在锁被占用时立即失败");
}

void test_semaphore_limits_concurrency() {
    resetTrace();
    g_semaphore = std::make_unique<Semaphore>(1);

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);

    runtime.createThread(5, semaphoreWorker);
    runtime.createThread(5, semaphoreWorker);
    runtime.createThread(5, semaphoreWorker);
    runtime.run();

    ASSERT(traceEquals({1, 1, 2, 2, 3, 3}), "Semaphore 可限制同一时刻仅一个线程进入");
    ASSERT(g_semaphore->getCount() == 1, "所有线程结束后 Semaphore 计数恢复");
}

void test_semaphore_signal_wakes_waiter() {
    resetTrace();
    g_semaphore = std::make_unique<Semaphore>(0);

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::FIFO);

    runtime.createThread(5, waitingWorker);
    runtime.createThread(5, signalingWorker);
    runtime.run();

    ASSERT(traceEquals({2, 1}), "signal 可唤醒等待中的线程");
    ASSERT(g_semaphore->getCount() == 0, "被唤醒线程会消费 signal 提供的资源");
}

void test_condition_variable_signal() {
    resetTrace();
    g_mutex = std::make_unique<Mutex>();
    g_condition_variable = std::make_unique<ConditionVariable>();
    g_condition_ready = false;
    g_condition_lock_reacquired = false;

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::FIFO);

    runtime.createThread(5, conditionWaitWorker);
    runtime.createThread(5, conditionSignalWorker);
    runtime.run();

    ASSERT(traceEquals({2, 1}), "ConditionVariable::signal 可唤醒一个等待线程");
    ASSERT(g_condition_lock_reacquired, "wait 返回后线程会重新持有 Mutex");
    ASSERT(!g_condition_variable->hasWaiters(), "signal 后等待队列为空");
}

void test_condition_variable_broadcast() {
    resetTrace();
    g_mutex = std::make_unique<Mutex>();
    g_condition_variable = std::make_unique<ConditionVariable>();
    g_condition_ready = false;

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::FIFO);

    runtime.createThread(5, conditionWaitWorker);
    runtime.createThread(5, conditionWaitWorker);
    runtime.createThread(5, conditionBroadcastWorker);
    runtime.run();

    ASSERT(traceEquals({3, 1, 2}), "ConditionVariable::broadcast 可唤醒所有等待线程");
    ASSERT(!g_condition_variable->hasWaiters(), "broadcast 后等待队列为空");
}

void test_producer_consumer_with_condition_variable() {
    resetTrace();
    g_mutex = std::make_unique<Mutex>();
    g_not_empty_cv = std::make_unique<ConditionVariable>();
    g_not_full_cv = std::make_unique<ConditionVariable>();
    g_buffer.clear();
    g_consumed_items.clear();

    UThreadRuntime runtime;
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);

    runtime.createThread(5, producerWorker);
    runtime.createThread(5, consumerWorker);
    runtime.run();

    ASSERT(traceEquals({101, 201, 102, 202}), "ConditionVariable 可配合 Mutex 实现生产者消费者");
    ASSERT(g_consumed_items == std::vector<int>({1, 2}), "消费者按顺序取到全部生产数据");
    ASSERT(g_buffer.empty(), "生产者消费者结束后缓冲区为空");
}

int main() {
    test_mutex_serializes_critical_section();
    test_mutex_try_lock();
    test_semaphore_limits_concurrency();
    test_semaphore_signal_wakes_waiter();
    test_condition_variable_signal();
    test_condition_variable_broadcast();
    test_producer_consumer_with_condition_variable();
    std::cout << "共 " << (pass_count + fail_count)
              << " 个测试, 通过 " << pass_count
              << " 个, 失败 " << fail_count << " 个" << std::endl;
    return fail_count == 0 ? 0 : 1;
}
