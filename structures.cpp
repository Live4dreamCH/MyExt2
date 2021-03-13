#pragma once

#include "Structures.h"

DiskSim::DiskSim()
{
    std::fstream exist;
    exist.open("FS.txt", std::ios::in | std::ios::binary);
    if (!exist.is_open())
    {
        old = false;
        exist.clear();
        //exist.close();
        exist.open("FS.txt", std::ios::out | std::ios::binary);
        if (exist.is_open())
        {
            for (int i = 0; i < FS_Size / BlockSize; i++)
            {
                exist.write(buffer, BlockSize);
            }
            std::cout << "Disk has been initialized with full 0!\n";
        }
        else
        {
            std::cerr << "Create disk(FS.txt) failed!\n";
            exit(-1);
        }
    }
    exist.close();

    disk.open("FS.txt", std::ios::out | std::ios::in | std::ios::binary);
    if (disk.is_open()) {
        std::cout << "Disk loaded!\n";
    }
    else
    {
        std::cerr << "Failed to load the disk(FS.txt)!\n";
        exit(-1);
    }
}

void DiskSim::read(u16 block_num, char* s)
{
    if (buf_dirty || block_num != bufpos)
    {
        disk.seekg((u64)BlockSize * block_num);
        disk.read(buffer, BlockSize);
        bufpos = block_num;
        buf_dirty = false;
    }

    if (s != nullptr) {
        for (int i = 0; i < BlockSize; i++)
        {
            s[i] = buffer[i];
        }
    }
}

void DiskSim::write(u16 block_num, const char* s)
{
    if (s != nullptr) {
        for (int i = 0; i < BlockSize; i++)
        {
            buffer[i] = s[i];
        }
        buf_dirty = true;
    }

    if (buf_dirty || block_num != bufpos)
    {
        disk.seekp((u64)BlockSize * block_num);
        disk.write(buffer, BlockSize);
        buf_dirty = false;
        bufpos = block_num;
    }
}

bool DiskSim::is_new() {
    return !old;
}

void DiskSim::clear() {
    char zerob[BlockSize] = { 0 };
    for (int i = 0; i < FS_Size / BlockSize; i++)
    {
        disk.seekp(0);
        disk.write(zerob, BlockSize);
    }
    std::cout << "Disk has been initialized with full 0!\n";
}

DiskSim::~DiskSim()
{
    disk.close();
}


BitMap::BitMap(bool is_inode) {
    this->start = is_inode;
}

void BitMap::set_bit(u16 pos) {
    pos -= start;
    if (pos >= 4096)
        l("-1 for uint!");
    u64 t = 0x01;
    bits[pos / 64] |= t << (pos % 64);
}

void BitMap::reset_bit(u16 pos) {
    pos -= start;
    if (pos >= 4096)
        l("-1 for uint!");
    u64 t = 0x01;
    bits[pos / 64] &= ~(t << (pos % 64));
}

char* BitMap::pointer() {
    return (char*)bits;
}

bool BitMap::get_bit(u16 pos) {
    pos -= start;
    if (pos >= 4096)
        l("-1 for uint!");
    u64 t = (u64)0x01 << (pos % 64);
    return bool(bits[pos / 64] & t);
}

int BitMap::find_zeros(u16 pos, u16 n) {
    if (n == 0)
        return pos;
    pos -= start;
    if (pos == 65535)
        l("-1 for uint!");
    int curr = 0, max = 0;
    for (u16 i = pos; i < BlockSize * 8; i++)
    {
        if (this->get_bit(i + start)) {
            if (max < curr)
                max = curr;
            curr = 0;
        }
        else {
            curr++;
            if (curr == n) {
                for (int j = i - n + 1; j <= i; j++) {
                    this->set_bit(j + start);
                }
                return i - n + 1 + start;
            }
        }
    }
    curr = 0;
    for (u16 i = 0; i < pos; i++)
    {
        if (this->get_bit(i + start)) {
            if (max < curr)
                max = curr;
            curr = 0;
        }
        else {
            curr++;
            if (curr == n) {
                for (int j = i - n + 1; j <= i; j++) {
                    this->set_bit(j + start);
                }
                return i - n + 1 + start;
            }
        }
    }
    return (curr > max) ? -curr - 1 : -max - 1;
}


