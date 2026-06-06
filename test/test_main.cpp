#include "uthread.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <algorithm>

using namespace uthread;

std::unique_ptr<Mutex> g_demo_mutex;
std::unique_ptr<Semaphore> g_demo_semaphore;
std::unique_ptr<ConditionVariable> g_demo_condition;
std::unique_ptr<ConditionVariable> g_demo_not_empty;
std::unique_ptr<ConditionVariable> g_demo_not_full;
bool g_demo_ready = false;
std::vector<int> g_demo_buffer;

// 死锁演示用全局锁
std::unique_ptr<Mutex> g_dl_mutex_a;
std::unique_ptr<Mutex> g_dl_mutex_b;
bool g_dl_ordered = false;

// 调度顺序记录（用于性能对比表，记录每个时间片的派发顺序）
std::vector<int> g_schedule_order;

// 生产者消费者环形缓冲区演示
std::unique_ptr<RingBuffer> g_ring_buffer;

// 读者-写者问题演示
std::unique_ptr<Mutex> g_rw_count_mutex;     // 保护读者计数
std::unique_ptr<Semaphore> g_rw_write_sem;   // 写者互斥信号量（计数为1）
int g_rw_read_count = 0;

// 哲学家进餐问题演示（5 把叉子，叉子为非递归互斥锁，按 ID 升序加锁避免死锁）
std::unique_ptr<Mutex> g_forks[5];

void printMenu() {
    std::cout << "\n======================================\n";
    std::cout << "      用户级线程库 - 集成测试\n";
    std::cout << "======================================\n";
    std::cout << "  1. 创建线程\n";
    std::cout << "  2. 查看所有线程\n";
    std::cout << "  3. 设置线程优先级\n";
    std::cout << "  4. 阻塞线程\n";
    std::cout << "  5. 唤醒线程\n";
    std::cout << "  6. 删除线程\n";
    std::cout << "  7. 选择调度算法 (FIFO/SJF/RR/Priority)\n";
    std::cout << "  8. 运行调度\n";
    std::cout << "  9. 自动化演示 (含性能对比表)\n";
    std::cout << " 10. 设置 RR 时间片\n";
    std::cout << " 11. 等待线程结束 (join)\n";
    std::cout << " 12. Mutex 演示\n";
    std::cout << " 13. Semaphore 演示\n";
    std::cout << " 14. ConditionVariable 演示\n";
    std::cout << " 15. 生产者消费者演示\n";
    std::cout << " 16. 死锁再现演示\n";
    std::cout << " 17. 死锁避免演示 (有序资源分配)\n";
    std::cout << " 18. 环形缓冲区高级演示\n";
    std::cout << " 19. 虚拟内存-页面置换 (FIFO vs LRU)\n";
    std::cout << " 20. 虚拟内存-动态分区 (FF vs BF)\n";
    std::cout << " 21. 读者-写者问题演示\n";
    std::cout << " 22. 哲学家进餐问题演示\n";
    std::cout << " 23. 虚拟文件系统演示\n";
    std::cout << "  0. 退出\n";
    std::cout << "======================================\n";
    std::cout << "请选择> ";
}

void demoThreadFunc() {
    Scheduler* sched = Scheduler::getInstance();
    auto self = sched ? sched->getCurrentThread() : nullptr;
    int tid = self ? self->getTid() : 0;
    // 运行片段数等于该线程的预估服务时间 burst_time
    int burst = self ? self->getBurstTime() : 3;
    for (int i = burst; i > 0; --i) {
        std::cout << "线程 #" << tid << " 正在执行 (剩余:" << i << ")\n";
        g_schedule_order.push_back(tid);  // 记录每个时间片的实际占用者
        Scheduler::yield();
    }
}

void dummyFunc() {
    Scheduler* sched = Scheduler::getInstance();
    int tid = 0;
    if (sched && sched->getCurrentThread()) {
        tid = sched->getCurrentThread()->getTid();
    }
    std::cout << "线程 #" << tid << " 执行了一次。\n";
}

void mutexDemoThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    g_demo_mutex->lock();
    std::cout << "线程 #" << tid << " 获取 Mutex，进入临界区。\n";
    Scheduler::yield();
    std::cout << "线程 #" << tid << " 离开临界区，释放 Mutex。\n";
    g_demo_mutex->unlock();
}

void semaphoreDemoThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    g_demo_semaphore->wait();
    std::cout << "线程 #" << tid << " 获取 Semaphore 资源。\n";
    Scheduler::yield();
    std::cout << "线程 #" << tid << " 释放 Semaphore 资源。\n";
    g_demo_semaphore->signal();
}

void conditionWaitDemoThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    g_demo_mutex->lock();
    while (!g_demo_ready) {
        std::cout << "线程 #" << tid << " 等待条件成立。\n";
        g_demo_condition->wait(*g_demo_mutex);
    }
    std::cout << "线程 #" << tid << " 被唤醒并重新持有 Mutex。\n";
    g_demo_mutex->unlock();
}

void conditionSignalDemoThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    g_demo_mutex->lock();
    std::cout << "线程 #" << tid << " 设置条件并发出 signal。\n";
    g_demo_ready = true;
    g_demo_condition->signal();
    g_demo_mutex->unlock();
}

void producerDemoThread() {
    for (int item = 1; item <= 2; ++item) {
        g_demo_mutex->lock();
        while (!g_demo_buffer.empty()) {
            g_demo_not_full->wait(*g_demo_mutex);
        }
        g_demo_buffer.push_back(item);
        std::cout << "生产者放入数据 " << item << "\n";
        g_demo_not_empty->signal();
        g_demo_mutex->unlock();
        Scheduler::yield();
    }
}

void consumerDemoThread() {
    for (int i = 0; i < 2; ++i) {
        g_demo_mutex->lock();
        while (g_demo_buffer.empty()) {
            g_demo_not_empty->wait(*g_demo_mutex);
        }
        int item = g_demo_buffer.front();
        g_demo_buffer.erase(g_demo_buffer.begin());
        std::cout << "消费者取出数据 " << item << "\n";
        g_demo_not_full->signal();
        g_demo_mutex->unlock();
        Scheduler::yield();
    }
}

// 死锁演示线程：A 先锁 mutex_a 再锁 mutex_b
void deadlockThreadA() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    Mutex* first = g_dl_mutex_a.get();
    Mutex* second = g_dl_mutex_b.get();
    if (g_dl_ordered && first->getMutexId() > second->getMutexId()) {
        std::swap(first, second);
    }
    std::cout << "线程 #" << tid << " 请求锁 #" << first->getMutexId() << "\n";
    first->lock();
    std::cout << "线程 #" << tid << " 获得锁 #" << first->getMutexId() << "\n";
    Scheduler::yield();
    std::cout << "线程 #" << tid << " 请求锁 #" << second->getMutexId() << "\n";
    second->lock();
    std::cout << "线程 #" << tid << " 获得锁 #" << second->getMutexId() << "\n";
    second->unlock();
    first->unlock();
}

// 死锁演示线程：B 先锁 mutex_b 再锁 mutex_a
void deadlockThreadB() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    Mutex* first = g_dl_mutex_b.get();
    Mutex* second = g_dl_mutex_a.get();
    if (g_dl_ordered && first->getMutexId() > second->getMutexId()) {
        std::swap(first, second);
    }
    std::cout << "线程 #" << tid << " 请求锁 #" << first->getMutexId() << "\n";
    first->lock();
    std::cout << "线程 #" << tid << " 获得锁 #" << first->getMutexId() << "\n";
    Scheduler::yield();
    std::cout << "线程 #" << tid << " 请求锁 #" << second->getMutexId() << "\n";
    second->lock();
    std::cout << "线程 #" << tid << " 获得锁 #" << second->getMutexId() << "\n";
    second->unlock();
    first->unlock();
}

// 环形缓冲区生产者
void ringProducerThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    for (int i = 1; i <= 4; ++i) {
        int item = tid * 100 + i;
        g_ring_buffer->produce(item);
        Scheduler::yield();
    }
}

// 环形缓冲区消费者
void ringConsumerThread() {
    for (int i = 0; i < 4; ++i) {
        g_ring_buffer->consume();
        Scheduler::yield();
    }
}

// ===== 读者-写者问题（读者优先）=====
void readerThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    for (int i = 0; i < 2; ++i) {
        // 进入区：第一个读者负责锁住写者
        g_rw_count_mutex->lock();
        ++g_rw_read_count;
        if (g_rw_read_count == 1) {
            g_rw_write_sem->wait();  // 首个读者阻塞写者
        }
        g_rw_count_mutex->unlock();

        std::cout << "  [读者 #" << tid << "] 正在读取 (当前读者数=" << g_rw_read_count << ")\n";
        Scheduler::yield();

        // 退出区：最后一个读者释放写者
        g_rw_count_mutex->lock();
        --g_rw_read_count;
        if (g_rw_read_count == 0) {
            g_rw_write_sem->signal();
        }
        g_rw_count_mutex->unlock();
        Scheduler::yield();
    }
}

void writerThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    for (int i = 0; i < 2; ++i) {
        g_rw_write_sem->wait();
        std::cout << "  [写者 #" << tid << "] >>> 独占写入中 <<<\n";
        Scheduler::yield();
        std::cout << "  [写者 #" << tid << "] 写入完成，释放资源\n";
        g_rw_write_sem->signal();
        Scheduler::yield();
    }
}

// ===== 哲学家进餐问题（有序加锁避免死锁）=====
void philosopherThread() {
    int tid = Scheduler::getInstance()->getCurrentThread()->getTid();
    int id = tid - 1;                 // 哲学家编号 0..4
    int left = id;                    // 左叉
    int right = (id + 1) % 5;         // 右叉
    // 破坏循环等待：总是先拿编号小的叉子
    int first = std::min(left, right);
    int second = std::max(left, right);

    std::cout << "  [哲学家 #" << id << "] 思考中...\n";
    Scheduler::yield();

    g_forks[first]->lock();
    std::cout << "  [哲学家 #" << id << "] 拿起叉子 " << first << "\n";
    Scheduler::yield();
    g_forks[second]->lock();
    std::cout << "  [哲学家 #" << id << "] 拿起叉子 " << second << "，开始进餐\n";
    Scheduler::yield();

    std::cout << "  [哲学家 #" << id << "] 用餐完毕，放下叉子\n";
    g_forks[second]->unlock();
    g_forks[first]->unlock();
}

void resetDemoState(UThreadRuntime& runtime) {
    runtime.clear();
    g_demo_ready = false;
    g_demo_buffer.clear();
    g_schedule_order.clear();
    g_rw_read_count = 0;
    DeadlockDetector::getInstance().clear();
}

void handleCreate(UThreadRuntime& runtime) {
    int prio;
    std::cout << "请输入线程优先级(1-10): ";
    std::cin >> prio;
    int tid = runtime.createThread(prio, dummyFunc);
    std::cout << "成功创建线程 #" << tid << "\n";
}

void handleSetPriority(UThreadRuntime& runtime) {
    int tid, prio;
    std::cout << "请输入线程ID: ";
    std::cin >> tid;
    std::cout << "请输入新优先级(1-10): ";
    std::cin >> prio;
    auto t = runtime.getThread(tid);
    if (t) {
        t->setPriority(prio);
        std::cout << "修改成功。\n";
    } else {
        std::cout << "未找到线程。\n";
    }
}

void handleBlock(UThreadRuntime& runtime) {
    int tid;
    std::cout << "请输入线程ID: ";
    std::cin >> tid;
    auto t = runtime.getThread(tid);
    if (t) {
        if (t->setState(ThreadState::BLOCKED)) {
            std::cout << "阻塞成功。\n";
        } else {
            std::cout << "无法直接阻塞；演示程序将 READY 线程临时挂起为 BLOCKED。\n";
            if (t->getState() == ThreadState::READY) {
                t->setState(ThreadState::RUNNING);
                t->setState(ThreadState::BLOCKED);
                std::cout << "已强制阻塞。\n";
            }
        }
    } else {
        std::cout << "未找到线程。\n";
    }
}

void handleWake(UThreadRuntime& runtime) {
    int tid;
    std::cout << "请输入线程ID: ";
    std::cin >> tid;
    if (runtime.wakeThread(tid)) {
        std::cout << "唤醒成功。\n";
    } else {
        std::cout << "唤醒失败。\n";
    }
}

void handleRemove(UThreadRuntime& runtime) {
    int tid;
    std::cout << "请输入线程ID: ";
    std::cin >> tid;
    if (runtime.removeThread(tid)) {
        std::cout << "删除成功。\n";
    } else {
        std::cout << "未找到线程。\n";
    }
}

