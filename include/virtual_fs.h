#pragma once

#include <string>
#include <vector>
#include <cstring>

namespace uthread {

constexpr int VFS_BLOCK_SIZE = 64;
constexpr int VFS_MAX_BLOCKS = 128;
constexpr int VFS_MAX_FILES = 32;
constexpr int VFS_MAX_NAME = 32;
constexpr int VFS_BLOCKS_PER_FILE = 8;

struct Inode {
    bool used;
    char name[VFS_MAX_NAME];
    int size;
    int blocks[VFS_BLOCKS_PER_FILE];
    int block_count;
};

class VirtualFS {
public:
    VirtualFS();
    ~VirtualFS() = default;

    // 创建文件
    int my_create(const std::string& name);
    // 写入文件
    int my_write(const std::string& name, const std::string& data);
    // 读取文件
    std::string my_read(const std::string& name);
    // 删除文件
    int my_delete(const std::string& name);

    // 列出所有文件
    void listFiles() const;
    // 打印磁盘使用情况
    void printDiskUsage() const;

private:
    int findFile(const std::string& name) const;
    int allocBlock();
    void freeBlock(int block_id);

    Inode inodes_[VFS_MAX_FILES];
    bool bitmap_[VFS_MAX_BLOCKS];
    char data_blocks_[VFS_MAX_BLOCKS][VFS_BLOCK_SIZE];
};

} // namespace uthread
