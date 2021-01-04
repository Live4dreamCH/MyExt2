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
        inode = *pt;
        return true;
    }

    //д���inode
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
    //���캯������
    File(){}
    File(u16 node_num, DiskSim* diskp, std::string fname)
        :node_index(node_num), disk(diskp), name(fname) {
        if (!this->read_inode(node_num))
            node_index = 0;
    }
    /*�½��ļ�ʱҪ��ʲô:
    MyExt2::inode_map��һ���յ�nodei��ռ��
    MyExt2::gd.free_inodes_count--
    (����Ŀ¼�ļ�, MyExt2::gd.used_dirs_count++)
    ����һ��inode��д�����
    �ڸ�Ŀ¼���һ��Ŀ¼��
    */
    bool create(){}
    /*
    ���ļ�Ҫ��ʲô:
    ����MyExt2::fopen_table
    �ӻ���?���ߴ��̻�ȡinode
    */
    bool open(){}

    /*
    ���ļ�:
    ���ļ���ɢ�ڸ����ݿ��������������, ���ڻ�������
    ����ͷָ��ͳ���
    */
    char* read(){}
    /*
    д�ļ�:
    �����ַ�ָ��ͳ���
    ���ַ���д�뻺����
    */
    bool write(){}
    /*
    ׷������:
    ͬ��, �����������ڴ�,�γɶ��ָ��,����vector buf��
    */
    bool append(){}

    /*
    �ر�:
    Ϊд�뻺������Ӳ��, ��MyExt2::block_map, gd������/�ͷ����ݿ�
    ��������д�����ɢ���ݿ���
    ���ı�inode.size��
    д��inode
    ɾ��MyExt2::fopen_table
    */
    bool close(){}
    /*
    ɾ��:
    �ͷ�MyExt2::inode_map, block_map, �޸�gd
    ���ô�inode
    �ڸ�Ŀ¼��ɾ����Ŀ¼��
    */
    bool del(){}
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
    bool change(DirEntry& de) {}
    bool add(){}
    bool del_this(){}
};