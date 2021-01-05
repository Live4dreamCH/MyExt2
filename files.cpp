#pragma once

#include "pch.h"
#include "structures.cpp"
#include <string>
//#include <vector>
#include <utility>
#include <map>
#include <iostream>

class Dir;

template<class T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

u32 ceiling(u32 a) {
    return (a % BlockSize == 0) ? a : a - a % BlockSize + BlockSize;
}

class File {
protected:
    DiskSim* disk = nullptr;
    BitMap* inode_map = nullptr, * block_map = nullptr;
    Group_Descriptor* gd = nullptr;
    Dir* parent = nullptr;
    std::map<u16, std::string>* fopen_table;

    u16 node_index = 0;
    Inode inode;
    std::string name = "";

    char* buffer;
    u32 len, buflen;
    bool has_open = false, dirty = false;

    //读取index号对应的inode索引结构
    bool read_inode(u16 nodei) {
        Inode* pt;
        if (nodei == 0 || nodei >= BlockSize * 8) {
            std::cerr << "File::read_inode(u16 nodei) out of range\n";
            return false;
        }
        if (!inode_map->get_bit(nodei)) {
            std::cerr << "File::read_inode(u16 nodei) inode not used\n";
            return false;
        }
        char block[BlockSize] = { 0 };
        disk->read((nodei - 1) / InodePerBlock + 3, block);
        pt = ((Inode*)block) + ((nodei - 1) % InodePerBlock);
        this->inode = *pt;
        return true;
    }

