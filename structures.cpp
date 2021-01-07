#pragma once

#include "pch.h"
#include <iostream>
#include <ctime>
#include <fstream>
#include <utility>

//文件系统的数据结构及其相关操作
//硬盘上的"静态的"文件系统

//用当前系统下的一个文件FS.txt, 模拟一个FS_Size大小的硬盘.
//每次读写BlockSize大小的一块数据
class DiskSim
{
    std::fstream disk;//连接文件的流
    bool old = true;//由于文件系统首次启动和再次启动的行为可能不同, 所以需要保留,传递判断结果

    char buffer[BlockSize] = { 0 };//缓冲区
    u16 bufpos = 0;
    bool buf_dirty = true;

public:

    //(若无文件则)创建文件, 打开文件并连接到流
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
                std::cerr << "Create disk(FS.txt) failed!\n";
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
            std::cerr << "Failed to load the disk(FS.txt)!\n";
            exit(-1);
        }
    }

    //从硬盘读到缓冲区,如有指针再复制过去
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void read(u16 block_num, char* s = nullptr)
    {
        if (buf_dirty || block_num != bufpos)
        {
            disk.seekg((u64)BlockSize * block_num);
            disk.read(buffer, BlockSize);
            bufpos = block_num;
            buf_dirty = false;
        }

        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                s[i] = buffer[i];
            }
        }
    }

    //将缓冲区写入硬盘; 如有指针, 先从指针写到缓冲区
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void write(u16 block_num, const char* s = nullptr)
    {
        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                buffer[i] = s[i];
            }
            buf_dirty = true;
        }

        if (buf_dirty || block_num != bufpos)
        {
            disk.seekp((u64)BlockSize * block_num);
            disk.write(buffer, BlockSize);
            buf_dirty = false;
            bufpos = block_num;
        }
    }

    //硬盘是否是新建的
    bool is_new() {
        return !old;
    }

    //数据全部清零
    void clear() {
        char zerob[BlockSize] = { 0 };
        for (int i = 0; i < FS_Size / BlockSize; i++)
        {
            disk.write(zerob, BlockSize);
        }
        std::cout << "Disk has been initialized with full 0!\n";
    }

    ~DiskSim()
    {
        disk.close();
    }
};

//位图块结构
class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };
    bool start;

public:
    BitMap(bool is_inode) {
        this->start = is_inode;
    }

    //置pos位为1
    void set_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] |= t << (pos % 64);
    }

    //重置pos位为0
    void reset_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] &= ~(t << (pos % 64));
    }

    //获取位图首地址,用于读写磁盘
    char* pointer() {
        return (char*)bits;
    }

    //取某一位的值
    bool get_bit(u16 pos) {
        pos -= start;
        u64 t = (u64)0x01 << (pos % 64);
        return bool(bits[pos / 64] & t);
    }

    //寻找空位: 从某一位(pos,含)向后寻找
    //若有连续的n位为空, 则返回这n位的首位位置, 并置这n位为1
    //若无连续n位, 则返回最大的连续空位数的相反数-1(避免0的二义性)
    int find_zeros(u16 pos, u16 n) {
        if (n == 0)
            return pos;
        pos -= start;
        int curr = 0, max = 0;
        for (u16 i = pos; i < BlockSize * 8; i++)
        {
            if (this->get_bit(i)) {
                if (max < curr)
                    max = curr;
                curr = 0;
            }
            else {
                curr++;
                if (curr == n) {
                    for (int j = i - n + 1; j <= i; j++) {
                        this->set_bit(j);
                    }
                    return i - n + 1 + start;
                }
            }
        }
        curr = 0;
        for (u16 i = 0; i < pos; i++)
        {
            if (this->get_bit(i)) {
                if (max < curr)
                    max = curr;
                curr = 0;
            }
            else {
                curr++;
                if (curr == n) {
                    for (int j = i - n + 1; j <= i; j++) {
                        this->set_bit(j);
                    }
                    return i - n + 1 + start;
                }
            }
        }
        return (curr > max) ? -curr - 1 : -max - 1;
    }
};

//组描述符, 保存文件系统的基础信息
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
    char volume_name[16] = { 0 };//卷名
    u16 block_bitmap = 1;//数据块位图所在的磁盘块号
    u16 inode_bitmap = 2;//索引结点位图的磁盘块号
    u16 inode_table = 3;//索引结点表的起始磁盘块号

    //针对这几个值有一系列操作,如判零 自减等.封装它们很没意思,直接外部修改吧
    u16 free_blocks_count = BlockSize * 8;//空闲块的个数(指数据块)
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

//用于间接寻址的数据块内容
struct IndexBlock
{
    u16 index[BlockSize / 2] = { 0 };
};

