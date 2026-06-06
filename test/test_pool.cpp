#include "thread_pool.h"
#include <iostream>
#include <cassert>

using namespace uthread;

namespace uthread {
    void threadWrapper() {}
}

void dummy_func() {}

void test_thread_pool() {
    ThreadPool pool;
    assert(pool.size() == 0);
    
    int tid1 = pool.createThread(5, dummy_func);
    assert(tid1 == 1);
    
    int tid2 = pool.createThread(2, dummy_func);
    assert(tid2 == 2);
    
    int tid3 = pool.createThread(8, dummy_func);
    assert(tid3 == 3);
    
    assert(pool.size() == 3);
    
    auto t1 = pool.getThread(tid1);
    assert(t1 != nullptr);
    assert(t1->getTid() == 1);
    assert(t1->getPriority() == 5);
    
    auto t_invalid = pool.getThread(99);
    assert(t_invalid == nullptr);
    
    assert(pool.removeThread(tid2) == true);
    assert(pool.size() == 2);
    
    assert(pool.removeThread(99) == false);
    
    auto ready_threads = pool.getThreadsByState(ThreadState::READY);
    assert(ready_threads.size() == 2);
    
    pool.printAll();
    
    pool.clear();
    assert(pool.size() == 0);
    
    std::cout << "All ThreadPool tests passed!\n";
}

int main() {
    test_thread_pool();
    return 0;
}
