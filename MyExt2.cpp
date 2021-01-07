#pragma once

#include "pch.h"
#include "structures.cpp"
#include <map>
#include <string>
#include <regex>
#include "files.h"

struct Res
{
    bool succ = false;
    u16 nodei = 0;
    std::string name = "";
    u16 parent = 0;
    bool dir = false;
};

//文件系统的内存数据结构及操作
//"动态的"文件系统
class MyExt2
{
    DiskSim disk;
    std::map<u16, File*> fopen_table;//文件打开表
    u16 current_dir = 0;//当前目录(索引结点）
    std::string current_path = "";//当前路径(字符串) 
    Group_Descriptor gdcache;//组描述符的内存缓存
    BitMap inode_map, block_map;//位图的内存缓存
    bool is_fmt;//是否已格式化
    Dir* rootdir = nullptr, * parent = nullptr;
    File* file = nullptr;

    //将一个path转换为inode号, 此path不能为空串
    //失败返回0, 一个错误的inode号
    //若成功, name值为文件名
    Res path2inode(std::string path, std::string& name) {
        if (path[0] != '/')
            path = current_path + path;
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        Dir pp = *rootdir;
        Res re;
        *parent = *rootdir;
        bool is_dir = true;
        std::pair<bool, DirEntry> n;
        while (++it != end)
        {
            name = it->str();
            if (!is_dir) {
                l("path: " + name + " is not a dir!");
                return re;
            }
            parent->read();
            n = parent->find(name);
            if (!n.first) {
                l("path: no file named " + name);
                return re;
            }
            if (n.second.file_type != 2) {
                is_dir = false;
                file->open(name, n.second.inode, parent);
            }
            else {
                pp = *parent;
                parent->close();
                parent->open(name, n.second.inode, &pp);
            }
        }
        re.name = name;
        re.nodei = n.second.inode;
        if (is_dir)
            re.parent = pp.get_nodei();
        else
            re.parent = parent->get_nodei();
        re.dir = is_dir;
        re.succ = true;
        return re;
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
            rootdir = new Dir(&disk, &inode_map, &block_map, &gdcache, nullptr, &fopen_table);
            rootdir->open("/", 1, rootdir);
            parent = new Dir(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
            file = new Dir(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
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

    //格式化
    //数据全部清零, 重置控制字段, 并初始化根目录, 及其他杂项
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

        //新建根目录
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
        rootdir = new Dir(&disk, &inode_map, &block_map, &gdcache, nullptr, &fopen_table);
        rootdir->open("/", 1, rootdir);
        parent = new Dir(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
        file = new Dir(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
    }


    void ls(std::string path) {
        std::string name;
        Res in = path2inode(path, name);
        if (!in.succ) {
            std::cout << "ls: cannot access \'" + path + "\': No such file or directory\n";
        }
        else {
            std::cout << path + ":\n";
            if (in.dir) {
                parent->read();
                parent->print();
            }
            else {
                file->print();
            }
        }
    }

    ~MyExt2()
    {
        //auto it = fopen_table.begin();
        //while (it != fopen_table.end())
        //{
        //    if (it != fopen_table.end())
        //        it++;
        //}
        while (fopen_table.size() > 0) {
            auto it = fopen_table.begin();
            if (!it->second->close())
                l("close fail!may lose data!");
        }
        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
    }
};