void handleSelectPolicy(UThreadRuntime& runtime) {
    int choice;
    std::cout << "选择算法 (1: FIFO, 2: SJF, 3: RR, 4: Priority): ";
    std::cin >> choice;
    if (choice == 1) runtime.setPolicy(SchedulePolicy::FIFO);
    else if (choice == 2) runtime.setPolicy(SchedulePolicy::SJF);
    else if (choice == 3) runtime.setPolicy(SchedulePolicy::RR);
    else if (choice == 4) runtime.setPolicy(SchedulePolicy::PRIORITY);
    else std::cout << "无效选择。\n";
    std::cout << "当前策略: " << runtime.getScheduler().getPolicyStr() << "\n";
}

void handleSetQuantum(UThreadRuntime& runtime) {
    int quantum;
    std::cout << "请输入 RR 时间片片段数: ";
    std::cin >> quantum;
    runtime.setQuantum(quantum);
    std::cout << "RR 时间片已更新为 " << quantum << "\n";
}

void handleJoin(UThreadRuntime& runtime) {
    int tid;
    std::cout << "请输入线程ID: ";
    std::cin >> tid;
    if (runtime.joinThread(tid)) {
        std::cout << "线程 #" << tid << " 已执行结束。\n";
    } else {
        std::cout << "等待失败：线程不存在或当前无可调度线程。\n";
    }
}

void runMutexDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_demo_mutex = std::make_unique<Mutex>();
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, mutexDemoThread);
    runtime.createThread(5, mutexDemoThread);
    runtime.run();
}

void runSemaphoreDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_demo_semaphore = std::make_unique<Semaphore>(2);
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, semaphoreDemoThread);
    runtime.createThread(5, semaphoreDemoThread);
    runtime.createThread(5, semaphoreDemoThread);
    runtime.run();
}

void runConditionDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_demo_mutex = std::make_unique<Mutex>();
    g_demo_condition = std::make_unique<ConditionVariable>();
    runtime.setPolicy(SchedulePolicy::FIFO);
    runtime.createThread(5, conditionWaitDemoThread);
    runtime.createThread(5, conditionSignalDemoThread);
    runtime.run();
}

void runProducerConsumerDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_demo_mutex = std::make_unique<Mutex>();
    g_demo_not_empty = std::make_unique<ConditionVariable>();
    g_demo_not_full = std::make_unique<ConditionVariable>();
    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, producerDemoThread);
    runtime.createThread(5, consumerDemoThread);
    runtime.run();
}

void runDeadlockDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_dl_mutex_a = std::make_unique<Mutex>();
    g_dl_mutex_b = std::make_unique<Mutex>();
    g_dl_ordered = false;

    std::cout << "\n=== 死锁再现演示 ===\n";
    std::cout << "线程A: 锁A -> 锁B  |  线程B: 锁B -> 锁A\n";
    std::cout << "由于循环等待条件成立，死锁检测器会捕获到环路。\n\n";

    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, deadlockThreadA);
    runtime.createThread(5, deadlockThreadB);

    // 死锁会导致调度无法推进，限制最大步数避免卡死
    int max_steps = 30;
    while (max_steps-- > 0) {
        if (!runtime.step()) break;
    }

    auto& detector = DeadlockDetector::getInstance();
    if (detector.detect()) {
        std::cout << "\n" << detector.getLastCycleInfo() << "\n";
        std::cout << "⚠️  系统检测到死锁，演示结束。\n";
    } else {
        std::cout << "\n本次未观测到死锁。\n";
    }
}

void runDeadlockAvoidDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_dl_mutex_a = std::make_unique<Mutex>();
    g_dl_mutex_b = std::make_unique<Mutex>();
    g_dl_ordered = true;

    std::cout << "\n=== 死锁避免演示 (有序资源分配) ===\n";
    std::cout << "强制所有线程按 Mutex ID 升序加锁，破坏循环等待条件。\n\n";

    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, deadlockThreadA);
    runtime.createThread(5, deadlockThreadB);
    runtime.run();
    std::cout << "\n✅ 系统平滑运行结束，未发生死锁。\n";
}

void runRingBufferDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_ring_buffer = std::make_unique<RingBuffer>(3);

    std::cout << "\n=== 环形缓冲区高级演示 (容量=3) ===\n";
    std::cout << "2 个生产者 + 2 个消费者协作运作。\n\n";

    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, ringProducerThread);
    runtime.createThread(5, ringProducerThread);
    runtime.createThread(5, ringConsumerThread);
    runtime.createThread(5, ringConsumerThread);
    runtime.run();
}

