#pragma once

#include "pch.h"
#include "structures.cpp"
#include <string>
#include <utility>
#include <map>
#include <iostream>

class Dir;

class File {
protected:
    DiskSim* disk = nullptr;
    BitMap* inode_map = nullptr, * block_map = nullptr;
    Group_Descriptor* gd = nullptr;
    Dir* parent = nullptr;
    std::map<u16, File*>* fopen_table;

    u16 node_index = 0;
    Inode inode;
    std::string name = "";

    char* buffer = nullptr;
    u32 len = 0, buflen = 0;
    bool has_open = false, dirty = false, has_read = false;

    //读取index号对应的inode索引结构
    bool read_inode(u16 nodei);

    //写入此inode
    bool write_inode();

    //对某Inode节点node, 在索引号为node.i_blocks处, 添加一个数据块索引, block是数据块号
    //由于过程中可能会申请间接索引块,磁盘满后可能失败,故返回是否成功
    //若同时申请一二级间接索引块,二级失败, 会进行回退,避免1级索引块变为"死块"
    bool add_block(u16 block);

    //减去一个索引号为node.i_blocks的数据块索引
    bool sub_block();

    //对某Inode节点node, 用索引号index, 获取一个数据块号
    u16 get_block(u16 index);

public:
    //构造函数待定
    File(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot);

    /*新建文件时要做什么:
    MyExt2::inode_map找一个空的nodei并占用
    MyExt2::gd.free_inodes_count--
    (若是目录文件, MyExt2::gd.used_dirs_count++, 并设. ..两项)
    构建一个inode并写入磁盘
    在父目录添加一个目录项
    */
    bool create(std::string nm, Inode ino);
    /*
    打开文件要做什么:
    插入MyExt2::fopen_table
    从磁盘获取inode
    修改访问时间
    */
    bool open(std::string nm, u16 nodei, Dir* pa);

    /*
    读文件:
    将文件分散在各数据块的内容连接起来, 放在缓冲区里
    返回头指针和长度
    */
    std::pair<const char*, u32> read();
    /*
    写文件:
    输入字符指针和长度
    将字符流写入缓冲区
    */
    bool write(char* str, u32 strlen);

    bool append(char* str, u32 applen);

    bool change(char* str, u32 begin, u32 end);

    /*
    关闭:
    为写入缓冲区至硬盘, 在MyExt2::block_map, gd中申请/释放数据块
    将缓冲区写入各分散数据块中
    并改变inode.size, i_blocks, 修改时间等
    写入inode
    删除MyExt2::fopen_table
    */
    bool close();
    /*
    删除:
    释放MyExt2::inode_map, block_map, 修改gd
    重置此inode
    在父目录中删除此目录项
    */
    bool del();

    operator bool()const;

    bool print();
};

//管理DirEntry
class Dir :public File {
protected:
    //目录项指针
    int offset = 0, end = -1;
    DirEntry temp;
public:
    //对继承方法的修改
    bool create(std::string nm, Inode ino);

    bool del();
protected:
    //低级api
    bool ready();
    bool head();
    bool next();
    bool alive();
    DirEntry get_this();
    //change后如果原位不能存放,则可能移动其位置
    bool set_this(DirEntry de);
    bool del_this();
    bool _find(u16 nodei);
    bool _find(std::string nm);
public:
    Dir(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot);
    //更高级的api
    bool add(DirEntry de);
    bool remove(u16 nodei);
    bool remove(std::string nm);
    std::pair<bool, DirEntry> find(u16 nodei);
    std::pair<bool, DirEntry> find(std::string nm);
    bool change_de(std::string nm, DirEntry de);
    bool change_de(u16 nodei, DirEntry de);
    bool print();
    //void get_all() {}
};