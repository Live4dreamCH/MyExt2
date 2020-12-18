#include "pch.h"
#include <iostream>
#include <ctime>
#include <fstream>

//用当前系统下的一个文件, 模拟一个FS_Size大小的硬盘.
//每次读写BlockSize大小的一块数据
class DiskSim
{
    std::fstream disk;
    bool old = true;

public:
    char buffer[BlockSize] = { 0 };

    //创建文件, 打开文件并连接到流
    DiskSim()
    {
        std::fstream exist;
        exist.open("FS.txt", std::ios::in | std::ios::binary);
        if (!exist.is_open())
        {
            old = false;
            exist.clear();
            //exist.close();
            exist.open("FS.txt", std::ios::out | std::ios::binary);
            if (exist.is_open())
            {
                for (int i = 0; i < FS_Size / BlockSize; i++)
                {
                    exist.write(buffer, BlockSize);
                }
                std::cout << "Disk has been initialized with full 0!\n";
            }
            else
            {
                std::cout << "Create disk(FS.txt) failed!\n";
                exit(-1);
            }
        }
        exist.close();

        disk.open("FS.txt", std::ios::out | std::ios::in | std::ios::binary);
        if (disk.is_open()) {
            std::cout << "Disk loaded!\n";
        }
        else
        {
            std::cout << "Failed to load the disk(FS.txt)!\n";
            exit(-1);
        }
    }

    //从硬盘读到缓冲区,如有指针再复制过去
    void read(u16 block_num, char* s = nullptr)
    {
        disk.seekg(BlockSize * block_num);
        disk.read(buffer, BlockSize);
        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                s[i] = buffer[i];
            }
        }
    }

    //将缓冲区写入硬盘; 如有指针, 先从指针写到缓冲区
    void write(u16 block_num, const char* s = nullptr)
    {
        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                buffer[i] = s[i];
            }
        }
        disk.seekp(BlockSize * block_num);
        disk.write(buffer, BlockSize);
    }

    //void init() {
    //    disk.seekp(0);
    //    for (int i = 0; i < FS_Size; i++)
    //    {
    //        disk.put(char(0x0));
    //    }
    //    std::cout << "Disk has been initialized with full 0!\n";
    //}

    //硬盘是否是新建的
    bool is_new() {
        return !old;
    }

    ~DiskSim()
    {
        disk.close();
    }
};

class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };

public:
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
    u16 free_blocks_count;//空闲块的个数(指数据块?)
    u16 free_inodes_count;//空闲索引结点的个数
    u16 used_dirs_count;//目录的个数
    char pad[4];//填充
    char remain_padding[480] = { 0 };//继续填充至512字节

    Group_Descriptor(char name[], u16 fb = BlockSize * 8, u16 fi = BlockSize * 8, u16 dir = 0) :free_blocks_count(fb), free_inodes_count(fi), used_dirs_count(dir)
    {
        for (int i = 0; i < 4; i++)
        {
            pad[i] = (char)0xff;
        }
    }
};

//索引节点, 保存文件信息及位置
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//高8位是文件类型, 低八位是访问权限
    u16 i_blocks = 0;//文件占用的数据块个数
    u32 i_size = 0;//文件大小, 单位Byte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a访问, c创建, m修改, d删除时间
    u16 i_block[8] = { 0 };//文件所占用数据块的索引表(存储数据块号)
    char i_pad[8];//填充至64字节

    Inode(char type = 1, char perm = 7)
    {
        i_mode = (type << 8) + perm;
        i_atime = i_ctime = i_mtime = time(NULL);
        for (int i = 0; i < 8; i++)
        {
            i_pad[i] = (char)0xff;
        }
    }
    void perm(char p) {
        i_mode = (i_mode & (u16)0xff00) + p;
    }
    void access() {
        i_atime = time(NULL);
    }
    void modify(u32 size) {
        i_mtime = time(NULL);
        i_size = size;
        i_blocks = size / BlockSize;
        if (size % BlockSize)
            i_blocks++;
    }
    void set_index() {
        ;
    }
    u16 get_index() {
        ;
    }
};

//目录文件内容中的一项
struct DirEntry
{
    u16 inode = 0;
    u16 rec_len = 7;
    char name_len = 1;
    char file_type = 0;
    char name[256] = { 0 };
};

//用于间接寻址的数据块内容
struct IndexBlock
{
    u16 index[BlockSize / 2] = { 0 };
};