void runVirtualMemoryDemo() {
    std::cout << "\n=== 虚拟内存-页面置换演示 ===\n";
    std::cout << "物理帧数: 3, 页面访问序列: 1 2 3 4 1 2 5 1 2 3 4 5\n\n";

    std::vector<int> sequence = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};

    std::cout << "--- FIFO 算法 ---\n";
    VirtualMemory vm_fifo(3, 10, PageReplaceAlgo::FIFO);
    vm_fifo.runDemo(sequence);

    std::cout << "\n--- LRU 算法 ---\n";
    VirtualMemory vm_lru(3, 10, PageReplaceAlgo::LRU);
    vm_lru.runDemo(sequence);

    std::cout << "\n┌──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ 算法     │ 总访问   │ 缺页次数 │ 缺页率   │\n";
    std::cout << "├──────────┼──────────┼──────────┼──────────┤\n";
    std::cout << "│ " << std::left << std::setw(8) << "FIFO" << " │ "
              << std::setw(8) << vm_fifo.getTotalAccesses() << " │ "
              << std::setw(8) << vm_fifo.getPageFaults() << " │ "
              << std::setw(7) << std::fixed << std::setprecision(1) << vm_fifo.getPageFaultRate() << "% │\n";
    std::cout << "│ " << std::left << std::setw(8) << "LRU" << " │ "
              << std::setw(8) << vm_lru.getTotalAccesses() << " │ "
              << std::setw(8) << vm_lru.getPageFaults() << " │ "
              << std::setw(7) << std::fixed << std::setprecision(1) << vm_lru.getPageFaultRate() << "% │\n";
    std::cout << "└──────────┴──────────┴──────────┴──────────┘\n";
}

void runVirtualFSDemo() {
    std::cout << "\n=== 虚拟文件系统演示 ===\n";
    VirtualFS fs;
    fs.my_create("hello.txt");
    fs.my_create("notes.md");
    fs.my_write("hello.txt", "Hello, UThread Virtual FS!");
    fs.my_write("notes.md", "This is a notes file. It contains multiple bytes of text data.");
    fs.listFiles();
    fs.printDiskUsage();

    std::cout << "\n--- 读取 hello.txt ---\n";
    std::string content = fs.my_read("hello.txt");
    std::cout << "  内容: \"" << content << "\"\n";

    std::cout << "\n--- 删除 notes.md ---\n";
    fs.my_delete("notes.md");
    fs.listFiles();
    fs.printDiskUsage();
}

// 动态分区分配演示：FF 与 BF 对比
void runPartitionDemo() {
    std::cout << "\n=== 动态分区内存管理演示 (总内存 640KB) ===\n";
    std::cout << "构造场景: 先占满再回收, 形成'大洞在前、小洞在后'的空闲布局,\n";
    std::cout << "随后申请 50KB, 观察 FF 与 BF 的选块差异。\n";

    auto scenario = [](PartitionAlgo algo) {
        PartitionAllocator alloc(640, algo);
        std::cout << "\n--- " << alloc.getAlgorithmStr() << " ---\n";
        alloc.allocate(1, 100);  // A
        alloc.allocate(2, 200);  // B  (大块)
        alloc.allocate(3, 100);  // C
        alloc.allocate(4, 60);   // D  (小块)
        alloc.allocate(5, 100);  // E
        std::cout << "  >> 回收作业B(200KB) 与 作业D(60KB)\n";
        alloc.reclaim(2);
        alloc.reclaim(4);
        alloc.printLayout();
        std::cout << "  >> 申请作业F(50KB): FF 选首个(200KB大洞), BF 选最接近(60KB小洞)\n";
        alloc.allocate(6, 50);   // F
        alloc.printLayout();
    };

    scenario(PartitionAlgo::FIRST_FIT);
    scenario(PartitionAlgo::BEST_FIT);
    std::cout << "\n说明: FF 从低地址首个够大的空闲块切分(切碎大洞); BF 选容量最接近的空闲块,\n";
    std::cout << "      保留大洞备用但更易产生难以利用的小碎片。\n";
}

void runReadersWritersDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    g_rw_count_mutex = std::make_unique<Mutex>();
    g_rw_write_sem = std::make_unique<Semaphore>(1);  // 二元信号量保证写者互斥

    std::cout << "\n=== 读者-写者问题演示 (读者优先) ===\n";
    std::cout << "3 个读者可并发读取; 写者独占访问。\n\n";

    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    runtime.createThread(5, readerThread);
    runtime.createThread(5, readerThread);
    runtime.createThread(5, writerThread);
    runtime.createThread(5, readerThread);
    runtime.run();
}