//索引节点inode, 保存文件信息及位置
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//高8位是文件类型, 低八位是访问权限
    u16 i_blocks = 0;//文件占用的数据块个数
    u32 i_size = 0;//文件大小, 单位Byte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a访问, c创建, m修改, d删除时间
    u16 i_block[8] = { 0 };//文件所占用数据块的索引表(存储数据块号)
    char i_pad[8];//填充至64字节

    //perm=permission,没词用了
    Inode(char type = 1, char perm = 7)
    {
        i_mode = (type << 8) + perm;
        i_atime = i_ctime = i_mtime = time(NULL);
        for (int i = 0; i < 8; i++)
        {
            i_pad[i] = (char)0xff;
        }
    }
    //修改访问权限
    void set_access(char p) {
        i_mode = (i_mode & (u16)0xff00) + p;
    }
    char get_access() {
        return i_mode & (u16)0x00ff;
    }
    void set_type(char type) {
        i_mode = (i_mode & (u16)0x00ff) + (type << 8);
    }
    char get_type() {
        return i_mode & (u16)0xff00;
    }
    //读取时
    void access() {
        i_atime = time(NULL);
    }
    //修改时,size为文件新大小
    void modify(u32 size) {
        i_mtime = time(NULL);
        i_size = size;
    }
    //删除时
    void del() {
        i_dtime = time(NULL);
        i_mode = 0;
        i_blocks = 0;
        i_size = 0;
        i_atime = i_ctime = i_mtime = 0;
        for (int i = 0; i < 8; i++) {
            i_block[i] = 0;
        }
    }

    void print() {
        u16 type = (i_mode >> 8) & 0x00ff;
        char rwx[4] = "---", d = '-', buff[26];
        if (type == 2)
            d = 'd';
        rwx[2] = (i_mode & (0x0001 << 0)) == 1 ? 'x' : '-';
        rwx[1] = (i_mode & (0x0001 << 1)) == 1 ? 'w' : '-';
        rwx[0] = (i_mode & (0x0001 << 2)) == 1 ? 'r' : '-';
        ctime_s(buff, sizeof buff, (time_t*)&(i_mtime));
        buff[24] = 0;
        std::cout << d << rwx << ' ' << i_size << '\t' << buff;
    }
};

//目录文件内容中的一项
struct DirEntry
{
    u16 inode = 0;//此目录项对应文件的inode号, 非0则为正确的目录项
    u16 rec_len = 0;//此目录项的长度(不定长,至少7,可能大于实际长度,是为了在有空隙的情况下找到下一个目录项)
    char name_len = 0;//文件名长度(不包括\0)
    char file_type = 0;//文件类型
    char name[256] = { 0 };//文件名

    DirEntry(u16 nodei, std::string nm, char type)
        :inode(nodei), file_type(type) {
        u32 strl = (nm.size() > sizeof(name) - 1) ? sizeof(name) - 1 : nm.size();
        nm.copy(name, strl);
        rec_len = 7 + strl;
        name_len = strl;
    }

    DirEntry() {}

    //判断一段二进制字节流是否为正确存在的目录项
    //若此目录项inode为0, 返回false, rec_len不变
    //若此目录项inode为1, 返回true, rec_len置为此目录项的rec_len
    std::pair<bool, u16> is_alive(char* head) {
        u16* test = (u16*)head;
        if (*test == 0)
            return { false, *(++test) };
        return { true, *(++test) };
    }

    //从二进制数据中建立结构, 返回可能存在的下一项的首地址
    //使用时需保证数据的正确性
    char* init(char* head) {
        u16* pt16 = (u16*)head;
        inode = *(pt16++);
        rec_len = *pt16;
        char* pt8 = head + 4;
        name_len = *(pt8++);
        file_type = *(pt8++);
        u16 i = 0;
        for (; i < name_len; i++) {
            name[i] = *(pt8++);
        }
        name[i] = 0;
        return head + rec_len;
    }

    //获取从指针位置, 到下一个已用目录项之间的空隙的长度
    //要求指针指向一个已被删除的目录项首地址, 且其后仍有正确存在的目录项
    //同样提供最大长度, 
    //u64 gap_len(char* bits) {
    //    u64 sum_len = 0;
    //    u16 this_len = 0;
    //    while (!is_alive(bits)) {
    //        this_len = *(((u16*)bits) + 1);
    //        sum_len += this_len;
    //        bits += this_len;
    //    }
    //    return sum_len;
    //}

    //从指针head开始, 找到下一个目录项的指针
    //从head指向的字节算起, 若在max_len内无目录项, 则返回nullptr
    //max_len=buf_tail-head=可用长度
    //下一项至少要有inode和rec_len存在,便于指针移去之后的判断
    char* next_head(char* head, u64 max_len) {
        u16* p16 = (u16*)head;
        char* p8 = head;
        u64 len1 = *(p16 + 1);
        if (len1 > max_len - 4)
            return nullptr;
        p8 += len1;//p8->next_head
        return p8;
    }
};
