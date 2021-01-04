#pragma once

#include "pch.h"
#include "structures.cpp"
#include <string>
#include <vector>
#include <utility>

class File {
protected:
    u16 node_index = 0;
    Inode inode;
    DiskSim* disk = nullptr;
    std::string name = "";
    std::vector<std::pair<char*, u32>> buf;

    //读取index号对应的inode索引结构
    bool read_inode(u16 index) {
        Inode* pt;
        if (index == 0) {
            std::cerr << "File::read_inode(u16 index=0)\n";
            return false;
        }
        char block[BlockSize] = { 0 };
        disk->read((index - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        inode = *pt;
        return true;
    }

    //写入此inode
    bool write_inode() {
        if (node_index == 0) {
            std::cerr << "void write_inode(Inode& inode, u16 index=0)\n";
            return false;
        }
        Inode* pt;
        char block[BlockSize] = { 0 };
        disk->read((node_index - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((node_index - 1) % InodePerBlock);
        *pt = inode;
        disk->write((node_index - 1) / InodePerBlock + 3, block);
        return true;
    }

public:
    //构造函数待定
    File(){}
    File(u16 node_num, DiskSim* diskp, std::string fname)
        :node_index(node_num), disk(diskp), name(fname) {
        if (!this->read_inode(node_num))
            node_index = 0;
    }
    /*新建文件时要做什么:
    MyExt2::inode_map找一个空的nodei并占用
    MyExt2::gd.free_inodes_count--
    (若是目录文件, MyExt2::gd.used_dirs_count++)
    构建一个inode并写入磁盘
    在父目录添加一个目录项
    */
    bool create(){}
    /*
    打开文件要做什么:
    插入MyExt2::fopen_table
    从缓存?或者磁盘获取inode
    */
    bool open(){}

    /*
    读文件:
    将文件分散在各数据块的内容连接起来, 放在缓冲区里
    返回头指针和长度
    */
    char* read(){}
    /*
    写文件:
    输入字符指针和长度
    将字符流写入缓冲区
    */
    bool write(){}
    /*
    追加内容:
    同上, 但需多次申请内存,形成多个指针,存于vector buf中
    */
    bool append(){}

    /*
    关闭:
    为写入缓冲区至硬盘, 在MyExt2::block_map, gd中申请/释放数据块
    将缓冲区写入各分散数据块中
    并改变inode.size等
    写入inode
    删除MyExt2::fopen_table
    */
    bool close(){}
    /*
    删除:
    释放MyExt2::inode_map, block_map, 修改gd
    重置此inode
    在父目录中删除此目录项
    */
    bool del(){}
    operator bool()const {
        return node_index == 0;
    }
};

class Dir :public File {
protected:
    //目录项指针
public:
    bool head(){}
    bool next(){}
    bool is_tail(){}
    DirEntry get_this(){}
    //change后如果原位不能存放,则可能移动其位置
    bool change(DirEntry& de) {}
    bool add(){}
    bool del_this(){}
};