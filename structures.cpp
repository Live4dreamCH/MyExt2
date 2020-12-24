#include "pch.h"
#include <iostream>
#include <ctime>
#include <fstream>

//�õ�ǰϵͳ�µ�һ���ļ�, ģ��һ��FS_Size��С��Ӳ��.
//ÿ�ζ�дBlockSize��С��һ������
class DiskSim
{
    std::fstream disk;
    bool old = true;

public:
    char buffer[BlockSize] = { 0 };

    //�����ļ�, ���ļ������ӵ���
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

    //��Ӳ�̶���������,����ָ���ٸ��ƹ�ȥ
    void read(u16 block_num, char* s = nullptr)
    {
        disk.seekg(BlockSize * block_num);
        disk.read(buffer, BlockSize);
        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                s[i] = buffer[i];
            }
        }
    }

    //��������д��Ӳ��; ����ָ��, �ȴ�ָ��д��������
    void write(u16 block_num, const char* s = nullptr)
    {
        if (s != nullptr) {
            for (int i = 0; i < BlockSize; i++)
            {
                buffer[i] = s[i];
            }
        }
        disk.seekp(BlockSize * block_num);
        disk.write(buffer, BlockSize);
    }

    //void init() {
    //    disk.seekp(0);
    //    for (int i = 0; i < FS_Size; i++)
    //    {
    //        disk.put(char(0x0));
    //    }
    //    std::cout << "Disk has been initialized with full 0!\n";
    //}

    //Ӳ���Ƿ����½���
    bool is_new() {
        return !old;
    }

    ~DiskSim()
    {
        disk.close();
    }
};

class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };

public:
    void set_bit(u64 pos) {
        u64 t = 0x01;
        bits[pos / 64] |= t << (pos % 64);
    }

    void reset_bit(u64 pos) {
        u64 t = 0x01;
        bits[pos / 64] &= ~(t << (pos % 64));
    }

    char* pointer() {
        return (char*)bits;
    }

};

//��������, �����ļ�ϵͳ�Ļ�����Ϣ
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
    char volume_name[16];//����
    u16 block_bitmap = 1;//���ݿ�λͼ���ڵĴ��̿��
    u16 inode_bitmap = 2;//�������λͼ�Ĵ��̿��
    u16 inode_table = 3;//������������ʼ���̿��
    u16 free_blocks_count;//���п�ĸ���(ָ���ݿ�?)
    u16 free_inodes_count;//�����������ĸ���
    u16 used_dirs_count;//Ŀ¼�ĸ���
    char pad[4];//���
    char remain_padding[480] = { 0 };//���������512�ֽ�

    Group_Descriptor(char name[], u16 fb = BlockSize * 8, u16 fi = BlockSize * 8, u16 dir = 0) :free_blocks_count(fb), free_inodes_count(fi), used_dirs_count(dir)
    {
        for (int i = 0; i < 4; i++)
        {
            pad[i] = (char)0xff;
        }
    }
};

//�����ڵ�, �����ļ���Ϣ��λ��
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//��8λ���ļ�����, �Ͱ�λ�Ƿ���Ȩ��
    u16 i_blocks = 0;//�ļ�ռ�õ����ݿ����
    u32 i_size = 0;//�ļ���С, ��λByte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a����, c����, m�޸�, dɾ��ʱ��
    u16 i_block[8] = { 0 };//�ļ���ռ�����ݿ��������(�洢���ݿ��)
    char i_pad[8];//�����64�ֽ�

    Inode(char type = 1, char perm = 7)
    {
        i_mode = (type << 8) + perm;
        i_atime = i_ctime = i_mtime = time(NULL);
        for (int i = 0; i < 8; i++)
        {
            i_pad[i] = (char)0xff;
        }
    }
    void perm(char p) {
        i_mode = (i_mode & (u16)0xff00) + p;
    }
    void access() {
        i_atime = time(NULL);
    }
    void modify(u32 size) {
        i_mtime = time(NULL);
        i_size = size;
        i_blocks = size / BlockSize;
        if (size % BlockSize)
            i_blocks++;
    }
    void set_index() {
        ;
    }
    u16 get_index() {
        ;
    }
};

//Ŀ¼�ļ������е�һ��
struct DirEntry
{
    u16 inode = 0;
    u16 rec_len = 7;
    char name_len = 1;
    char file_type = 0;
    char name[256] = { 0 };
};

//���ڼ��Ѱַ�����ݿ�����
struct IndexBlock
{
    u16 index[BlockSize / 2] = { 0 };
};