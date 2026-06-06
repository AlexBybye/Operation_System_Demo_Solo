#pragma once

#define _XOPEN_SOURCE 600

/**
 * @file context.h
 * @brief 上下文切换工具函数
 *
 * 封装 ucontext 系列函数，提供更简洁的上下文操作接口。
 */

#include <ucontext.h>
#include <cstddef>

namespace uthread {

/**
 * @brief 为线程分配栈空间
 * @param size 栈大小（字节）
 * @return 栈底指针（malloc 分配）
 *
 * 注意：调用者负责释放（free）
 */
char* allocateStack(size_t size);

/**
 * @brief 释放栈空间
 * @param stack 栈指针
 */
void freeStack(char* stack);

/**
 * @brief 初始化一个上下文用于运行指定函数
 * @param ctx 要初始化的上下文
 * @param stack 栈空间指针
 * @param stack_size 栈大小
 * @param link_ctx 后继上下文（函数返回后切换到此处）
 * @param func 要执行的函数
 *
 * 封装 getcontext + 设置栈 + makecontext
 */
void initContext(ucontext_t& ctx, char* stack, size_t stack_size,
                 ucontext_t* link_ctx, void (*func)());

/**
 * @brief 上下文切换
 * @param from 保存当前上下文
 * @param to 切换到目标上下文
 *
 * 封装 swapcontext
 */
void switchContext(ucontext_t& from, ucontext_t& to);

} // namespace uthread
