#include "pch.h"
#include <iostream>
#include <ctime>
#include <fstream>

//文件系统的数据结构及其相关操作
//硬盘上的"静态的"文件系统

//用当前系统下的一个文件FS.txt, 模拟一个FS_Size大小的硬盘.
//每次读写BlockSize大小的一块数据
class DiskSim
{
    std::fstream disk;//连接文件的流
    bool old = true;//由于文件系统首次启动和再次启动的行为可能不同, 所以需要保留,传递判断结果

    char buffer[BlockSize] = { 0 };//缓冲区
    u16 bufpos = 0;
    bool buf_dirty = true;

public:

    //(若无文件则)创建文件, 打开文件并连接到流
    DiskSim()
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
                std::cout << "Create disk(FS.txt) failed!\n";
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
            std::cout << "Failed to load the disk(FS.txt)!\n";
            exit(-1);
        }
    }

    //从硬盘读到缓冲区,如有指针再复制过去
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void read(u16 block_num, char* s = nullptr)
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

    //将缓冲区写入硬盘; 如有指针, 先从指针写到缓冲区
    //block_num是硬盘块号, 使用数据块号时注意+DataBlockOffset
    void write(u16 block_num, const char* s = nullptr)
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

    //返回缓冲区指针, 会置脏位为1
    //约定不允许存储此指针以后使用
    char* get_buf() {
        buf_dirty = true;
        return buffer;
    }

    //硬盘是否是新建的
    bool is_new() {
        return !old;
    }

    //数据全部清零
    void clear() {
        char zerob[BlockSize] = { 0 };
        for (int i = 0; i < FS_Size / BlockSize; i++)
        {
            disk.write(zerob, BlockSize);
        }
        std::cout << "Disk has been initialized with full 0!\n";
    }

    ~DiskSim()
    {
        disk.close();
    }
};

//位图块结构
class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };
    bool start;

public:
    BitMap(bool is_inode) {
        this->start = is_inode;
    }

    //置pos位为1
    void set_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] |= t << (pos % 64);
    }

    //重置pos位为0
    void reset_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] &= ~(t << (pos % 64));
    }

    //获取位图首地址,用于读写磁盘
    char* pointer() {
        return (char*)bits;
    }

    //取某一位的值
    bool get_bit(u16 pos) {
        pos -= start;
        u64 t = (u64)0x01 << (pos % 64);
        return bool(bits[pos / 64] & t);
    }

    //寻找空位: 从某一位(pos,含)向后寻找
    //若有连续的n位为空, 则返回这n位的首位位置, 并置这n位为1
    //若无连续n位, 则返回最大的连续空位数的相反数-1(避免0的二义性)
    //所谓"连续"是首尾循环的
    int find_zeros(u16 pos, u16 n) {
        if (n == 0)
            return pos;
        pos -= start;
        int curr = 0, max = 0;
        for (u16 i = pos; i < BlockSize * 8; i++)
        {
            if (this->get_bit(i)) {
                if (max < curr)
                    max = curr;
                curr = 0;
            }
            else {
                curr++;
                if (curr == n) {
                    for (int j = i - n + 1; j <= i; j++) {
                        this->set_bit(j);
                    }
                    return i - n + 1 + start;
                }
            }
        }
        for (u16 i = 0; i < pos; i++)
        {
            if (this->get_bit(i)) {
                if (max < curr)
                    max = curr;
                curr = 0;
            }
            else {
                curr++;
                if (curr == n) {
                    if (i + 1 >= n) {
                        for (int j = i - n + 1; j <= i; j++) {
                            this->set_bit(j);
                        }
                        return i - n + 1 + start;
                    }
                    else {
                        for (int j = i + BlockSize * 8 - n + 1; j !=i+1; j++) {
                            if (j == BlockSize * 8)
                                j = 0;
                            this->set_bit(j);
                        }
                        return i + BlockSize * 8 - n + 1 + start;
                    }
                }
            }
        }
        return (curr > max) ? -curr - 1 : -max - 1;
    }
};

