#pragma once

#include "pch.h"
#include "structures.cpp"
#include <string>
#include <vector>
#include <utility>
#include <map>

class Dir;

class File {
protected:
    u16 node_index = 0;
    Inode inode;
    DiskSim* disk = nullptr;
    std::string name = "";

    std::vector<std::pair<char*, u32> > buf;
    bool has_open = false;

    BitMap* inode_map=nullptr, * block_map = nullptr;
    Group_Descriptor* gd = nullptr;
    Dir* parent = nullptr;

    //��ȡindex�Ŷ�Ӧ��inode�����ṹ
    bool read_inode(u16 index) {
        Inode* pt;
        if (index == 0) {
            std::cerr << "File::read_inode(u16 index=0)\n";
            return false;
        }
        char block[BlockSize] = { 0 };
        disk->read((index - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        this->inode = *pt;
        return true;
    }

    //д���inode
    bool write_inode() {
        if (this->node_index == 0) {
            std::cerr << "void write_inode(Inode& inode, u16 index=0)\n";
            return false;
        }
        Inode* pt;
        char block[BlockSize] = { 0 };
        disk->read((this->node_index - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((this->node_index - 1) % InodePerBlock);
        *pt = this->inode;
        disk->write((this->node_index - 1) / InodePerBlock + 3, block);
        return true;
    }

public:
    //���캯������
    File(BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par)
    :inode_map(ino_map), block_map(blk_map), gd(gdc), parent(par){}

    //File(u16 node_num, DiskSim* diskp, std::string fname)
    //    :node_index(node_num), disk(diskp), name(fname) {}

    /*�½��ļ�ʱҪ��ʲô:
    MyExt2::inode_map��һ���յ�nodei��ռ��
    MyExt2::gd.free_inodes_count--
    (����Ŀ¼�ļ�, MyExt2::gd.used_dirs_count++)
    ����һ��inode��д�����
    �ڸ�Ŀ¼���һ��Ŀ¼��
    */
    bool create(std::string nm, Inode ino){
        int nodei = this->inode_map->find_zeros(1, 1);
        if (nodei < 0) {
            std::cerr << "inode full! create fail\n";
            return false;
        }
        this->node_index = nodei;
        this->gd->free_inodes_count--;
        this->inode = ino;
        this->write_inode();
        DirEntry de(this->node_index, this->name, this->inode.get_type());
        this->parent->add(de);
    }
    /*
    ���ļ�Ҫ��ʲô:
    ����MyExt2::fopen_table
    �Ӵ��̻�ȡinode
    �޸ķ���ʱ��
    */
    bool open(std::map<u16, std::string>& fopen_table){
        ;
    }

    /*
    ���ļ�:
    ���ļ���ɢ�ڸ����ݿ��������������, ���ڻ�������
    ����ͷָ��ͳ���
    */
    std::pair<char*, u32> read(){
        ;
    }
    /*
    д�ļ�:
    �����ַ�ָ��ͳ���
    ���ַ���д�뻺����
    */
    bool write(char* str, u32 len){
        ;
    }
    /*
    ׷������:
    ͬ��, �����������ڴ�,�γɶ��ָ��,����vector buf��
    */
    bool append(char* str, u32 len){
        ;
    }

    /*
    �ر�:
    Ϊд�뻺������Ӳ��, ��MyExt2::block_map, gd������/�ͷ����ݿ�
    ��������д�����ɢ���ݿ���
    ���ı�inode.size, �޸�ʱ��,����ʱ���
    д��inode
    ɾ��MyExt2::fopen_table
    */
    bool close(std::map<u16, std::string>& fopen_table){
        ;
    }
    /*
    ɾ��:
    �ͷ�MyExt2::inode_map, block_map, �޸�gd
    ���ô�inode
    �ڸ�Ŀ¼��ɾ����Ŀ¼��
    */
    bool del(){
        ;
    }
    operator bool()const {
        return node_index == 0;
    }
};

class Dir :public File {
protected:
    //Ŀ¼��ָ��
public:
    bool head(){}
    bool next(){}
    bool is_tail(){}
    DirEntry get_this(){}
    //change�����ԭλ���ܴ��,������ƶ���λ��
    bool change(DirEntry de) {}
    bool add(DirEntry de){}
    bool del_this(){}
};