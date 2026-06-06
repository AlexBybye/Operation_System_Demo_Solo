# 编译器与选项
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -g -I include
LDFLAGS  :=

# 跨平台兼容性：macOS 上 ucontext 系列函数已废弃，抑制相关警告
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CXXFLAGS += -Wno-deprecated-declarations
endif

# 目录
SRC_DIR   := src
TEST_DIR  := test
BUILD_DIR := build
INC_DIR   := include

# 源文件
SRCS := $(SRC_DIR)/tcb.cpp \
        $(SRC_DIR)/thread_pool.cpp \
        $(SRC_DIR)/scheduler.cpp \
        $(SRC_DIR)/context.cpp \
        $(SRC_DIR)/uthread.cpp \
        $(SRC_DIR)/mutex.cpp \
        $(SRC_DIR)/semaphore.cpp \
        $(SRC_DIR)/condition_variable.cpp \
        $(SRC_DIR)/deadlock_detector.cpp \
        $(SRC_DIR)/ring_buffer.cpp \
        $(SRC_DIR)/virtual_memory.cpp \
        $(SRC_DIR)/partition_allocator.cpp \
        $(SRC_DIR)/virtual_fs.cpp

# 目标文件
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# 测试目标
TEST_MAIN  := $(BUILD_DIR)/test_main
TEST_TCB   := $(BUILD_DIR)/test_tcb
TEST_POOL  := $(BUILD_DIR)/test_pool
TEST_SCHED := $(BUILD_DIR)/test_sched
TEST_SYNC  := $(BUILD_DIR)/test_sync

# ========== 目标 ==========

.PHONY: all clean run test_main test_tcb test_pool test_sched test_sync

all: $(TEST_MAIN) $(TEST_TCB) $(TEST_POOL) $(TEST_SCHED) $(TEST_SYNC)
	@echo "✅ 全部编译完成"

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译源文件为目标文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ===== 集成测试 =====
$(TEST_MAIN): $(TEST_DIR)/test_main.cpp $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_main: $(TEST_MAIN)
	@echo "🚀 运行集成测试..."
	./$(TEST_MAIN)

# ===== TCB 单测 =====
$(TEST_TCB): $(TEST_DIR)/test_tcb.cpp $(BUILD_DIR)/tcb.o $(BUILD_DIR)/context.o | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_tcb: $(TEST_TCB)
	@echo "🧪 运行 TCB 单测..."
	./$(TEST_TCB)

# ===== 线程池单测 =====
$(TEST_POOL): $(TEST_DIR)/test_pool.cpp $(BUILD_DIR)/tcb.o $(BUILD_DIR)/thread_pool.o $(BUILD_DIR)/context.o | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_pool: $(TEST_POOL)
	@echo "🧪 运行线程池单测..."
	./$(TEST_POOL)

# ===== 调度器单测 =====
$(TEST_SCHED): $(TEST_DIR)/test_scheduler.cpp $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_sched: $(TEST_SCHED)
	@echo "🧪 运行调度器单测..."
	./$(TEST_SCHED)

# ===== 同步原语单测 =====
$(TEST_SYNC): $(TEST_DIR)/test_sync.cpp $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_sync: $(TEST_SYNC)
	@echo "🧪 运行同步原语单测..."
	./$(TEST_SYNC)

# ===== 运行集成测试 =====
run: $(TEST_MAIN)
	./$(TEST_MAIN)

# ===== 清理 =====
clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 清理完成"