Group_Descriptor::Group_Descriptor()
{
    for (int i = 0; i < 4; i++)
    {
        pad[i] = (char)0xff;
    }
}

Inode::Inode(char type, char perm)
{
    i_mode = (type << 8) + perm;
    i_atime = i_ctime = i_mtime = time(NULL);
    for (int i = 0; i < 8; i++)
    {
        i_pad[i] = (char)0xff;
    }
}

void Inode::set_access(char p) {
    i_mode = (i_mode & (u16)0xff00) + p;
}

char Inode::get_access() {
    return i_mode & (u16)0x00ff;
}

void Inode::set_type(char type) {
    i_mode = (i_mode & (u16)0x00ff) + (type << 8);
}

char Inode::get_type() {
    return (i_mode & (u16)0xff00) >> 8;
}

void Inode::access() {
    i_atime = time(NULL);
}

void Inode::modify(u32 size) {
    i_mtime = time(NULL);
    i_size = size;
}

void Inode::del() {
    i_dtime = time(NULL);
    i_mode = 0;
    i_blocks = 0;
    i_size = 0;
    i_atime = i_ctime = i_mtime = 0;
    for (int i = 0; i < 8; i++) {
        i_block[i] = 0;
    }
}

void Inode::print() {
    u16 type = (i_mode >> 8) & 0x00ff;
    char rwx[4] = "---", d = '-', buff[26];
    if (type == 2)
        d = 'd';
    rwx[2] = (i_mode & (0x0001 << 0)) > 0 ? 'x' : '-';
    rwx[1] = (i_mode & (0x0001 << 1)) > 0 ? 'w' : '-';
    rwx[0] = (i_mode & (0x0001 << 2)) > 0 ? 'r' : '-';
    ctime_s(buff, sizeof buff, (time_t*)&(i_mtime));
    buff[24] = 0;
    std::cout << d << rwx << rwx << rwx << ' ' << i_size << '\t' << buff;
}


DirEntry::DirEntry(u16 nodei, std::string nm, char type)
    :inode(nodei), file_type(type) {
    u32 strl = (nm.size() > sizeof(name) - 1) ? sizeof(name) - 1 : nm.size();
    nm.copy(name, strl);
    rec_len = 7 + strl;
    name_len = strl;
}

DirEntry::DirEntry() {}

std::pair<bool, u16> DirEntry::is_alive(char* head) {
    u16* test = (u16*)head;
    if (*test == 0)
        return { false, *(++test) };
    return { true, *(++test) };
}

char* DirEntry::init(char* head) {
    u16* pt16 = (u16*)head;
    inode = *(pt16++);
    rec_len = *pt16;
    char* pt8 = head + 4;
    name_len = *(pt8++);
    file_type = *(pt8++);
    u16 i = 0;
    for (; i < name_len; i++) {
        name[i] = *(pt8++);
    }
    name[i] = 0;
    return head + rec_len;
}

//获取从指针位置, 到下一个已用目录项之间的空隙的长度
//要求指针指向一个已被删除的目录项首地址, 且其后仍有正确存在的目录项
//同样提供最大长度, 
//u64 gap_len(char* bits) {
//    u64 sum_len = 0;
//    u16 this_len = 0;
//    while (!is_alive(bits)) {
//        this_len = *(((u16*)bits) + 1);
//        sum_len += this_len;
//        bits += this_len;
//    }
//    return sum_len;
//}

char* DirEntry::next_head(char* head, u64 max_len) {
    u16* p16 = (u16*)head;
    char* p8 = head;
    u64 len1 = *(p16 + 1);
    if (len1 > max_len - 4)
        return nullptr;
    p8 += len1;//p8->next_head
    return p8;
}
