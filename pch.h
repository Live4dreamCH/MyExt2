#pragma once

#include <iostream>
#include <string>

typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

constexpr u16 BlockSize = 512;//块大小
constexpr u16 InodeSize = 64;//inode大小
constexpr u64 FS_Size = BlockSize * (1 + 1 + 1 + InodeSize * 8 + BlockSize * 8);//整个文件系统所占磁盘大小
constexpr u16 InodePerBlock = BlockSize / 64;
constexpr u16 DataBlockOffset = 1 + 1 + 1 + InodeSize * 8;


void l(std::string log) {
    std::cerr << log << '\n';
}

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块
//实际编写中发现数据块号从0开始很不方便, 后来人建议从1开始

/*todo:总
已知缺陷:文件名中不能出现空格
担心哪里没有改gd.free_x和gd.used_dirs_count, 以及inode和相应的两种map, 后期应检查
补足注释
写完,清空无用代码注释
权限控制
*/