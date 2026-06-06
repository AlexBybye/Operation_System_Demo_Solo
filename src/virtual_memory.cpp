#include "virtual_memory.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace uthread {

VirtualMemory::VirtualMemory(int num_frames, int num_pages, PageReplaceAlgo algo)
    : num_frames_(num_frames),
      num_pages_(num_pages),
      algo_(algo),
      page_faults_(0),
      total_accesses_(0) {
    page_table_.resize(num_pages);
    for (int i = 0; i < num_pages; ++i) {
        page_table_[i] = {i, false, -1};
    }
    frames_.resize(num_frames, -1);
}

bool VirtualMemory::accessPage(int page_id) {
    if (page_id < 0 || page_id >= num_pages_) return false;
    ++total_accesses_;

    if (page_table_[page_id].valid) {
        // 命中
        if (algo_ == PageReplaceAlgo::LRU) {
            lru_list_.remove(page_id);
            lru_list_.push_front(page_id);
        }
        return true;
    }

    // 缺页
    ++page_faults_;
    std::cout << "  [缺页] 页面 " << page_id << " 不在内存中";

    int frame = -1;
    // 查找空闲帧
    for (int i = 0; i < num_frames_; ++i) {
        if (frames_[i] == -1) {
            frame = i;
            break;
        }
    }

    if (frame == -1) {
        if (algo_ == PageReplaceAlgo::FIFO) {
            frame = replaceFIFO(page_id);
        } else {
            frame = replaceLRU(page_id);
        }
    } else {
        frames_[frame] = page_id;
        if (algo_ == PageReplaceAlgo::FIFO) {
            fifo_queue_.push_back(page_id);
        } else {
            lru_list_.push_front(page_id);
        }
    }

    page_table_[page_id].valid = true;
    page_table_[page_id].frame_id = frame;
    std::cout << " -> 装入帧 " << frame << "\n";
    return false;
}

int VirtualMemory::replaceFIFO(int page_id) {
    int victim_page = fifo_queue_.front();
    fifo_queue_.pop_front();
    int frame = page_table_[victim_page].frame_id;
    std::cout << " (淘汰页面 " << victim_page << ")";

    page_table_[victim_page].valid = false;
    page_table_[victim_page].frame_id = -1;

    frames_[frame] = page_id;
    fifo_queue_.push_back(page_id);
    return frame;
}

int VirtualMemory::replaceLRU(int page_id) {
    int victim_page = lru_list_.back();
    lru_list_.pop_back();
    int frame = page_table_[victim_page].frame_id;
    std::cout << " (淘汰页面 " << victim_page << ")";

    page_table_[victim_page].valid = false;
    page_table_[victim_page].frame_id = -1;

    frames_[frame] = page_id;
    lru_list_.push_front(page_id);
    return frame;
}

double VirtualMemory::getPageFaultRate() const {
    if (total_accesses_ == 0) return 0.0;
    return static_cast<double>(page_faults_) / total_accesses_ * 100.0;
}

int VirtualMemory::getPageFaults() const {
    return page_faults_;
}

int VirtualMemory::getTotalAccesses() const {
    return total_accesses_;
}

void VirtualMemory::setAlgorithm(PageReplaceAlgo algo) {
    algo_ = algo;
    reset();
}

std::string VirtualMemory::getAlgorithmStr() const {
    return algo_ == PageReplaceAlgo::FIFO ? "FIFO" : "LRU";
}

void VirtualMemory::printState() const {
    std::cout << "  物理帧状态: [";
    for (int i = 0; i < num_frames_; ++i) {
        if (frames_[i] == -1) std::cout << " - ";
        else std::cout << " " << frames_[i] << " ";
        if (i < num_frames_ - 1) std::cout << "|";
    }
    std::cout << "]\n";
}

void VirtualMemory::reset() {
    for (auto& entry : page_table_) {
        entry.valid = false;
        entry.frame_id = -1;
    }
    std::fill(frames_.begin(), frames_.end(), -1);
    fifo_queue_.clear();
    lru_list_.clear();
    page_faults_ = 0;
    total_accesses_ = 0;
}

void VirtualMemory::runDemo(const std::vector<int>& access_sequence) {
    std::cout << "  页面访问序列: ";
    for (int p : access_sequence) std::cout << p << " ";
    std::cout << "\n  置换算法: " << getAlgorithmStr() << "\n";
    std::cout << "  物理帧数: " << num_frames_ << "\n\n";

    for (int page : access_sequence) {
        bool hit = accessPage(page);
        if (hit) {
            std::cout << "  [命中] 页面 " << page << " 已在内存中\n";
        }
        printState();
    }

    std::cout << "\n  统计: 总访问 " << total_accesses_
              << " 次, 缺页 " << page_faults_
              << " 次, 缺页率 " << std::fixed << std::setprecision(1)
              << getPageFaultRate() << "%\n";
}

} // namespace uthread
