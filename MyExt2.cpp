#pragma once

#include "pch.h"
#include "structures.cpp"
#include <map>
#include <string>
#include <regex>
#include "files.h"

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

    //将一个path转换为inode号, 此path不能为空串
    //失败返回0, 一个错误的inode号
    //若成功, name值为文件名
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

    //在目录dir_index中,寻找名为name的文件, 并返回其inode号
    //由于有可能一个目录项横跨两个磁盘块, 故使用2个缓冲区,达到"连续"的效果
    //未找到返回0
    u16 name2inode(std::string name, u16 dir_index) {
        return 0;
    }

    //todo:在目录dir_inode下新建文件name
    bool create_file_inode(char name[], u16 dir_inode) {
        ;
    }

    //todo:将一个新建的空文件修改为目录文件, 并初始化. ..两项
    void file2dir(u16 inode, u16 parent_inode) {
        ;
    }
    void file2dir(Inode& inode, u16 parent) {
        ;
    }

    //对目录, 列出其下的文件
    //对文件, 列出其文件属性
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
    }

    //在当前目录下新建文件name
    bool create_file(char name[]) {
        return this->create_file_inode(name, current_dir);
    }

    //在目录path下新建文件name
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
