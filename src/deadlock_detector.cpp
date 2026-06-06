#include "deadlock_detector.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace uthread {

DeadlockDetector& DeadlockDetector::getInstance() {
    static DeadlockDetector instance;
    return instance;
}

void DeadlockDetector::addWaitEdge(int tid, int mutex_id) {
    int t_node = threadNode(tid);
    int m_node = mutexNode(mutex_id);
    graph_[t_node].push_back(m_node);
}

void DeadlockDetector::addHoldEdge(int mutex_id, int tid) {
    int m_node = mutexNode(mutex_id);
    int t_node = threadNode(tid);
    graph_[m_node].push_back(t_node);
}

void DeadlockDetector::removeHoldEdge(int mutex_id, int tid) {
    int m_node = mutexNode(mutex_id);
    int t_node = threadNode(tid);
    auto& neighbors = graph_[m_node];
    neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), t_node), neighbors.end());
}

void DeadlockDetector::removeWaitEdge(int tid, int mutex_id) {
    int t_node = threadNode(tid);
    int m_node = mutexNode(mutex_id);
    auto& neighbors = graph_[t_node];
    neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), m_node), neighbors.end());
}

bool DeadlockDetector::detect() {
    std::unordered_set<int> visited;
    std::unordered_set<int> rec_stack;
    std::vector<int> path;

    for (auto& [node, _] : graph_) {
        if (visited.find(node) == visited.end()) {
            if (dfs(node, visited, rec_stack, path)) {
                return true;
            }
        }
    }
    last_cycle_info_ = "";
    return false;
}

bool DeadlockDetector::dfs(int node, std::unordered_set<int>& visited,
                           std::unordered_set<int>& rec_stack, std::vector<int>& path) {
    visited.insert(node);
    rec_stack.insert(node);
    path.push_back(node);

    if (graph_.find(node) != graph_.end()) {
        for (int neighbor : graph_[node]) {
            if (rec_stack.find(neighbor) != rec_stack.end()) {
                // 发现环路，构建环路信息
                std::ostringstream oss;
                oss << "⚠️  死锁检测: 发现环路! ";
                auto it = std::find(path.begin(), path.end(), neighbor);
                for (; it != path.end(); ++it) {
                    if (*it >= 0) {
                        oss << "线程#" << *it;
                    } else {
                        oss << "锁#" << (-(*it) - 1);
                    }
                    oss << " -> ";
                }
                if (neighbor >= 0) {
                    oss << "线程#" << neighbor;
                } else {
                    oss << "锁#" << (-(neighbor) - 1);
                }
                last_cycle_info_ = oss.str();
                return true;
            }
            if (visited.find(neighbor) == visited.end()) {
                if (dfs(neighbor, visited, rec_stack, path)) {
                    return true;
                }
            }
        }
    }

    path.pop_back();
    rec_stack.erase(node);
    return false;
}

std::string DeadlockDetector::getLastCycleInfo() const {
    return last_cycle_info_;
}

void DeadlockDetector::clear() {
    graph_.clear();
    last_cycle_info_.clear();
}

} // namespace uthread
