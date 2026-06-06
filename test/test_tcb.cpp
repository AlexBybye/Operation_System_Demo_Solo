#include "tcb.h"
#include <iostream>
#include <cassert>
#include <unistd.h>

using namespace uthread;

namespace uthread {
    void threadWrapper() {}
}

void dummy_func() {}

void test_initial_state() {
    TCB tcb(1, 5, dummy_func);
    assert(tcb.getState() == ThreadState::READY);
    assert(tcb.getTid() == 1);
    assert(tcb.getPriority() == 5);
    assert(tcb.getPid() == getpid());
    assert(!tcb.isFinished());
    std::cout << "test_initial_state passed\n";
}

void test_valid_transitions() {
    TCB tcb(1, 5, dummy_func);
    
    // READY -> RUNNING
    assert(tcb.setState(ThreadState::RUNNING) == true);
    assert(tcb.getState() == ThreadState::RUNNING);
    
    // RUNNING -> READY
    assert(tcb.setState(ThreadState::READY) == true);
    assert(tcb.getState() == ThreadState::READY);
    
    // READY -> RUNNING again
    assert(tcb.setState(ThreadState::RUNNING) == true);
    
    // RUNNING -> BLOCKED
    assert(tcb.setState(ThreadState::BLOCKED) == true);
    assert(tcb.getState() == ThreadState::BLOCKED);
    
    // BLOCKED -> READY
    assert(tcb.setState(ThreadState::READY) == true);
    assert(tcb.getState() == ThreadState::READY);
    
    // READY -> RUNNING again
    assert(tcb.setState(ThreadState::RUNNING) == true);
    
    // RUNNING -> FINISHED
    assert(tcb.setState(ThreadState::FINISHED) == true);
    assert(tcb.getState() == ThreadState::FINISHED);
    
    std::cout << "test_valid_transitions passed\n";
}

void test_invalid_transitions() {
    TCB tcb(1, 5, dummy_func);
    
    // READY -> BLOCKED
    assert(tcb.setState(ThreadState::BLOCKED) == false);
    assert(tcb.getState() == ThreadState::READY);
    
    // READY -> FINISHED
    assert(tcb.setState(ThreadState::FINISHED) == false);
    
    tcb.setState(ThreadState::RUNNING);
    tcb.setState(ThreadState::FINISHED);
    
    // FINISHED -> READY
    assert(tcb.setState(ThreadState::READY) == false);
    assert(tcb.getState() == ThreadState::FINISHED);
    
    std::cout << "test_invalid_transitions passed\n";
}

void test_priority_clamp() {
    TCB tcb1(1, 0, dummy_func); // < 1
    assert(tcb1.getPriority() == 1);
    
    TCB tcb2(2, 11, dummy_func); // > 10
    assert(tcb2.getPriority() == 10);
    
    TCB tcb3(3, 5, dummy_func);
    tcb3.setPriority(15);
    assert(tcb3.getPriority() == 10);
    tcb3.setPriority(-5);
    assert(tcb3.getPriority() == 1);
    
    std::cout << "test_priority_clamp passed\n";
}

int main() {
    test_initial_state();
    test_valid_transitions();
    test_invalid_transitions();
    test_priority_clamp();
    std::cout << "All TCB tests passed!\n";
    return 0;
}
