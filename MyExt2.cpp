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
    std::string path = "";
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
    Res path2inode(std::string path) {
        if (path[0] != '/')
            path = current_path + path;
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        Dir pp = *rootdir;
        Res re;
        bool is_dir = true;
        std::pair<bool, DirEntry> n;
        std::string name;
        *parent = *rootdir;
        if (!parent->open("/", 1, rootdir)) {
            l("path: open fail!"); 
            return re;
        }
        if (++it == end) {
            name = "/";
            n.second.inode = 1;
        }
        while (it != end)
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
                if (!file->open(name, n.second.inode, parent)) {
                    l("path: open fail!");
                    return re;
                }
            }
            else {
                pp = *parent;
                parent->close();
                if (!parent->open(name, n.second.inode, &pp)) {
                    l("path: open fail!");
                    return re;
                }
            }
            ++it;
        }
        re.name = name;
        re.nodei = n.second.inode;
        if (is_dir) {
            if (path.back() != '/')
                path.push_back('/');
            re.parent = pp.get_nodei();
        }
        else {
            if (path.back() == '/')
                path.pop_back();
            re.parent = parent->get_nodei();
        }

        std::string old;
        std::regex again("/\\./");
        std::regex up("/[^/]*/\\.\\./");
        do {
            old = path;
            path = std::regex_replace(path, again, "/");
        } while (old != path);

        while (path.substr(0, 4) == "/../") {
            path.erase(1, 3);
        }
        do {
            old = path;
            path = std::regex_replace(path, up, "/");
        } while (old != path);
        re.path = path;

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
            file = new File(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
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
        fopen_table.clear();

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
        root_inode.i_size = 17;
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
        file = new File(&disk, &inode_map, &block_map, &gdcache, rootdir, &fopen_table);
    }

    void ls(std::string path) {
        Res in = path2inode(path);
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

    void cd(std::string path) {
        Res in = path2inode(path);
        if (!in.succ) {
            std::cout << "cd: cannot access \'" + path + "\': No such file or directory\n";
        }
        else {
            if (in.dir) {
                current_dir = in.nodei;
                current_path = in.path;
            }
            else {
                l("cd: \'" + path + "\': not a directory");
            }
        }
    }

    void mkdir(std::string path) {
        if (path2inode(path).succ) {
            l("mkdir: " + path + " already exist!");
            return;
        }
        if (path[0] != '/')
            path = current_path + path;
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        std::string name;
        while (it != end) {
            name = it->str();
            it++;
        }
        if (path.back() == '/') {
            path.pop_back();
        }
        path.resize(path.size() - name.size());
        Res in = path2inode(path);
        if (!in.succ) {
            l("mkdir: dir path fail!");
            return;
        }
        if (!in.dir) {
            l("mkdir: cannot mkdir under a file!");
            return;
        }
        parent->read();
        Dir mk(&disk, &inode_map, &block_map, &gdcache, parent, &fopen_table);
        Inode ino(2);
        mk.create(name, ino);
        //mk.close();
        parent->close();
    }

    void create(std::string path) {
        if (path2inode(path).succ) {
            l("create: " + path + " already exist!");
            return;
        }
        if (path[0] != '/')
            path = current_path + path;
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        std::string name;
        while (it != end) {
            name = it->str();
            it++;
        }
        if (path.back() == '/') {
            path.pop_back();
        }
        path.resize(path.size() - name.size());
        Res in = path2inode(path);
        if (!in.succ) {
            l("create: dir path fail!");
            return;
        }
        if (!in.dir) {
            l("create: cannot create under a file!");
            return;
        }

        parent->read();
        File mk(&disk, &inode_map, &block_map, &gdcache, parent, &fopen_table);
        Inode ino(1);
        mk.create(name, ino);
        //mk.close();
        parent->close();
    }

    void rm(std::string path) {
        Res full = path2inode(path);
        if (!full.succ) {
            l("rm: " + path + " did not exist!");
            return;
        }
        if (path[0] != '/')
            path = current_path + path;
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        std::string name;
        while (it != end) {
            name = it->str();
            it++;
        }
        if (path.back() == '/') {
            path.pop_back();
        }
        path.resize(path.size() - name.size());
        Res in = path2inode(path);
        if (!in.succ) {
            l("rm: dir path fail!");
            return;
        }
        if (!in.dir) {
            l("rm: cannot rm under a file!");
            return;
        }

        parent->read();
        if (!full.dir) {
            File ff(&disk, &inode_map, &block_map, &gdcache, parent, &fopen_table);
            ff.open(full.name, full.nodei, parent);
            ff.del();
        }
        else {
            Dir dir(&disk, &inode_map, &block_map, &gdcache, parent, &fopen_table);
            dir.open(full.name, full.nodei, parent);
            dir.del();
        }
        //mk.close();
        parent->close();
    }

    ~MyExt2()
    {
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
