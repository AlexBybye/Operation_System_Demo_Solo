#include "partition_allocator.h"
#include <iostream>
#include <iomanip>

namespace uthread {

PartitionAllocator::PartitionAllocator(int total_size, PartitionAlgo algo)
    : total_size_(total_size), algo_(algo) {
    // 初始为一整块空闲分区
    partitions_.push_back({0, total_size, true, -1});
}

int PartitionAllocator::allocate(int job_id, int size) {
    if (size <= 0) return -1;

    auto target = partitions_.end();

    if (algo_ == PartitionAlgo::FIRST_FIT) {
        // 首次适应：取第一个足够大的空闲块
        for (auto it = partitions_.begin(); it != partitions_.end(); ++it) {
            if (it->free && it->size >= size) {
                target = it;
                break;
            }
        }
    } else {
        // 最佳适应：取容量最接近的空闲块
        int best_size = total_size_ + 1;
        for (auto it = partitions_.begin(); it != partitions_.end(); ++it) {
            if (it->free && it->size >= size && it->size < best_size) {
                best_size = it->size;
                target = it;
            }
        }
    }

    if (target == partitions_.end()) {
        std::cout << "  [分配失败] 作业#" << job_id << " 请求 " << size
                  << "KB，无足够大的连续空闲块 (" << getAlgorithmStr() << ")\n";
        return -1;
    }

    int start = target->start;
    int remaining = target->size - size;

    // 占用块
    target->free = false;
    target->size = size;
    target->owner_id = job_id;

    // 若有剩余空间，分裂出一个新的空闲块插在其后
    if (remaining > 0) {
        Partition leftover{start + size, remaining, true, -1};
        auto next = target;
        ++next;
        partitions_.insert(next, leftover);
    }

    std::cout << "  [分配成功] 作业#" << job_id << " 获得 [" << start << ", "
              << (start + size - 1) << "] 共 " << size << "KB (" << getAlgorithmStr() << ")\n";
    return start;
}

bool PartitionAllocator::reclaim(int job_id) {
    bool found = false;
    for (auto& p : partitions_) {
        if (!p.free && p.owner_id == job_id) {
            p.free = true;
            p.owner_id = -1;
            found = true;
            std::cout << "  [回收] 作业#" << job_id << " 释放 [" << p.start << ", "
                      << (p.start + p.size - 1) << "]\n";
        }
    }
    if (found) {
        mergeAdjacentFree();
    } else {
        std::cout << "  [回收失败] 未找到作业#" << job_id << " 的分区\n";
    }
    return found;
}

void PartitionAllocator::mergeAdjacentFree() {
    auto it = partitions_.begin();
    while (it != partitions_.end()) {
        auto next = it;
        ++next;
        if (next != partitions_.end() && it->free && next->free) {
            // 合并相邻空闲块
            it->size += next->size;
            partitions_.erase(next);
            // 不前进 it，继续尝试与新的后继合并
        } else {
            ++it;
        }
    }
}

void PartitionAllocator::setAlgorithm(PartitionAlgo algo) {
    algo_ = algo;
}

std::string PartitionAllocator::getAlgorithmStr() const {
    return algo_ == PartitionAlgo::FIRST_FIT ? "首次适应FF" : "最佳适应BF";
}

void PartitionAllocator::printLayout() const {
    std::cout << "  内存布局 (总 " << total_size_ << "KB):\n  ";
    for (const auto& p : partitions_) {
        if (p.free) {
            std::cout << "[空闲 " << p.size << "KB]";
        } else {
            std::cout << "[作业#" << p.owner_id << " " << p.size << "KB]";
        }
    }
    std::cout << "\n  空闲块数: " << freeBlockCount()
              << ", 总空闲: " << totalFreeSize() << "KB\n";
}

int PartitionAllocator::freeBlockCount() const {
    int n = 0;
    for (const auto& p : partitions_) {
        if (p.free) ++n;
    }
    return n;
}

int PartitionAllocator::totalFreeSize() const {
    int total = 0;
    for (const auto& p : partitions_) {
        if (p.free) total += p.size;
    }
    return total;
}

} // namespace uthread
