#include "pch.h"
#include "structures.cpp"
#include <set>

//文件系统的内存数据结构及操作
//"动态的"文件系统
class MyExt2
{
    DiskSim disk;
    std::set<u16> fopen_table;//文件打开表
    u16 last_alloc_inode = 0;//上次分配的索引结点号
    u16 last_alloc_block = 0;//上次分配的数据块号
    u16 current_dir = 1;//当前目录(索引结点）
    std::string current_path = "";//当前路径(字符串) 
    Group_Descriptor gdcache;//组描述符的内存缓存
public:
    //todo:初始化要做哪些事?
    MyExt2() {}

    std::string curr_path() const {
        return current_path;
    }
    void format(std::string vn = "MyExt2") {
        Group_Descriptor gd;
        vn.copy(gd.volume_name, sizeof(gd.volume_name));
        gdcache = gd;
        gdcache.used_dirs_count++;
        gdcache.free_blocks_count--;
        gdcache.free_inodes_count--;

        BitMap data_block_bitmap, inode_bitmap;
        data_block_bitmap.set_bit(0);
        inode_bitmap.set_bit(0);

        DirEntry root_de;
        root_de.file_type = 2;
        root_de.inode = 1;

        Inode root_inode;
        root_inode.i_mode = (2 << 8) + 7;
        root_inode.i_blocks = 1;
        root_inode.i_block[0] = 0;
        root_inode.i_ctime = root_inode.i_atime = root_inode.i_mtime = time(NULL);


        disk.write(0, (const char*)&gd);
        disk.write(1, (const char*)&data_block_bitmap);
        disk.write(2, (const char*)&inode_bitmap);
    }
    std::string volume_name() {
        return gdcache.volume_name;
    }
    ~MyExt2()
    {
        disk.write(0, (const char*)&gdcache);
    }
};
