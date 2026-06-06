#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <string>

namespace uthread {

enum class PageReplaceAlgo {
    FIFO,
    LRU
};

struct PageTableEntry {
    int page_id;
    bool valid;
    int frame_id;
};

class VirtualMemory {
public:
    VirtualMemory(int num_frames, int num_pages, PageReplaceAlgo algo = PageReplaceAlgo::FIFO);
    ~VirtualMemory() = default;

    // 访问一个页面，返回 true 表示命中，false 表示缺页
    bool accessPage(int page_id);

    // 获取缺页率
    double getPageFaultRate() const;
    int getPageFaults() const;
    int getTotalAccesses() const;

    // 设置置换算法
    void setAlgorithm(PageReplaceAlgo algo);
    std::string getAlgorithmStr() const;

    // 打印当前页表和物理帧状态
    void printState() const;

    // 重置统计
    void reset();

    // 运行演示序列
    void runDemo(const std::vector<int>& access_sequence);

private:
    int replaceFIFO(int page_id);
    int replaceLRU(int page_id);

    int num_frames_;
    int num_pages_;
    PageReplaceAlgo algo_;

    std::vector<PageTableEntry> page_table_;
    std::vector<int> frames_;           // 物理帧中存放的页号 (-1 表示空)
    std::list<int> fifo_queue_;         // FIFO 队列
    std::list<int> lru_list_;           // LRU 链表 (最近使用在前)

    int page_faults_;
    int total_accesses_;
};

} // namespace uthread