//组描述符, 保存文件系统的基础信息
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
    char volume_name[16] = { 0 };//卷名
    u16 block_bitmap = 1;//数据块位图所在的磁盘块号
    u16 inode_bitmap = 2;//索引结点位图的磁盘块号
    u16 inode_table = 3;//索引结点表的起始磁盘块号

    //针对这几个值有一系列操作,如判零 自减等.封装它们很没意思,直接外部修改吧
    u16 free_blocks_count = BlockSize * 8;//空闲块的个数(指数据块)
    u16 free_inodes_count = BlockSize * 8;//空闲索引结点的个数
    u16 used_dirs_count = 0;//目录的个数

    char pad[4];//填充
    char remain_padding[480] = { 0 };//继续填充至512字节

    Group_Descriptor()
    {
        for (int i = 0; i < 4; i++)
        {
            pad[i] = (char)0xff;
        }
    }
};

//用于间接寻址的数据块内容
struct IndexBlock
{
    u16 index[BlockSize / 2] = { 0 };
};

//索引节点inode, 保存文件信息及位置
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//高8位是文件类型, 低八位是访问权限
    u16 i_blocks = 0;//文件占用的数据块个数
    u32 i_size = 0;//文件大小, 单位Byte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a访问, c创建, m修改, d删除时间
    u16 i_block[8] = { 0 };//文件所占用数据块的索引表(存储数据块号)
    char i_pad[8];//填充至64字节

    //perm=permission,没词用了
    Inode(char type = 1, char perm = 7)
    {
        i_mode = (type << 8) + perm;
        i_atime = i_ctime = i_mtime = time(NULL);
        for (int i = 0; i < 8; i++)
        {
            i_pad[i] = (char)0xff;
        }
    }
    //修改访问权限
    void perm(char p) {
        i_mode = (i_mode & (u16)0xff00) + p;
    }
    //读取时
    void access() {
        i_atime = time(NULL);
    }
    //修改时,size为文件新大小
    void modify(u32 size) {
        i_mtime = time(NULL);
        i_size = size;
    }
    //删除时
    void del() {
        i_dtime = time(NULL);
        i_mode = 0;
        i_blocks = 0;
        i_size = 0;
        i_atime = i_ctime = i_mtime = 0;
        for (int i = 0; i < 8; i++) {
            i_block[i] = 0;
        }
    }

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
        else if (index < 6 + BlockSize / 2 + BlockSize * BlockSize / 4) {
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

};

//目录文件内容中的一项
struct DirEntry
{
    u16 inode = 0;//此目录项对应文件的inode号, 非0则为正确的目录项
    u16 rec_len = 0;//此目录项的长度(不定长,至少7,可能大于实际长度,是为了在有空隙的情况下找到下一个目录项)
    char name_len = 0;//文件名长度(不包括\0)
    char file_type = 0;//文件类型
    char name[256] = { 0 };//文件名

    DirEntry(u16 index, std::string nm, char type) :inode(index), file_type(type) {
        u32 strl = (nm.size() > sizeof(name) - 1) ? sizeof(name) - 1 : nm.size();
        nm.copy(name, strl);
        rec_len = 7 + strl;
        name_len = strl;
    }

    DirEntry() {}

    //判断一段二进制字节流是否为正确存在的目录项
    //若此目录项inode为0, 返回false, rec_len不变
    //若此目录项inode为1, 返回true, rec_len置为此目录项的rec_len
    //使用时遵照请求调页的思想
    bool is_alive(char* head, u16& rec_len) {
        u16* test = (u16*)head;
        if (*test == 0)
            return false;
        rec_len = *(++test);
        return true;
    }

    //从二进制数据中建立结构, 返回可能存在的下一项的首地址
    //使用时需保证数据的正确性
    char* init(char* head) {
        u16* pt16 = (u16*)head;
        inode = *(pt16++);
        rec_len = *pt16;
        char* pt8 = head + 4;
        name_len = *(pt8++);
        file_type= *(pt8++);
        u16 i = 0;
        for (; i < name_len; i++) {
            name[i]= *(pt8++);
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

    //从指针head开始, 找到下一个目录项的指针
    //从head指向的字节算起, 若在max_len内无目录项, 则返回nullptr
    //max_len=buf_tail-head=可用长度
    //下一项至少要有inode和rec_len存在,便于指针移去之后的判断
    char* next_head(char* head, u64 max_len) {
        u16* p16 = (u16*)head;
        char* p8 = head;
        u64 len1 = *(p16 + 1);
        if (len1 > max_len - 4)
            return nullptr;
        p8 += len1;//p8->next_head
        return p8;
    }
};