void runDiningPhilosophersDemo(UThreadRuntime& runtime) {
    resetDemoState(runtime);
    for (int i = 0; i < 5; ++i) {
        g_forks[i] = std::make_unique<Mutex>();
    }

    std::cout << "\n=== 哲学家进餐问题演示 (5 位哲学家) ===\n";
    std::cout << "采用'资源有序分配'(总是先拿编号小的叉子)破坏循环等待，保证无死锁。\n\n";

    runtime.setPolicy(SchedulePolicy::RR);
    runtime.setQuantum(1);
    for (int i = 0; i < 5; ++i) {
        runtime.createThread(5, philosopherThread);
    }
    runtime.run();
    std::cout << "\n✅ 全部哲学家用餐完毕，未发生死锁或饿死。\n";
}

// 调度演示统计：基于调度器逻辑时钟计算真实的等待/周转时间
struct DemoStats {
    std::vector<int> order;
    double avg_wait;        // 平均等待时间（逻辑时钟单位）
    double avg_turn;        // 平均周转时间（逻辑时钟单位）
    double throughput;      // 吞吐量（任务 / 逻辑时钟）
};

// burst：每个线程的预估服务时间（逻辑片段数）
DemoStats runSchedDemo(UThreadRuntime& runtime, SchedulePolicy policy,
                       const std::vector<int>& priorities, const std::vector<int>& bursts) {
    runtime.clear();
    g_schedule_order.clear();
    runtime.getScheduler().resetLogicalClock();

    std::vector<int> tids;
    for (size_t i = 0; i < priorities.size(); ++i) {
        int tid = runtime.createThread(priorities[i], demoThreadFunc);
        auto t = runtime.getThread(tid);
        if (t) {
            t->setBurstTime(bursts[i]);
            t->setArrivalTick(0);  // 批处理模型：所有作业 0 时刻到达
        }
        tids.push_back(tid);
    }
    runtime.setPolicy(policy);
    runtime.run();

    DemoStats stats;
    stats.order = g_schedule_order;

    // 真实量化：等待 = 首次运行时刻 - 到达; 周转 = 完成时刻 - 到达
    double total_wait = 0.0, total_turn = 0.0;
    int n = 0, last_finish = 0;
    for (int tid : tids) {
        auto t = runtime.getThread(tid);
        if (!t || t->getFinishTick() < 0) continue;
        int wait = t->getFirstRunTick() - t->getArrivalTick();
        int turn = t->getFinishTick() - t->getArrivalTick();
        total_wait += wait;
        total_turn += turn;
        last_finish = std::max(last_finish, t->getFinishTick());
        ++n;
    }
    stats.avg_wait = n > 0 ? total_wait / n : 0.0;
    stats.avg_turn = n > 0 ? total_turn / n : 0.0;
    stats.throughput = last_finish > 0 ? static_cast<double>(n) / last_finish : 0.0;
    return stats;
}

void printScheduleOrder(const std::string& name, const std::vector<int>& order) {
    std::cout << "│ " << std::left << std::setw(8) << name << " │ ";
    bool first = true;
    int width_left = 54;
    for (int tid : order) {
        std::string s = "#" + std::to_string(tid);
        if (!first) {
            std::cout << " -> ";
            width_left -= 4;
        }
        std::cout << s;
        width_left -= s.size();
        first = false;
        if (width_left <= 6) break;
    }
    for (int i = 0; i < width_left; ++i) std::cout << " ";
    std::cout << " │\n";
}

void printStatsRow(const std::string& name, const DemoStats& s) {
    std::cout << "│ " << std::left << std::setw(8) << name << " │ "
              << std::setw(16) << std::fixed << std::setprecision(2) << s.avg_wait << " │ "
              << std::setw(16) << std::fixed << std::setprecision(2) << s.avg_turn << " │ "
              << std::setw(16) << std::fixed << std::setprecision(3) << s.throughput << " │\n";
}

