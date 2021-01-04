#include "pch.h"
#include "structures.cpp"
#include <string>

class File {
    u16 node_index = 0;
    Inode inode = 0;
    DiskSim* disk = nullptr;
    std::string name = "";

    //获取index号对应的inode索引结构
    bool get_inode(u16 index) {
        Inode* pt;
        if (index == 0) {
            std::cerr << "File::get_inode(u16 index=0)\n";
            return false;
        }
        char block[BlockSize] = { 0 };
        disk->read((index - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        inode = *pt;
        return true;
    }

    //设置第index个inode
    void set_inode(Inode& inode, u16 index) {
        if (index == 0) {
            std::cout << "void set_inode(Inode& inode, u16 index=0)\n";
            return;
        }
        Inode* pt;
        char block[BlockSize] = { 0 };
        disk.read((index - 1) / InodePerBlock + gdcache.inode_table, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        *pt = inode;
        disk.write((index - 1) / InodePerBlock + gdcache.inode_table, block);
    }

public:
    File(){}
    File(u16 node_num, DiskSim* diskp, std::string fname)
        :node_index(node_num), disk(diskp), name(fname) {
        if (!this->get_inode(node_num))
            node_index = 0;
    }
    bool create(){}
    bool open(){}
    
    bool close(){}
    bool del(){}
    operator bool()const {
        return node_index == 0;
    }
};

class Dir :public File {
    bool head(){}
    bool next(){}
    DirEntry get_this(){}
    bool set(DirEntry& de){}
    bool add(){}
};