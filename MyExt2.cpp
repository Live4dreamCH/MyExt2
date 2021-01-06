#pragma once

#include "pch.h"
#include "structures.cpp"
#include <map>
#include <string>
#include <regex>
#include "files.cpp"

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

    /*
    //对某Inode节点node, 在索引号为node.i_blocks处, 添加一个数据块索引, block是数据块号
    //由于过程中可能会申请间接索引块,磁盘满后可能失败,故返回是否成功
    //若同时申请一二级间接索引块,二级失败, 会进行回退,避免1级索引块变为"死块"
    bool add_block(Inode& node, u16 block) {
        u16 index = node.i_blocks;
        if (index < 6) {
            node.i_block[index] = block;
        }
        //这里假设间接索引块已经建立, 实际上不行,下同且有两次
        //比较复杂的是数据块号从0起始,如何判断间接索引块是否存在?可能需要利用0号必为根目录的目录内容来做/i_blocks限制+及时设置
        //可能在设置一个数据块索引的同时,需要再多次分配给路径上的索引块以可用块号,这需要块位图,所以应怎么做?
        //解决方法:只允许在末尾i_blocks添加一个索引,用到间接索引块时立即分配,用i_blocks来判断间接索引块是否已建立
        //因为需要用到dsk,bmp等MyExt2的成员,这个方法应该挪过去
        //并且分配间接索引块后也要修改组描述符 块位图等
        //已解决, 以上供纪念
        else if (index < 6 + BlockSize / 2) {
            IndexBlock i1;
            index -= 6;
            if (index == 0) {
                int ib = block_map.find_zeros(0, 1);
                if (ib < 0)
                    return false;
                gdcache.free_blocks_count--;
                node.i_block[6] = ib;
                i1.index[index] = block;
                disk.write(node.i_block[6] + DataBlockOffset, (char*)&i1);
            }
            else {
                disk.read(node.i_block[6] + DataBlockOffset, (char*)&i1);
                i1.index[index] = block;
                disk.write(node.i_block[6] + DataBlockOffset, (char*)&i1);
            }
        }
        else if(index < 6 + BlockSize / 2 + BlockSize * BlockSize / 4){
            IndexBlock i1, i2;
            index -= 6 + BlockSize / 2;
            if (index == 0) {
                int ib1 = block_map.find_zeros(0, 1);
                int ib2 = block_map.find_zeros(0, 1);
                if (ib1 < 0) {
                    return false;
                }
                else if (ib2 < 0) {
                    block_map.reset_bit(ib1);
                    return false;
                }
                gdcache.free_blocks_count += 2;
                node.i_block[7] = ib1;
                i1.index[0] = ib2;
                i2.index[0] = block;
                disk.write(node.i_block[7] + DataBlockOffset, (char*)&i1);
                disk.write(i1.index[0] + DataBlockOffset, (char*)&i2);
            }
            else if (index % (BlockSize / 2) == 0) {
                int ib2 = block_map.find_zeros(0, 1);
                if (ib2 < 0) {
                    return false;
                }
                gdcache.free_blocks_count--;
                disk.read(node.i_block[7] + DataBlockOffset, (char*)&i1);
                i1.index[index / (BlockSize / 2)] = ib2;
                i2.index[0] = block;
                disk.write(node.i_block[7] + DataBlockOffset, (char*)&i1);
                disk.write(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
            }
            else {
                disk.read(node.i_block[7] + DataBlockOffset, (char*)&i1);
                disk.read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
                i2.index[index % (BlockSize / 2)] = block;
                disk.write(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
            }
        }
        else {
            return false;
        }
        node.i_blocks++;
        return true;
    }

    //对某Inode节点node, 用索引号index, 获取一个数据块号
    u16 get_block(Inode& node, u16 index) {
        if (index > node.i_blocks) {
            std::cout << "index in i_node.get_block() out of range!please check\n";
            return 0;
        }
        if (index < 6) {
            return node.i_block[index];
        }
        else if (index < 6 + BlockSize / 2) {
            IndexBlock i1;
            index -= 6;
            disk.read(node.i_block[6] + DataBlockOffset, (char*)&i1);
            return i1.index[index];
        }
        else {
            IndexBlock i1, i2;
            index -= 6 + BlockSize / 2;
            disk.read(node.i_block[7] + DataBlockOffset, (char*)&i1);
            disk.read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
            return i2.index[index % (BlockSize / 2)];
        }
    }
    */

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
        //u16 res = 0;
        if (dir_index == 0) {
            std::cout << name << ":file not found\n";
            return 0;
        }
        Inode dir_inode = get_inode(dir_index);
        char buff[2 * BlockSize] = { 0 }, * head = buff;
        char* mid = head + BlockSize, * curr = head, * temp;
        DirEntry onebyone;
        bool last, live;
        //调页
        u16 i = 0, dbn, maxlen = 0, len = 0;
        while (i < dir_inode.i_blocks)
        {
            if (i == dir_inode.i_blocks - 1)
                last = true;

            dbn = get_block(dir_inode, i);
            if (i == 0)
                disk.read(dbn + DataBlockOffset, head);
            else if (i == 1)
                disk.read(dbn + DataBlockOffset, mid);
            else {
                for (int j = 0; j < BlockSize; j++) {
                    *(head + j) = *(mid + j);
                }
                curr -= BlockSize;
                disk.read(dbn + DataBlockOffset, mid);
            }
            maxlen += BlockSize;

            //目录项
            while (true)
            {
                live = onebyone.is_alive(curr, len);
                if (len > maxlen) {
                    break;
                }
                if (live) {
                    onebyone.init(curr);
                    if (name == onebyone.name) {
                        return onebyone.inode;
                    }
                }
                temp = onebyone.next_head(curr, maxlen);
                if (temp == nullptr)
                    break;
                else {
                    maxlen -= (temp - curr);
                    curr = temp;
                }
            }
            i++;
        }
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

    /*
    //获取index号对应的inode索引结构
    Inode get_inode(u16 index) {
        Inode res, *pt;
        if (index == 0) {
            std::cout << "Inode get_inode(u16 index=0)\n";
            return res;
        }
        char block[BlockSize] = { 0 };
        disk.read((index - 1) / InodePerBlock + gdcache.inode_table, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        res = *pt;
        return res;
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
    */

    //对目录, 列出其下的文件
    //对文件, 列出其文件属性
    void list(u16 index, std::string name) {
        Inode target = get_inode(index);
        u16 type = (target.i_mode >> 8) & 0x00ff;
        if (type == 2) {
            //todo:1
        }
        else {
            char rwx[4] = "---", d = '-';
            rwx[2] = (target.i_mode & (0x0001 << 0)) == 1 ? 'x' : '-';
            rwx[1] = (target.i_mode & (0x0001 << 1)) == 1 ? 'w' : '-';
            rwx[0] = (target.i_mode & (0x0001 << 2)) == 1 ? 'r' : '-';
            std::cout << d << rwx << ' ' << target.i_size << '\t' << ctime((time_t*)&(target.i_mtime)) << '\t' << name << '\n';
        }
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
        this->set_inode(root_inode, 1);

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
