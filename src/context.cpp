#include "context.h"
#include <cstdlib>
#include <iostream>

namespace uthread {

char* allocateStack(size_t size) {
    if (size == 0) return nullptr;
    char* stack = static_cast<char*>(std::malloc(size));
    if (!stack) {
        std::cerr << "Failed to allocate stack of size " << size << std::endl;
        return nullptr;
    }
    return stack;
}

void freeStack(char* stack) {
    if (stack) {
        std::free(stack);
    }
}

void initContext(ucontext_t& ctx, char* stack, size_t stack_size,
                 ucontext_t* link_ctx, void (*func)()) {
    if (getcontext(&ctx) == -1) {
        std::cerr << "Failed to get context" << std::endl;
        return;
    }
    
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = stack_size;
    ctx.uc_stack.ss_flags = 0;
    ctx.uc_link = link_ctx;
    
    makecontext(&ctx, func, 0);
}

void switchContext(ucontext_t& from, ucontext_t& to) {
    if (swapcontext(&from, &to) == -1) {
        std::cerr << "Failed to swap context" << std::endl;
    }
}

} // namespace uthread
