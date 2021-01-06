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

    char* buffer = nullptr;
    u32 len = 0, buflen = 0;
    bool has_open = false, dirty = false, has_read = false;

    //��ȡindex�Ŷ�Ӧ��inode�����ṹ
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

    //д���inode
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

    //��ĳInode�ڵ�node, ��������Ϊnode.i_blocks��, ���һ�����ݿ�����, block�����ݿ��
    //���ڹ����п��ܻ�������������,�����������ʧ��,�ʷ����Ƿ�ɹ�
    //��ͬʱ����һ�������������,����ʧ��, ����л���,����1���������Ϊ"����"
    bool add_block(u16 block) {
        if (this->has_open) {
            u16 index = this->inode.i_blocks;
            if (index < 6) {
                this->inode.i_block[index] = block;
            }
            //����������������Ѿ�����, ʵ���ϲ���,��ͬ��������
            //�Ƚϸ��ӵ������ݿ�Ŵ�0��ʼ,����жϼ���������Ƿ����?������Ҫ����0�ű�Ϊ��Ŀ¼��Ŀ¼��������/this->inode.i_blocks����+��ʱ����
            //����������һ�����ݿ�������ͬʱ,��Ҫ�ٶ�η����·���ϵ��������Կ��ÿ��,����Ҫ��λͼ,����Ӧ��ô��?
            //�������:ֻ������ĩβthis->inode.i_blocks���һ������,�õ����������ʱ��������,��this->inode.i_blocks���жϼ���������Ƿ��ѽ���
            //��Ϊ��Ҫ�õ�dsk,bmp��MyExt2�ĳ�Ա,�������Ӧ��Ų��ȥ
            //���ҷ������������ҲҪ�޸��������� ��λͼ��
            //�ѽ��, ���Ϲ�����
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

    //��ȥһ��������Ϊnode.i_blocks�����ݿ�����
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

    //��ĳInode�ڵ�node, ��������index, ��ȡһ�����ݿ��
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
    //���캯������
    File(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, std::string>* fot)
        :disk(dsk), inode_map(ino_map), block_map(blk_map), gd(gdc), parent(par), fopen_table(fot) {}

    /*�½��ļ�ʱҪ��ʲô:
    MyExt2::inode_map��һ���յ�nodei��ռ��
    MyExt2::gd.free_inodes_count--
    (����Ŀ¼�ļ�, MyExt2::gd.used_dirs_count++, ����. ..����)
    ����һ��inode��д�����
    �ڸ�Ŀ¼���һ��Ŀ¼��
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
    ���ļ�Ҫ��ʲô:
    ����MyExt2::fopen_table
    �Ӵ��̻�ȡinode
    �޸ķ���ʱ��
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
    ���ļ�:
    ���ļ���ɢ�ڸ����ݿ��������������, ���ڻ�������
    ����ͷָ��ͳ���
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
                this->dirty = false;
                this->has_read = true;
                return res;
            }
        }
        else {
            std::cerr << "read() not open yet\n";
            return { nullptr, 0 };
        }
    }
    /*
    д�ļ�:
    �����ַ�ָ��ͳ���
    ���ַ���д�뻺����
    */
    bool write(char* str, u32 strlen){
        if (this->has_open) {
            if (!this->has_read) {
                l("write() not read yet");
                return false;
            }
            if (strlen > this->buflen) {
                u32 new_buflen = max<u32>(strlen * 3 / 2, ceiling(strlen));
                if (this->buflen)
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
            if (!this->has_read) {
                l("append() not read yet");
                return false;
            }
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
                if (this->buflen)
                    delete this->buffer;
                this->buflen = new_buflen;
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
        if (!this->has_read) {
            l("change() not read yet");
            return false;
        }

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
    �ر�:
    Ϊд�뻺������Ӳ��, ��MyExt2::block_map, gd������/�ͷ����ݿ�
    ��������д�����ɢ���ݿ���
    ���ı�inode.size, i_blocks, �޸�ʱ���
    д��inode
    ɾ��MyExt2::fopen_table
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

                //д��
                char* curr = this->buffer;
                for (int i = 0; i < buf_blocks; i++) {
                    this->disk->write(this->get_block(i) + DataBlockOffset, curr);
                    curr += BlockSize;
                }
                this->dirty = false;
                this->inode.modify(this->len);
            }
            delete this->buffer;
            this->len = 0;
            this->buflen = 0;
            this->has_read = false;
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
    ɾ��:
    �ͷ�MyExt2::inode_map, block_map, �޸�gd
    ���ô�inode
    �ڸ�Ŀ¼��ɾ����Ŀ¼��
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
        this->parent->remove(this->node_index);
        return true;
    }
    operator bool()const {
        return !(this->node_index == 0 || this->node_index >= BlockSize * 8);
    }
};

class Dir :public File {
protected:
    //Ŀ¼��ָ��
    int offset = 0, end = -1, max = -1;
    DirEntry temp;
    u16 recl = 0;
    //��Ŀ¼��λ�õ�map buffer
    std::map<std::string, u16> name2nodei;
    std::map<u16, u32> nodei2pos;
public:
    //�Լ̳з������޸�
    bool create(std::string nm, Inode ino) {
        if(!File::create(nm, ino))
            return false;
        this->gd->used_dirs_count++;
        int blk;
        blk = this->block_map->find_zeros(0, 1);
        if (blk < 0) {
            l("create ./ ../ fail:no block!");
            return false;
        }
        DirEntry p(this->node_index, this->name, 2), pp(this->parent->node_index, this->parent->name, 2);
        char b[BlockSize] = { 0 };
        char* bp = b;
        *((DirEntry*)bp) = p;
        bp += p.rec_len;
        *((DirEntry*)bp) = pp;
        this->disk->write(blk + DataBlockOffset, b);
        return true;
    }

    bool del() {
        if (!File::del())
            return false;
        this->gd->used_dirs_count--;
        return true;
    }
public:
    //�ͼ�api
    bool ready() {
        if (!this->has_open) {
            l("dir not open yet!");
            return false;
        }
        if (!this->has_read) {
            l("dir not read yet!");
            return false;
        }
        return true;
    }
    bool head(){
        if (!this->ready())
            return false;
        this->offset = 0;
        if (!this->temp.is_alive(this->buffer, recl)) {
            l("a dir without ./");
            return false;
        }
        this->temp.init(this->buffer);
        this->name2nodei.insert({ this->temp.name, this->temp.inode });
        this->nodei2pos.insert({ this->temp.inode, 0 });
        if (max < 0)
            max = 0;
        return true;
    }
    bool next(){
        if (!this->ready())
            return false;
        char* n = this->temp.next_head(this->buffer + this->offset, (int)this->len - this->offset);
        if (n == nullptr) {
            this->end = this->offset;
            return false;
        }
        //todo:����
    }
    bool is_tail(){}
    DirEntry get_this(){}
    //change�����ԭλ���ܴ��,������ƶ���λ��
    bool set_this(DirEntry de) {}
    bool del_this(){}
public:
    Dir(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, std::string>* fot)
        :File(dsk, ino_map, blk_map, gdc, par, fot) {}
    //���߼���api
    bool add(DirEntry de) {}
    bool remove(u16 nodei){}
    bool remove(std::string nm) {}
    DirEntry find(u16 nodei) {}
    DirEntry find(std::string nm) {}
    bool change_de(std::string nm, DirEntry de){}
    bool change_de(u16 nodei, DirEntry de) {}
    void list() {}
    void get_all() {}

    //... ...
};