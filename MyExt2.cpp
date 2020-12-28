#include "pch.h"
#include "structures.cpp"
#include <set>
#include <string>
#include <regex>

//文件系统的内存数据结构及操作
//"动态的"文件系统
class MyExt2
{
    DiskSim disk;
    std::set<u16> fopen_table;//文件打开表
    //u16 last_alloc_inode = 0;//上次分配的索引结点号
    //u16 last_alloc_block = 0;//上次分配的数据块号
    u16 current_dir = 0;//当前目录(索引结点）
    std::string current_path = "";//当前路径(字符串) 
    Group_Descriptor gdcache;//组描述符的内存缓存
    BitMap inode_map, block_map;//位图的内存缓存

    //对某Inode节点node, 在索引号为node.i_blocks处, 添加一个数据块索引, block是数据块号
    //由于过程中可能会申请间接索引块,磁盘满后可能失败,故返回是否成功
    //若同时申请一二级间接索引块,二级失败, 会进行回退,避免1级索引块变为"死块"
    bool add_index(Inode& node, u16 block) {
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
    u16 get_index(Inode& node, u16 index) {
        if (index > node.i_blocks) {
            std::cout << "index in i_node.get_index() out of range!please check\n";
            exit(-1);
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

    //将一个path转换为inode号
    //失败返回0, 一个错误的inode号
    u16 path2inode(std::string path) {
        ;
    }

    //在目录inode下新建文件name
    bool create_file_inode(char name[], u16 inode) {
        //todo:新建文件
    }

public:
    MyExt2()
        :inode_map(true), block_map(false) {}

    std::string curr_path() const {
        return current_path;
    }

    std::string volume_name() {
        return gdcache.volume_name;
    }

    bool is_new() {
        return disk.is_new();
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

        //todo:新建根目录
        DirEntry root_de;
        root_de.file_type = 2;
        root_de.inode = 1;

        Inode root_inode;
        root_inode.i_mode = (2 << 8) + 7;
        root_inode.i_blocks = 1;
        root_inode.i_block[0] = 0;
        root_inode.i_ctime = root_inode.i_atime = root_inode.i_mtime = time(NULL);

        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
    }

    //在当前目录下新建文件name
    bool create_file(char name[]) {
        return this->create_file_inode(name, current_dir);
    }

    //在目录path下新建文件name
    bool create_file(char name[], std::string path) {
        u16 inode = this->path2inode(path);
        if (inode == 0)
            return false;
        return this->create_file_inode(name, inode);
    }


    ~MyExt2()
    {
        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
    }
};
