#include "pch.h"
#include "structures.cpp"
#include <set>
#include <string>
#include <regex>

//�ļ�ϵͳ���ڴ����ݽṹ������
//"��̬��"�ļ�ϵͳ
class MyExt2
{
    DiskSim disk;
    std::set<u16> fopen_table;//�ļ��򿪱�
    //u16 last_alloc_inode = 0;//�ϴη������������
    //u16 last_alloc_block = 0;//�ϴη�������ݿ��
    u16 current_dir = 0;//��ǰĿ¼(������㣩
    std::string current_path = "";//��ǰ·��(�ַ���) 
    Group_Descriptor gdcache;//�����������ڴ滺��
    BitMap inode_map, block_map;//λͼ���ڴ滺��

    //��ĳInode�ڵ�node, ��������Ϊnode.i_blocks��, ���һ�����ݿ�����, block�����ݿ��
    //���ڹ����п��ܻ�������������,�����������ʧ��,�ʷ����Ƿ�ɹ�
    //��ͬʱ����һ�������������,����ʧ��, ����л���,����1���������Ϊ"����"
    bool add_index(Inode& node, u16 block) {
        u16 index = node.i_blocks;
        if (index < 6) {
            node.i_block[index] = block;
        }
        //����������������Ѿ�����, ʵ���ϲ���,��ͬ��������
        //�Ƚϸ��ӵ������ݿ�Ŵ�0��ʼ,����жϼ���������Ƿ����?������Ҫ����0�ű�Ϊ��Ŀ¼��Ŀ¼��������/i_blocks����+��ʱ����
        //����������һ�����ݿ�������ͬʱ,��Ҫ�ٶ�η����·���ϵ��������Կ��ÿ��,����Ҫ��λͼ,����Ӧ��ô��?
        //�������:ֻ������ĩβi_blocks���һ������,�õ����������ʱ��������,��i_blocks���жϼ���������Ƿ��ѽ���
        //��Ϊ��Ҫ�õ�dsk,bmp��MyExt2�ĳ�Ա,�������Ӧ��Ų��ȥ
        //���ҷ������������ҲҪ�޸��������� ��λͼ��
        //�ѽ��, ���Ϲ�����
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

    //��ĳInode�ڵ�node, ��������index, ��ȡһ�����ݿ��
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

    //��һ��pathת��Ϊinode��
    //ʧ�ܷ���0, һ�������inode��
    u16 path2inode(std::string path) {
        ;
    }

    //��Ŀ¼inode���½��ļ�name
    bool create_file_inode(char name[], u16 inode) {
        //todo:�½��ļ�
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

    //��ʽ��
    //����ȫ������, ���ÿ����ֶ�, ����ʼ����Ŀ¼, ����������
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

        //todo:�½���Ŀ¼
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

    //�ڵ�ǰĿ¼���½��ļ�name
    bool create_file(char name[]) {
        return this->create_file_inode(name, current_dir);
    }

    //��Ŀ¼path���½��ļ�name
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
