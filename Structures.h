#pragma once

#include "constants.h"
#include "log.h"
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
    DiskSim();

    //从硬盘读到缓冲区,如有指针再复制过去
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void read(u16 block_num, char* s = nullptr);

    //将缓冲区写入硬盘; 如有指针, 先从指针写到缓冲区
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void write(u16 block_num, const char* s = nullptr);

    //硬盘是否是新建的
    bool is_new();

    //数据全部清零
    void clear();

    ~DiskSim();
};

//位图块结构
class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };
    bool start;

public:
    BitMap(bool is_inode);

    //置pos位为1
    void set_bit(u16 pos);

    //重置pos位为0
    void reset_bit(u16 pos);

    //获取位图首地址,用于读写磁盘
    char* pointer();

    //取某一位的值
    bool get_bit(u16 pos);

    //寻找空位: 从某一位(pos,含)向后寻找
    //若有连续的n位为空, 则返回这n位的首位位置, 并置这n位为1
    //若无连续n位, 则返回最大的连续空位数的相反数-1(避免0的二义性)
    int find_zeros(u16 pos, u16 n);
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

    Group_Descriptor();
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
    Inode(char type = 1, char perm = 7);

    //修改访问权限
    void set_access(char p);

    char get_access();

    void set_type(char type);

    char get_type();

    //读取时
    void access();

    //修改时,size为文件新大小
    void modify(u32 size);

    //删除时
    void del();

    void print();
};

//目录文件内容中的一项
struct DirEntry
{
    u16 inode = 0;//此目录项对应文件的inode号, 非0则为正确的目录项
    u16 rec_len = 0;//此目录项的长度(不定长,至少7,可能大于实际长度,是为了在有空隙的情况下找到下一个目录项)
    char name_len = 0;//文件名长度(不包括\0)
    char file_type = 0;//文件类型
    char name[256] = { 0 };//文件名

    DirEntry(u16 nodei, std::string nm, char type);

    DirEntry();

    //判断一段二进制字节流是否为正确存在的目录项
    //若此目录项inode为0, 返回false, rec_len不变
    //若此目录项inode为1, 返回true, rec_len置为此目录项的rec_len
    std::pair<bool, u16> is_alive(char* head);

    //从二进制数据中建立结构, 返回可能存在的下一项的首地址
    //使用时需保证数据的正确性
    char* init(char* head);


    //从指针head开始, 找到下一个目录项的指针
    //从head指向的字节算起, 若在max_len内无目录项, 则返回nullptr
    //max_len=buf_tail-head=可用长度
    //下一项至少要有inode和rec_len存在,便于指针移去之后的判断
    char* next_head(char* head, u64 max_len);
};
