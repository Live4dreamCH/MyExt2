#pragma once

typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

constexpr u16 BlockSize = 512;//块大小
constexpr u16 InodeSize = 64;//inode大小
constexpr u64 FS_Size = BlockSize * (1 + 1 + 1 + InodeSize * 8 + BlockSize * 8);//整个文件系统所占磁盘大小
constexpr u16 InodePerBlock = BlockSize / 64;
constexpr u16 DataBlockOffset = 1 + 1 + 1 + InodeSize * 8;



//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块
//实际编写中发现数据块号从0开始很不方便, 后来人建议从1开始

/*todo:总
已知缺陷:文件名中不能出现空格
完善注释
单元测试
日志:增加l的级别参数, 在使用l的各处增加参数; 在Main, 根据命令行参数, 初始化使用的级别; 在l内进行大小判断
使用Exception, 向上传递or处理问题
分离出命令行参数解析
除模板外, 声明与定义分离
*/