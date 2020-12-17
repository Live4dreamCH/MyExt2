// MyExt2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// C++17

#include <iostream>
#include <fstream>
#include <set>
#include <string>

typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

constexpr u16 BlockSize = 512;//块大小
constexpr u16 InodeSize = 64;//inode大小
constexpr u64 FS_Size = BlockSize * (1 + 1 + 1 + InodeSize * BlockSize * 8 / BlockSize + BlockSize * 8);//整个文件系统所占磁盘大小

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块

//用当前系统下的一个文件, 模拟一个FS_Size大小的硬盘.
//每次读写BlockSize大小的一块数据
class SimDisk
{
    std::fstream disk;

public:
    SimDisk()
    {
        std::fstream exist;
        exist.open("FS.txt", std::ios::in | std::ios::binary);
        disk.open("FS.txt", std::ios::out | std::ios::in | std::ios::binary);
        if (!exist.is_open())
        {
            for (int i = 0; i < FS_Size; i++)
            {
                disk.put((char)0x00);
            }
            std::cout << "disk has been initialized with full 0!\n";
        }
        else
        {
            std::cout << "disk loaded!\n";
        }
        exist.close();
    }

    void read(u16 block_num, char* s)
    {
        disk.seekg(BlockSize * block_num);
        disk.read(s, BlockSize);
    }

    void write(u16 block_num, const char* s)
    {
        disk.seekp(BlockSize * block_num);
        disk.write(s, BlockSize);
    }

    ~SimDisk()
    {
        disk.close();
    }
};

class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };

    void set_bit(u64 pos) {
        u64 t = 0x01;
        bits[pos / 64] |= t << (pos % 64);
    }

    void reset_bit(u64 pos) {
        u64 t = 0x01;
        bits[pos / 64] &= ~(t << (pos % 64));
    }

    char* pointer() {
        return (char*)bits;
    }
    
};

//组描述符, 保存文件系统的基础信息
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
    char volume_name[16];//卷名
    u16 block_bitmap = 1;//数据块位图所在的磁盘块号
    u16 inode_bitmap = 2;//索引结点位图的磁盘块号
    u16 inode_table = 3;//索引结点表的起始磁盘块号
    u16 free_blocks_count = BlockSize * 8;//空闲块的个数(指数据块?)
    u16 free_inodes_count = BlockSize * 8;//空闲索引结点的个数
    u16 used_dirs_count = 0;//目录的个数
    char pad[4];//填充
    char remain_padding[480] = { 0 };//继续填充至512字节

    Group_Descriptor()
    {
        for (int i = 0; i < 4; i++)
        {
            pad[i] = (char)0xff;
        }
    }
};

struct Memory
{
    std::set<u16> fopen_table;//文件打开表
    u16 last_alloc_inode = 0;//上次分配的索引结点号
    u16 last_alloc_block = 0;//上次分配的数据块号
    u16 current_dir = 0;//当前目录(索引结点）
    std::string current_path = "/";//当前路径(字符串) 
    Group_Descriptor gdcache;
};

//索引节点, 保存文件信息及位置
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//高8位是文件类型, 低八位是访问权限
    u16 i_blocks = 0;//文件占用的数据块个数
    u32 i_size = 0;//文件大小, 单位Byte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a访问, c创建, m修改, d删除时间
    u16 i_block[8];//文件所占用数据块的索引表(存储数据块号)
    char i_pad[8];//填充至64字节
    
    Inode()
    {
        for (int i = 0; i < 8; i++)
        {
            i_block[i] = 0;
            i_pad[i] = (char)0xff;
        }
    }
};

int main()
{
    std::cout << sizeof(Group_Descriptor) << ' ' << sizeof(Inode) << ' ' << sizeof(BitMap);
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
