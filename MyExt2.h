#pragma once

#include <map>
#include <string>
#include <regex>
#include "constants.h"
#include "log.h"
#include "structures.h"
#include "files.h"

struct Res;

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
    Res path2inode(std::string path, bool silent = false);

public:
    MyExt2();

    std::string curr_path() const;

    std::string volume_name();

    bool is_formatted();

    //格式化
    //数据全部清零, 重置控制字段, 并初始化根目录, 及其他杂项
    void format(std::string vn);

    void ls(std::string path);

    void cd(std::string path);

    void mkdir(std::string path);

    void create(std::string path);

    void rm(std::string path);

    void read(std::string path);

    void write(std::string path);

    void chmod(char mode, std::string path);

    ~MyExt2();
};

