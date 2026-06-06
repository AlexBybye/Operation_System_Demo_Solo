#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace uthread {

class DeadlockDetector {
public:
    static DeadlockDetector& getInstance();

    // 线程正在等待某个锁
    void addWaitEdge(int tid, int mutex_id);
    // 线程成功获取了某个锁
    void addHoldEdge(int mutex_id, int tid);
    // 线程释放了某个锁
    void removeHoldEdge(int mutex_id, int tid);
    // 线程不再等待某个锁
    void removeWaitEdge(int tid, int mutex_id);

    // 检测是否存在死锁环路，返回 true 表示检测到死锁
    bool detect();

    // 获取最近一次检测到的死锁环路描述
    std::string getLastCycleInfo() const;

    // 清空所有状态
    void clear();

private:
    DeadlockDetector() = default;

    // DFS 环路检测
    bool dfs(int node, std::unordered_set<int>& visited,
             std::unordered_set<int>& rec_stack, std::vector<int>& path);

    // 节点编码：线程 TID 用正数，锁 ID 用负数
    static int threadNode(int tid) { return tid; }
    static int mutexNode(int mutex_id) { return -(mutex_id + 1); }

    // 邻接表：node -> [neighbors]
    std::unordered_map<int, std::vector<int>> graph_;
    std::string last_cycle_info_;
};

} // namespace uthread
