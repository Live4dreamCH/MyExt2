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

    //��ȡindex�Ŷ�Ӧ��inode�����ṹ
    bool read_inode(u16 nodei);

    //д���inode
    bool write_inode();

    //��ĳInode�ڵ�node, ��������Ϊnode.i_blocks��, ���һ�����ݿ�����, block�����ݿ��
    //���ڹ����п��ܻ�������������,�����������ʧ��,�ʷ����Ƿ�ɹ�
    //��ͬʱ����һ�������������,����ʧ��, ����л���,����1���������Ϊ"����"
    bool add_block(u16 block);

    //��ȥһ��������Ϊnode.i_blocks�����ݿ�����
    bool sub_block();

    //��ĳInode�ڵ�node, ��������index, ��ȡһ�����ݿ��
    u16 get_block(u16 index);

public:
    //���캯������
    File(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot);

    /*�½��ļ�ʱҪ��ʲô:
    MyExt2::inode_map��һ���յ�nodei��ռ��
    MyExt2::gd.free_inodes_count--
    (����Ŀ¼�ļ�, MyExt2::gd.used_dirs_count++, ����. ..����)
    ����һ��inode��д�����
    �ڸ�Ŀ¼���һ��Ŀ¼��
    */
    bool create(std::string nm, Inode ino);
    /*
    ���ļ�Ҫ��ʲô:
    ����MyExt2::fopen_table
    �Ӵ��̻�ȡinode
    �޸ķ���ʱ��
    */
    bool open(std::string nm, u16 nodei, Dir* pa);

    /*
    ���ļ�:
    ���ļ���ɢ�ڸ����ݿ��������������, ���ڻ�������
    ����ͷָ��ͳ���
    */
    std::pair<const char*, u32> read();
    /*
    д�ļ�:
    �����ַ�ָ��ͳ���
    ���ַ���д�뻺����
    */
    bool write(char* str, u32 strlen);

    bool append(char* str, u32 applen);

    bool change(char* str, u32 begin, u32 end);

    /*
    �ر�:
    Ϊд�뻺������Ӳ��, ��MyExt2::block_map, gd������/�ͷ����ݿ�
    ��������д�����ɢ���ݿ���
    ���ı�inode.size, i_blocks, �޸�ʱ���
    д��inode
    ɾ��MyExt2::fopen_table
    */
    bool close();
    /*
    ɾ��:
    �ͷ�MyExt2::inode_map, block_map, �޸�gd
    ���ô�inode
    �ڸ�Ŀ¼��ɾ����Ŀ¼��
    */
    bool del();

    operator bool()const;

    bool print();
};

//����DirEntry
class Dir :public File {
protected:
    //Ŀ¼��ָ��
    int offset = 0, end = -1;
    DirEntry temp;
public:
    //�Լ̳з������޸�
    bool create(std::string nm, Inode ino);

    bool del();
protected:
    //�ͼ�api
    bool ready();
    bool head();
    bool next();
    bool alive();
    DirEntry get_this();
    //change�����ԭλ���ܴ��,������ƶ���λ��
    bool set_this(DirEntry de);
    bool del_this();
    bool _find(u16 nodei);
    bool _find(std::string nm);
public:
    Dir(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot);
    //���߼���api
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