    //写入此inode
    bool write_inode() {
        if (this->node_index == 0 || this->node_index >= BlockSize * 8) {
            std::cerr << "void write_inode() out of range\n";
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

    //对某Inode节点node, 在索引号为node.i_blocks处, 添加一个数据块索引, block是数据块号
    //由于过程中可能会申请间接索引块,磁盘满后可能失败,故返回是否成功
    //若同时申请一二级间接索引块,二级失败, 会进行回退,避免1级索引块变为"死块"
    bool add_block(u16 block) {
        if (this->has_open) {
            u16 index = this->inode.i_blocks;
            if (index < 6) {
                this->inode.i_block[index] = block;
            }
            //这里假设间接索引块已经建立, 实际上不行,下同且有两次
            //比较复杂的是数据块号从0起始,如何判断间接索引块是否存在?可能需要利用0号必为根目录的目录内容来做/this->inode.i_blocks限制+及时设置
            //可能在设置一个数据块索引的同时,需要再多次分配给路径上的索引块以可用块号,这需要块位图,所以应怎么做?
            //解决方法:只允许在末尾this->inode.i_blocks添加一个索引,用到间接索引块时立即分配,用this->inode.i_blocks来判断间接索引块是否已建立
            //因为需要用到dsk,bmp等MyExt2的成员,这个方法应该挪过去
            //并且分配间接索引块后也要修改组描述符 块位图等
            //已解决, 以上供纪念
            else if (index < 6 + BlockSize / 2) {
                IndexBlock i1;
                index -= 6;
                if (index == 0) {
                    int ib = this->block_map->find_zeros(0, 1);
                    if (ib < 0)
                        return false;
                    this->gd->free_blocks_count--;
                    this->inode.i_block[6] = ib;
                    i1.index[index] = block;
                    this->disk->write(this->inode.i_block[6] + DataBlockOffset, (char*)&i1);
                }
                else {
                    this->disk->read(this->inode.i_block[6] + DataBlockOffset, (char*)&i1);
                    i1.index[index] = block;
                    this->disk->write(this->inode.i_block[6] + DataBlockOffset, (char*)&i1);
                }
            }
            else if (index < 6 + BlockSize / 2 + BlockSize * BlockSize / 4) {
                IndexBlock i1, i2;
                index -= 6 + BlockSize / 2;
                if (index == 0) {
                    int ib1 = this->block_map->find_zeros(0, 1);
                    int ib2 = this->block_map->find_zeros(0, 1);
                    if (ib1 < 0) {
                        return false;
                    }
                    else if (ib2 < 0) {
                        this->block_map->reset_bit(ib1);
                        return false;
                    }
                    this->gd->free_blocks_count -= 2;
                    this->inode.i_block[7] = ib1;
                    i1.index[0] = ib2;
                    i2.index[0] = block;
                    this->disk->write(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
                    this->disk->write(i1.index[0] + DataBlockOffset, (char*)&i2);
                }
                else if (index % (BlockSize / 2) == 0) {
                    int ib2 = this->block_map->find_zeros(0, 1);
                    if (ib2 < 0) {
                        return false;
                    }
                    this->gd->free_blocks_count--;
                    this->disk->read(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
                    i1.index[index / (BlockSize / 2)] = ib2;
                    i2.index[0] = block;
                    this->disk->write(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
                    this->disk->write(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
                }
                else {
                    this->disk->read(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
                    this->disk->read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
                    i2.index[index % (BlockSize / 2)] = block;
                    this->disk->write(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
                }
            }
            else {
                std::cerr << "add_block(u16 block) out of range\n";
                return false;
            }
            this->inode.i_blocks++;
            return true;
        }
        else {
            std::cerr << "add_block(u16 block) not open yet\n";
            return false;
        }
    }

    //减去一个索引号为node.i_blocks的数据块索引
    bool sub_block() {
        if (this->inode.i_blocks == 0) {
            std::cerr << "sub_block() no block already!\n";
            return false;
        }
        if (!this->has_open) {
            l("sub_block(u16 block) not open yet!");
            return false;
        }

        u16 index = this->inode.i_blocks - 1, blk;
        if (index < 6) {
            blk = this->inode.i_block[index];
        }
        else if (index < 6 + BlockSize / 2) {
            IndexBlock i1;
            index -= 6;
            this->disk->read(this->inode.i_block[6] + DataBlockOffset, (char*)&i1);
            blk = i1.index[index];

            if (index == 0) {
                this->block_map->reset_bit(this->inode.i_block[6]);
                this->gd->free_blocks_count++;
            }
        }
        else if (index < 6 + BlockSize / 2 + BlockSize * BlockSize / 4) {
            IndexBlock i1, i2;
            index -= 6 + BlockSize / 2;
            this->disk->read(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
            this->disk->read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
            blk = i2.index[index % (BlockSize / 2)];

            if (index == 0) {
                this->block_map->reset_bit(i1.index[index / (BlockSize / 2)]);
                this->block_map->reset_bit(this->inode.i_block[7]);
                this->gd->free_blocks_count += 2;
            }
            else if (index % (BlockSize / 2) == 0) {
                this->block_map->reset_bit(i1.index[index / (BlockSize / 2)]);
                this->gd->free_blocks_count++;
            }
        }
        else {
            std::cerr << "sub_block(u16 block) out of range!\n";
            return false;
        }
        this->block_map->reset_bit(blk);
        this->gd->free_blocks_count++;
        this->inode.i_blocks--;
        return true;
    }

    //对某Inode节点node, 用索引号index, 获取一个数据块号
    u16 get_block(u16 index) {
        if (has_open) {
            if (index > this->inode.i_blocks) {
                std::cerr << "index in i_node.get_block() out of range!please check\n";
                return 0;
            }
            if (index < 6) {
                return this->inode.i_block[index];
            }
            else if (index < 6 + BlockSize / 2) {
                IndexBlock i1;
                index -= 6;
                this->disk->read(this->inode.i_block[6] + DataBlockOffset, (char*)&i1);
                return i1.index[index];
            }
            else {
                IndexBlock i1, i2;
                index -= 6 + BlockSize / 2;
                this->disk->read(this->inode.i_block[7] + DataBlockOffset, (char*)&i1);
                this->disk->read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
                return i2.index[index % (BlockSize / 2)];
            }
        }
        else {
            std::cerr << "u16 get_block(u16 index) not open yet\n";
            return false;
        }
    }

public:
    //构造函数待定
    File(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, std::string>* fot)
        :disk(dsk), inode_map(ino_map), block_map(blk_map), gd(gdc), parent(par), fopen_table(fot) {}

    //File(u16 node_num, DiskSim* diskp, std::string fname)
    //    :node_index(node_num), disk(diskp), name(fname) {}

    /*新建文件时要做什么:
    MyExt2::inode_map找一个空的nodei并占用
    MyExt2::gd.free_inodes_count--
    (若是目录文件, MyExt2::gd.used_dirs_count++)
    构建一个inode并写入磁盘
    在父目录添加一个目录项
    */
    bool create(std::string nm, Inode ino){
        int nodei = this->inode_map->find_zeros(1, 1);
        if (nodei <= 0) {
            std::cerr << "inode full! create fail\n";
            return false;
        }
        this->node_index = nodei;
        this->gd->free_inodes_count--;
        this->inode = ino;
        this->name = nm;
        this->write_inode();
        DirEntry de(this->node_index, this->name, this->inode.get_type());
        return this->parent->add(de);
    }
    /*
    打开文件要做什么:
    插入MyExt2::fopen_table
    从磁盘获取inode
    修改访问时间
    */
    bool open(std::string nm, u16 nodei){
        if (this->node_index == 0 || this->node_index >= BlockSize * 8) {
            std::cerr << "bool open() out of range\n";
            return false;
        }
        this->fopen_table->insert({ nodei, nm });
        if (!this->read_inode(nodei)) {
            std::cerr << "bool open() read_inode fail\n";
            return false;
        }
        has_open = true;
        this->node_index = nodei;
        this->name = nm;
        this->inode.access();
        return true;
    }

    /*
    读文件:
    将文件分散在各数据块的内容连接起来, 放在缓冲区里
    返回头指针和长度
    */
    std::pair<const char*, u32> read(){
        if (has_open) {
            if (this->dirty) {
                return { (const char*)this->buffer, this->len };
            }
            else{
                this->len = this->inode.i_size;
                this->buflen = max<u32>(this->inode.i_blocks * BlockSize, this->len * 3 / 2);
                char* buf = new char[this->buflen];
                char* curr = buf;
                u16 block = 0;
                for (int i = 0; i < this->inode.i_blocks; i++) {
                    block = this->get_block(i);
                    this->disk->read(block + DataBlockOffset, curr);
                    curr += BlockSize;
                }
                std::pair<const char*, u32> res = { (const char*)buf, this->len };
                this->buffer = buf;
                dirty = false;
                return res;
            }
        }
        else {
            std::cerr << "read() not open yet\n";
            return { nullptr, 0 };
        }
    }
    /*
    写文件:
    输入字符指针和长度
    将字符流写入缓冲区
    */
    bool write(char* str, u32 strlen){
        if (has_open) {
            if (strlen > this->buflen) {
                u32 new_buflen = max<u32>(strlen * 3 / 2, ceiling(strlen));
                delete this->buffer;
                this->buffer = new char[new_buflen];
                this->buflen = new_buflen;
            }
            for (int i = 0; i < strlen; i++) {
                *(this->buffer + i) = *(str + i);
            }
            this->len = strlen;
            dirty = true;
            return true;
        }
        else {
            std::cerr << "write() not open yet\n";
            return false;
        }
    }

    bool append(char* str, u32 applen) {
        if (has_open) {
            u32 strlen = applen + this->len;
            if (strlen > this->buflen) {
                u32 new_buflen = max<u32>(strlen * 3 / 2, ceiling(strlen));
                char* new_buf = new char[new_buflen];
                for (int i = 0; i < this->len; i++) {
                    *(new_buf + i) = *(this->buffer + i);
                }
                for (int i = 0; i < applen; i++) {
                    *(new_buf + this->len + i) = *(str + i);
                }
                this->buflen = new_buflen;
                delete this->buffer;
                this->buffer = new_buf;
            }
            else {
                for (int i = 0; i < applen; i++) {
                    *(this->buffer + this->len + i) = *(str + i);
                }
            }
            this->len = strlen;
            dirty = true;
            return true;
        }
        else {
            std::cerr << "write() not open yet\n";
            return false;
        }
    }

    bool change(char* str, u32 begin, u32 end) {
        if (begin > end) {
            u32 temp = begin;
            begin = end;
            end = temp;
        }
        if (end <= this->len) {
            for (int i = 0; i < end - begin; i++) {
                *(this->buffer + begin + i) = *(str + i);
            }
            return true;
        }
        else if (begin == this->len) {
            if (!this->append(str, end - begin))
                return false;
            return true;
        }
        else if (begin > this->len) {
            char* zeros = new char[begin - this->len];
            for (int i = 0; i < begin - this->len; i++) {
                *(zeros + i) = 0;
            }
            if (!this->append(zeros, begin - this->len))
                return false;
            if (!this->append(str, end - begin))
                return false;
            return true;
        }
        else {
            if (!this->change(str, begin, this->len))
                return false;
            if (!this->append(str + this->len - begin, end - this->len))
                return false;
            return true;
        }
    }

    /*
    关闭:
    为写入缓冲区至硬盘, 在MyExt2::block_map, gd中申请/释放数据块
    将缓冲区写入各分散数据块中
    并改变inode.size, i_blocks, 修改时间等
    写入inode
    删除MyExt2::fopen_table
    */
    bool close(){
        if (this->has_open) {
            if (this->dirty) {
                u32 buf_blocks = ceiling(this->len) / BlockSize;
                if (this->gd->free_blocks_count + this->inode.i_blocks < buf_blocks) {
                    std::cerr << "bool File::close() no extra space!\n";
                    delete this->buffer;
                    return false;
                }

                if (buf_blocks > this->inode.i_blocks) {
                    int blk = 0, need = buf_blocks - this->inode.i_blocks;
                    u16 last_pos = this->get_block(this->inode.i_blocks - 1) + 1, query = need;
                    do {
                        blk = this->block_map->find_zeros(last_pos, query);
                        if (blk >= 0) {
                            for (int i = 0; i < query; i++) {
                                if (!this->add_block(i + blk)) {
                                    std::cerr << "add_block() failed\n";
                                    delete this->buffer;
                                    return false;
                                }
                            }
                            need -= query;
                            last_pos = blk + query;
                            query = (query > need) ? need : query;
                        }
                        else {
                            query = -blk - 1;
                        }
                    } while (need > 0);
                }
                else if (buf_blocks < this->inode.i_blocks) {
                    for (int i = 0; i < this->inode.i_blocks - buf_blocks; i++) {
                        this->sub_block();
                    }
                }

                if (this->inode.i_blocks != buf_blocks) {
                    l("bool file::close() did not set the right inode.i_blocks!");
                    delete this->buffer;
                    return false;
                }

                //写入
                char* curr = this->buffer;
                for (int i = 0; i < buf_blocks; i++) {
                    this->disk->write(this->get_block(i) + DataBlockOffset, curr);
                    curr += BlockSize;
                }
                this->dirty = false;
                this->inode.modify(this->len);
            }
            delete this->buffer;
            this->write_inode();
            this->fopen_table->erase(this->node_index);
            this->has_open = false;
            return true;
        }
        else {
            std::cerr << "bool File::close() not open yet!\n";
            return false;
        }
    }
    /*
    删除:
    释放MyExt2::inode_map, block_map, 修改gd
    重置此inode
    在父目录中删除此目录项
    */
    bool del(){
        if (!*this) {
            l("nodei out of range");
            return false;
        }
        if (this->has_open) {
            delete this->buffer;
            this->fopen_table->erase(this->node_index);
            this->has_open = false;
        }
        while (this->inode.i_blocks > 0) {
            this->sub_block();
        }
        this->inode_map->reset_bit(this->node_index);
        this->gd->free_inodes_count++;
        this->parent->remove(this->node_index);//todo:这波配和不是很好
        return true;
    }
    operator bool()const {
        return !(this->node_index == 0 || this->node_index >= BlockSize * 8);
    }
};

class Dir :public File {
protected:
    //todo:目录项指针
    //todo:对目录项位置的map buffer
public:
    bool head(){}
    bool next(){}
    bool is_tail(){}
    DirEntry get_this(){}
    //change后如果原位不能存放,则可能移动其位置
    bool set_this(DirEntry de) {}
    bool add(DirEntry de){}
    bool del_this(){}
public:
    //todo:更高级的api
    DirEntry find(u16 nodei) {}
    DirEntry find(std::string nm) {}
    void list(){}
    void get_all(){}
    //... ...
};