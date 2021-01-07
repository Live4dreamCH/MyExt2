#pragma once

#include "files.h"

class Dir;

template<class T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

u32 ceiling(u32 a) {
    return (a % BlockSize == 0) ? a : a - a % BlockSize + BlockSize;
}

bool File::read_inode(u16 nodei) {
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

bool File::write_inode() {
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

bool File::add_block(u16 block) {
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

bool File::sub_block() {
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

u16 File::get_block(u16 index) {
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

File::File(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot)
    :disk(dsk), inode_map(ino_map), block_map(blk_map), gd(gdc), parent(par), fopen_table(fot) {}

bool File::create(std::string nm, Inode ino) {
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

bool File::open(std::string nm, u16 nodei, Dir* pa) {
    if (nodei == 0 || nodei >= BlockSize * 8) {
        std::cerr << "bool open() out of range\n";
        return false;
    }
    this->fopen_table->insert({ nodei, this });
    if (!this->read_inode(nodei)) {
        std::cerr << "bool open() read_inode fail\n";
        return false;
    }
    has_open = true;
    this->node_index = nodei;
    this->name = nm;
    this->parent = pa;
    this->inode.access();
    return true;
}

std::pair<const char*, u32> File::read() {
    if (has_open) {
        if (this->dirty) {
            return { (const char*)this->buffer, this->len };
        }
        else {
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

bool File::write(char* str, u32 strlen) {
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

bool File::append(char* str, u32 applen) {
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

bool File::change(char* str, u32 begin, u32 end) {
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
        this->dirty = true;
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

bool File::close() {
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
        this->fopen_table->erase(this->node_index);
        return false;
    }
}

bool File::del() {
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

File::operator bool() const {
    return !(this->node_index == 0 || this->node_index >= BlockSize * 8);
}

bool File::print() {
    if (!this->has_open) {
        std::cerr << "bool File::print() not open yet!\n";
        return false;
    }
    this->inode.print();
    std::cout << '\n';
    return true;
}

bool Dir::create(std::string nm, Inode ino) {
    if (!File::create(nm, ino))
        return false;
    this->gd->used_dirs_count++;
    int blk;
    blk = this->block_map->find_zeros(0, 1);
    if (blk < 0) {
        l("create ./ ../ fail:no block!");
        return false;
    }
    DirEntry p(this->node_index, ".", 2), pp(this->parent->node_index, "..", 2);
    char b[BlockSize] = { 0 };
    char* bp = b;
    *((DirEntry*)bp) = p;
    bp += p.rec_len;
    *((DirEntry*)bp) = pp;
    this->disk->write(blk + DataBlockOffset, b);
    this->inode.i_size = p.rec_len + pp.rec_len;
    this->inode.i_blocks = 1;
    this->inode.i_block[0] = blk;
    this->write_inode();
    return true;
}

bool Dir::del() {
    if (!File::del())
        return false;
    this->gd->used_dirs_count--;
    return true;
}

bool Dir::ready() {
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

bool Dir::head() {
    if (!this->ready())
        return false;
    this->offset = 0;
    if (!this->temp.is_alive(this->buffer).first) {
        l("a dir without ./");
        return false;
    }
    return true;
}

bool Dir::next() {
    if (!this->ready())
        return false;
    if (end != -1 && offset == end)
        return false;
    while (true) {
        char* n = this->temp.next_head(this->buffer + this->offset, (u64)this->len - this->offset);
        if (n == nullptr) {
            this->end = this->offset;
            return false;
        }
        auto r = this->temp.is_alive(n);
        if (r.second < 7 || r.second>261) {
            this->end = this->offset;
            return false;
        }
        if (r.first) {
            this->offset = n - this->buffer;
            return true;
        }
    }
}

bool Dir::alive() {
    if (!this->ready())
        return false;
    auto r = temp.is_alive(buffer + offset);
    if (r.second < 7 || r.second>261) {
        return false;
    }
    return r.first;
}

DirEntry Dir::get_this() {
    temp.init(buffer + offset);
    return temp;
}

bool Dir::set_this(DirEntry de) {
    if (!this->ready())
        return false;
    temp.init(buffer + offset);
    if (de.rec_len <= temp.rec_len) {
        de.rec_len = temp.rec_len;
        change((char*)&de, offset, offset + de.rec_len);
        return true;
    }
    else {
        del_this();
        return add(de);
    }
}

bool Dir::del_this() {
    if (!this->ready())
        return false;
    int off = this->offset;
    if (next())
        offset = off;
    else
        end = -1;
    u16* nodi = (u16*)(buffer + offset);
    if (*nodi == node_index || *nodi == parent->node_index) {
        l("cannot remove ./ or ../");
        return false;
    }
    *nodi = 0;
    offset = 0;
    return true;
}

bool Dir::_find(u16 nodei) {
    if (!this->ready())
        return false;
    head();
    do {
        temp.init(buffer + offset);
        if (temp.inode == nodei) {
            return true;
        }
    } while (next());
    l("no such de to remove!");
    return false;
}

bool Dir::_find(std::string nm) {
    if (!this->ready())
        return false;
    head();
    do {
        temp.init(buffer + offset);
        if (nm == temp.name)
            return true;
    } while (next());
    l("no such de!");
    return false;
}

Dir::Dir(DiskSim* dsk, BitMap* ino_map, BitMap* blk_map, Group_Descriptor* gdc, Dir* par, std::map<u16, File*>* fot)
    :File(dsk, ino_map, blk_map, gdc, par, fot) {}

bool Dir::add(DirEntry de) {
    if (!this->ready())
        return false;

    if (end == -1) {
        while (next());
    }
    temp.init(buffer + end);
    return change((char*)&de, end + temp.rec_len, end + temp.rec_len + de.rec_len);
}

bool Dir::remove(u16 nodei) {
    if (!_find(nodei))
        return false;
    del_this();
    return true;
}

bool Dir::remove(std::string nm) {
    if (!_find(nm))
        return false;
    del_this();
    return true;
}

std::pair<bool, DirEntry> Dir::find(u16 nodei) {
    if (!_find(nodei))
        return { false, DirEntry() };
    return { true,get_this() };
}

std::pair<bool, DirEntry> Dir::find(std::string nm) {
    if (!_find(nm))
        return { false, DirEntry() };
    return { true,get_this() };
}

bool Dir::change_de(std::string nm, DirEntry de) {
    if (!_find(nm))
        return false;
    set_this(de);
    return true;
}

bool Dir::change_de(u16 nodei, DirEntry de) {
    if (!_find(nodei))
        return false;
    set_this(de);
    return true;
}

bool Dir::print() {
    if (!this->ready())
        return false;
    Inode store = inode;
    head();
    do {
        temp.init(buffer + offset);
        read_inode(temp.inode);
        inode.print();
        std::cout << '\t' << temp.name << '\n';
    } while (next());
    inode = store;
    return true;
}