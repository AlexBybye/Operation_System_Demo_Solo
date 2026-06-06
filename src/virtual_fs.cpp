#include "virtual_fs.h"
#include <iostream>
#include <iomanip>
#include <cstring>

namespace uthread {

VirtualFS::VirtualFS() {
    for (int i = 0; i < VFS_MAX_FILES; ++i) {
        inodes_[i].used = false;
        inodes_[i].size = 0;
        inodes_[i].block_count = 0;
        std::memset(inodes_[i].name, 0, VFS_MAX_NAME);
    }
    for (int i = 0; i < VFS_MAX_BLOCKS; ++i) {
        bitmap_[i] = false;
        std::memset(data_blocks_[i], 0, VFS_BLOCK_SIZE);
    }
}

int VirtualFS::findFile(const std::string& name) const {
    for (int i = 0; i < VFS_MAX_FILES; ++i) {
        if (inodes_[i].used && std::strncmp(inodes_[i].name, name.c_str(), VFS_MAX_NAME) == 0) {
            return i;
        }
    }
    return -1;
}

int VirtualFS::allocBlock() {
    for (int i = 0; i < VFS_MAX_BLOCKS; ++i) {
        if (!bitmap_[i]) {
            bitmap_[i] = true;
            return i;
        }
    }
    return -1;
}

void VirtualFS::freeBlock(int block_id) {
    if (block_id >= 0 && block_id < VFS_MAX_BLOCKS) {
        bitmap_[block_id] = false;
        std::memset(data_blocks_[block_id], 0, VFS_BLOCK_SIZE);
    }
}

int VirtualFS::my_create(const std::string& name) {
    if (name.empty() || name.size() >= VFS_MAX_NAME) return -1;
    if (findFile(name) >= 0) {
        std::cout << "  [VFS] 文件 \"" << name << "\" 已存在\n";
        return -1;
    }
    for (int i = 0; i < VFS_MAX_FILES; ++i) {
        if (!inodes_[i].used) {
            inodes_[i].used = true;
            std::strncpy(inodes_[i].name, name.c_str(), VFS_MAX_NAME - 1);
            inodes_[i].size = 0;
            inodes_[i].block_count = 0;
            std::cout << "  [VFS] 创建文件 \"" << name << "\" (inode #" << i << ")\n";
            return i;
        }
    }
    std::cout << "  [VFS] inode 表已满\n";
    return -1;
}

int VirtualFS::my_write(const std::string& name, const std::string& data) {
    int idx = findFile(name);
    if (idx < 0) {
        std::cout << "  [VFS] 文件 \"" << name << "\" 不存在\n";
        return -1;
    }

    Inode& inode = inodes_[idx];
    // 释放旧的数据块
    for (int i = 0; i < inode.block_count; ++i) {
        freeBlock(inode.blocks[i]);
    }
    inode.block_count = 0;
    inode.size = 0;

    int needed_blocks = (data.size() + VFS_BLOCK_SIZE - 1) / VFS_BLOCK_SIZE;
    if (needed_blocks > VFS_BLOCKS_PER_FILE) {
        std::cout << "  [VFS] 文件过大，超过单文件最大块数\n";
        return -1;
    }

    int written = 0;
    for (int i = 0; i < needed_blocks; ++i) {
        int blk = allocBlock();
        if (blk < 0) {
            std::cout << "  [VFS] 磁盘空间不足\n";
            return -1;
        }
        inode.blocks[i] = blk;
        ++inode.block_count;

        int chunk = std::min<int>(VFS_BLOCK_SIZE, data.size() - written);
        std::memcpy(data_blocks_[blk], data.data() + written, chunk);
        written += chunk;
    }
    inode.size = data.size();
    std::cout << "  [VFS] 写入 \"" << name << "\" 共 " << written
              << " 字节, 占用 " << inode.block_count << " 块\n";
    return written;
}

std::string VirtualFS::my_read(const std::string& name) {
    int idx = findFile(name);
    if (idx < 0) {
        std::cout << "  [VFS] 文件 \"" << name << "\" 不存在\n";
        return "";
    }
    const Inode& inode = inodes_[idx];
    std::string result;
    result.reserve(inode.size);
    int remaining = inode.size;
    for (int i = 0; i < inode.block_count && remaining > 0; ++i) {
        int chunk = std::min(VFS_BLOCK_SIZE, remaining);
        result.append(data_blocks_[inode.blocks[i]], chunk);
        remaining -= chunk;
    }
    std::cout << "  [VFS] 读取 \"" << name << "\" 共 " << inode.size << " 字节\n";
    return result;
}

int VirtualFS::my_delete(const std::string& name) {
    int idx = findFile(name);
    if (idx < 0) {
        std::cout << "  [VFS] 文件 \"" << name << "\" 不存在\n";
        return -1;
    }
    Inode& inode = inodes_[idx];
    for (int i = 0; i < inode.block_count; ++i) {
        freeBlock(inode.blocks[i]);
    }
    inode.used = false;
    inode.size = 0;
    inode.block_count = 0;
    std::memset(inode.name, 0, VFS_MAX_NAME);
    std::cout << "  [VFS] 删除文件 \"" << name << "\"\n";
    return 0;
}

void VirtualFS::listFiles() const {
    std::cout << "  [VFS] 文件列表:\n";
    std::cout << "  " << std::left << std::setw(20) << "文件名"
              << std::setw(10) << "大小(字节)"
              << std::setw(10) << "块数" << "\n";
    std::cout << "  ─────────────────────────────────────\n";
    int count = 0;
    for (int i = 0; i < VFS_MAX_FILES; ++i) {
        if (inodes_[i].used) {
            std::cout << "  " << std::left << std::setw(20) << inodes_[i].name
                      << std::setw(10) << inodes_[i].size
                      << std::setw(10) << inodes_[i].block_count << "\n";
            ++count;
        }
    }
    if (count == 0) std::cout << "  (无文件)\n";
}

void VirtualFS::printDiskUsage() const {
    int used = 0;
    for (int i = 0; i < VFS_MAX_BLOCKS; ++i) {
        if (bitmap_[i]) ++used;
    }
    std::cout << "  [VFS] 磁盘使用: " << used << "/" << VFS_MAX_BLOCKS
              << " 块 (" << std::fixed << std::setprecision(1)
              << (used * 100.0 / VFS_MAX_BLOCKS) << "%)\n";
    std::cout << "  位图: ";
    for (int i = 0; i < VFS_MAX_BLOCKS; ++i) {
        std::cout << (bitmap_[i] ? "■" : "□");
        if ((i + 1) % 32 == 0) std::cout << "\n        ";
    }
    std::cout << "\n";
}

} // namespace uthread
