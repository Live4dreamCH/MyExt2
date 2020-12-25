#pragma once

typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

constexpr u16 BlockSize = 512;//块大小
constexpr u16 InodeSize = 64;//inode大小
constexpr u64 FS_Size = BlockSize * (1 + 1 + 1 + InodeSize * 8 + BlockSize * 8);//整个文件系统所占磁盘大小
constexpr u16 InodeOffset = 2;
constexpr u16 DataBlockOffset = 1 + 1 + 1 + InodeSize * 8;

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块

/*todo:总
main中的指令用正则表达式来做吧
*/