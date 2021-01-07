#pragma once

#include "pch.h"
#include "structures.cpp"
#include <map>
#include <string>
#include <regex>
#include "files.h"

//�ļ�ϵͳ���ڴ����ݽṹ������
//"��̬��"�ļ�ϵͳ
class MyExt2
{
    DiskSim disk;
    std::map<u16, File*> fopen_table;//�ļ��򿪱�
    u16 current_dir = 0;//��ǰĿ¼(������㣩
    std::string current_path = "";//��ǰ·��(�ַ���) 
    Group_Descriptor gdcache;//�����������ڴ滺��
    BitMap inode_map, block_map;//λͼ���ڴ滺��
    bool is_fmt;//�Ƿ��Ѹ�ʽ��

    //��һ��pathת��Ϊinode��, ��path����Ϊ�մ�
    //ʧ�ܷ���0, һ�������inode��
    //���ɹ�, nameֵΪ�ļ���
    u16 path2inode(std::string path, std::string& name) {
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        u16 index;
        if (it->str() == "") {
            index = 1;
        }
        else {
            index = current_dir;
        }
        while (it!=end)
        {
            name = it->str();
            index = this->name2inode(name, index);
            if (index == 0) {
                name = "";
                return 0;
            }
        }
        return index;
    }

    //��Ŀ¼dir_index��,Ѱ����Ϊname���ļ�, ��������inode��
    //�����п���һ��Ŀ¼�����������̿�, ��ʹ��2��������,�ﵽ"����"��Ч��
    //δ�ҵ�����0
    u16 name2inode(std::string name, u16 dir_index) {
        return 0;
    }

    //todo:��Ŀ¼dir_inode���½��ļ�name
    bool create_file_inode(char name[], u16 dir_inode) {
        ;
    }

    //todo:��һ���½��Ŀ��ļ��޸�ΪĿ¼�ļ�, ����ʼ��. ..����
    void file2dir(u16 inode, u16 parent_inode) {
        ;
    }
    void file2dir(Inode& inode, u16 parent) {
        ;
    }

    //��Ŀ¼, �г����µ��ļ�
    //���ļ�, �г����ļ�����
    void list(u16 index, std::string name) {
    }

public:
    MyExt2()
        :inode_map(true), block_map(false) {
        is_fmt = !disk.is_new();
        if (is_fmt) {
            disk.read(0, (char*)&gdcache);
            disk.read(gdcache.block_bitmap, block_map.pointer());
            disk.read(gdcache.inode_bitmap, inode_map.pointer());
            current_dir = 1;
            current_path = "/";
        }
    }

    std::string curr_path() const {
        return current_path;
    }

    std::string volume_name() {
        return gdcache.volume_name;
    }

    bool is_formatted() {
        return is_fmt;
    }

    //��ʽ��
    //����ȫ������, ���ÿ����ֶ�, ����ʼ����Ŀ¼, ����������
    void format(std::string vn) {
        disk.clear();

        Group_Descriptor gd;
        vn.copy(gd.volume_name, sizeof(gd.volume_name)-1);
        gdcache = gd;
        //gdcache.used_dirs_count++;
        //gdcache.free_blocks_count--;
        //gdcache.free_inodes_count--;
        BitMap db(false), in(true);
        block_map = db;
        inode_map = in;

        //�½���Ŀ¼
        Inode root_inode(2, 7);
        root_inode.i_blocks = 1;
        root_inode.i_size = 64 * 2;
        root_inode.i_block[0] = 0;
        //this->set_inode(root_inode, 1);
        char block[BlockSize] = { 0 };
        *((Inode*)block) = root_inode;
        disk.write(gdcache.inode_table, block);


        DirEntry p(1,".",2), pp(1, "..", 2);
        char temp[BlockSize] = { 0 }, * pt = temp;
        *((DirEntry*)pt) = p;
        pt += p.rec_len;
        *((DirEntry*)pt) = pp;
        disk.write(0 + DataBlockOffset, temp);

        current_dir = 1;
        current_path = "/";
        gdcache.free_blocks_count--;
        gdcache.free_inodes_count--;
        gdcache.used_dirs_count++;
        block_map.set_bit(0);
        inode_map.set_bit(1);

        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
        is_fmt = true;
    }

    //�ڵ�ǰĿ¼���½��ļ�name
    bool create_file(char name[]) {
        return this->create_file_inode(name, current_dir);
    }

    //��Ŀ¼path���½��ļ�name
    bool create_file(char name[], std::string path) {
        std::string dir;
        u16 inode = this->path2inode(path, dir);
        if (inode == 0)
            return false;
        return this->create_file_inode(name, inode);
    }

    void ls(std::string path) {
        std::string name;
        u16 in = path2inode(path, name);
        if (in == 0) {
            std::cout << "ls: cannot access \'" + path + "\': No such file or directory\n";
        }
        else {
            std::cout << path + ":\n";
            this->list(in, name);
        }
    }

    ~MyExt2()
    {
        for (auto it = fopen_table.begin(); it != fopen_table.end(); it++) {
            if (!it->second->close())
                l("close fail!may lose data!");
        }
        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
    }
};