void autoDemo(UThreadRuntime& runtime) {
    runtime.clear();
    std::cout << "--- 自动化演示开始 (批处理模型, 各作业 0 时刻到达) ---\n";

    // 优先级与预估服务时间(burst)。tid 依次为 1..5
    std::vector<int> priorities = {5, 2, 8, 1, 4};
    std::vector<int> bursts     = {3, 5, 2, 4, 1};
    std::cout << "作业表: ";
    for (size_t i = 0; i < priorities.size(); ++i) {
        std::cout << "T" << (i + 1) << "(pri=" << priorities[i]
                  << ",burst=" << bursts[i] << ") ";
    }
    std::cout << "\n";

    std::cout << "\n[FCFS/FIFO 调度演示]\n";
    DemoStats fifo_stats = runSchedDemo(runtime, SchedulePolicy::FIFO, priorities, bursts);

    std::cout << "\n[SJF 短作业优先调度演示]\n";
    DemoStats sjf_stats = runSchedDemo(runtime, SchedulePolicy::SJF, priorities, bursts);

    std::cout << "\n[RR 时间片轮转调度演示 (quantum=2)]\n";
    runtime.setQuantum(2);
    DemoStats rr_stats = runSchedDemo(runtime, SchedulePolicy::RR, priorities, bursts);

    std::cout << "\n[Priority 优先级调度演示]\n";
    DemoStats prio_stats = runSchedDemo(runtime, SchedulePolicy::PRIORITY, priorities, bursts);

    std::cout << "\n--- 自动化演示结束 ---\n\n";

    // 打印调度顺序对比表（按时间片粒度）
    std::cout << "┌──────────┬────────────────────────────────────────────────────────┐\n";
    std::cout << "│ 调度算法 │              线程时间片执行顺序(逻辑时钟)              │\n";
    std::cout << "├──────────┼────────────────────────────────────────────────────────┤\n";
    printScheduleOrder("FCFS", fifo_stats.order);
    printScheduleOrder("SJF", sjf_stats.order);
    printScheduleOrder("RR", rr_stats.order);
    printScheduleOrder("Priority", prio_stats.order);
    std::cout << "└──────────┴────────────────────────────────────────────────────────┘\n\n";

    // 打印性能量化对比表（真实逻辑时钟统计）
    std::cout << "┌──────────┬──────────────────┬──────────────────┬──────────────────┐\n";
    std::cout << "│ 调度算法 │   平均等待时间   │   平均周转时间   │ 吞吐量(任务/时钟)│\n";
    std::cout << "├──────────┼──────────────────┼──────────────────┼──────────────────┤\n";
    printStatsRow("FCFS", fifo_stats);
    printStatsRow("SJF", sjf_stats);
    printStatsRow("RR", rr_stats);
    printStatsRow("Priority", prio_stats);
    std::cout << "└──────────┴──────────────────┴──────────────────┴──────────────────┘\n";
    std::cout << "说明: 时间单位为调度器逻辑时钟(每个时间片+1)。SJF 通常取得最短平均等待时间;\n";
    std::cout << "      RR 兼顾响应公平; Priority 优先处理紧急(低优先级数)任务。\n";
}

int main() {
    UThreadRuntime runtime;

    while (true) {
        printMenu();
        int choice;
        if (!(std::cin >> choice)) {
            break;
        }
        switch (choice) {
            case 1: handleCreate(runtime); break;
            case 2: runtime.printAll(); break;
            case 3: handleSetPriority(runtime); break;
            case 4: handleBlock(runtime); break;
            case 5: handleWake(runtime); break;
            case 6: handleRemove(runtime); break;
            case 7: handleSelectPolicy(runtime); break;
            case 8: runtime.run(); break;
            case 9: autoDemo(runtime); break;
            case 10: handleSetQuantum(runtime); break;
            case 11: handleJoin(runtime); break;
            case 12: runMutexDemo(runtime); break;
            case 13: runSemaphoreDemo(runtime); break;
            case 14: runConditionDemo(runtime); break;
            case 15: runProducerConsumerDemo(runtime); break;
            case 16: runDeadlockDemo(runtime); break;
            case 17: runDeadlockAvoidDemo(runtime); break;
            case 18: runRingBufferDemo(runtime); break;
            case 19: runVirtualMemoryDemo(); break;
            case 20: runPartitionDemo(); break;
            case 21: runReadersWritersDemo(runtime); break;
            case 22: runDiningPhilosophersDemo(runtime); break;
            case 23: runVirtualFSDemo(); break;
            case 0: return 0;
            default: std::cout << "无效选项\n";
        }
    }
    return 0;
}
