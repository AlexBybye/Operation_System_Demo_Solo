#pragma once

#include <list>
#include <string>

namespace uthread {

/// 动态分区分配算法
enum class PartitionAlgo {
    FIRST_FIT,  // 首次适应
    BEST_FIT    // 最佳适应
};

/// 内存分区块
struct Partition {
    int start;      // 起始地址
    int size;       // 大小
    bool free;      // 是否空闲
    int owner_id;   // 占用者ID（-1 表示空闲）
};

/**
 * @class PartitionAllocator
 * @brief 动态分区内存管理器
 *
 * 模拟连续内存的动态分区分配与回收，支持首次适应(FF)与最佳适应(BF)。
 * 回收时自动合并相邻空闲分区，可视化展示内存空闲块的分裂与合并。
 */
class PartitionAllocator {
public:
    PartitionAllocator(int total_size, PartitionAlgo algo = PartitionAlgo::FIRST_FIT);
    ~PartitionAllocator() = default;

    /// 为作业分配内存，返回起始地址，失败返回 -1
    int allocate(int job_id, int size);

    /// 回收指定作业的内存，并合并相邻空闲块
    bool reclaim(int job_id);

    void setAlgorithm(PartitionAlgo algo);
    std::string getAlgorithmStr() const;

    /// 可视化打印当前内存布局
    void printLayout() const;

    /// 统计外部碎片（空闲块总数及总空闲大小）
    int freeBlockCount() const;
    int totalFreeSize() const;

private:
    void mergeAdjacentFree();

    int total_size_;
    PartitionAlgo algo_;
    std::list<Partition> partitions_;
};

} // namespace uthread
