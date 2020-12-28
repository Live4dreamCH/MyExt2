#include "pch.h"
#include <iostream>
#include <ctime>
#include <fstream>

//�ļ�ϵͳ�����ݽṹ������ز���
//Ӳ���ϵ�"��̬��"�ļ�ϵͳ

//�õ�ǰϵͳ�µ�һ���ļ�FS.txt, ģ��һ��FS_Size��С��Ӳ��.
//ÿ�ζ�дBlockSize��С��һ������
class DiskSim
{
    std::fstream disk;//�����ļ�����
    bool old = true;//�����ļ�ϵͳ�״��������ٴ���������Ϊ���ܲ�ͬ, ������Ҫ����,�����жϽ��

    char buffer[BlockSize] = { 0 };//������
    u16 bufpos = 0;
    bool buf_dirty = false;

public:

    //(�����ļ���)�����ļ�, ���ļ������ӵ���
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
    //block_num��Ӳ�̿��, ʹ�����ݿ��ʱע��+DataBlockOffset
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

    //��������д��Ӳ��; ����ָ��, �ȴ�ָ��д��������
    //block_num��Ӳ�̿��, ʹ�����ݿ��ʱע��+DataBlockOffset
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

    //���ػ�����ָ��, ������λΪ1
    //Լ��������洢��ָ���Ժ�ʹ��
    char* get_buf() {
        buf_dirty = true;
        return buffer;
    }

    //Ӳ���Ƿ����½���
    bool is_new() {
        return !old;
    }

    //����ȫ������
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

//λͼ��ṹ
class BitMap
{
    u64 bits[BlockSize / 8] = { 0 };
    bool start;

public:
    BitMap(bool is_inode) {
        this->start = is_inode;
    }

    //��posλΪ1
    void set_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] |= t << (pos % 64);
    }

    //����posλΪ0
    void reset_bit(u16 pos) {
        pos -= start;
        u64 t = 0x01;
        bits[pos / 64] &= ~(t << (pos % 64));
    }

    //��ȡλͼ�׵�ַ,���ڶ�д����
    char* pointer() {
        return (char*)bits;
    }

    //ȡĳһλ��ֵ
    bool get_bit(u16 pos) {
        pos -= start;
        u64 t = (u64)0x01 << (pos % 64);
        return bool(bits[pos / 64] & t);
    }

    //Ѱ�ҿ�λ: ��ĳһλ(pos,��)���Ѱ��
    //����������nλΪ��, �򷵻���nλ����λλ��, ������nλΪ1
    //��������nλ, �򷵻�����������λ�����෴��-1(����0�Ķ�����)
    //��ν"����"����βѭ����
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

//��������, �����ļ�ϵͳ�Ļ�����Ϣ
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
    char volume_name[16] = { 0 };//����
    u16 block_bitmap = 1;//���ݿ�λͼ���ڵĴ��̿��
    u16 inode_bitmap = 2;//�������λͼ�Ĵ��̿��
    u16 inode_table = 3;//�����������ʼ���̿��

    //todo:����⼸��ֵ��һϵ�в���,������ �Լ���.��װ���Ǻ�û��˼,ֱ���ⲿ�޸İ�
    u16 free_blocks_count = BlockSize * 8;//���п�ĸ���(ָ���ݿ�)
    u16 free_inodes_count = BlockSize * 8;//�����������ĸ���
    u16 used_dirs_count = 0;//Ŀ¼�ĸ���

    char pad[4];//���
    char remain_padding[480] = { 0 };//���������512�ֽ�

    Group_Descriptor()
    {
        for (int i = 0; i < 4; i++)
        {
            pad[i] = (char)0xff;
        }
    }
};

//���ڼ��Ѱַ�����ݿ�����
struct IndexBlock
{
    u16 index[BlockSize / 2] = { 0 };
};

//�����ڵ�inode, �����ļ���Ϣ��λ��
//sizeof(Inode) = 64 Bytes
struct Inode
{
    u16 i_mode = 0;//��8λ���ļ�����, �Ͱ�λ�Ƿ���Ȩ��
    u16 i_blocks = 0;//�ļ�ռ�õ����ݿ����
    u32 i_size = 0;//�ļ���С, ��λByte
    u64 i_atime = 0, i_ctime = 0, i_mtime = 0, i_dtime = 0;//a����, c����, m�޸�, dɾ��ʱ��
    u16 i_block[8] = { 0 };//�ļ���ռ�����ݿ��������(�洢���ݿ��)
    char i_pad[8];//�����64�ֽ�

    //perm=permission,û������
    Inode(char type = 1, char perm = 7)
    {
        i_mode = (type << 8) + perm;
        i_atime = i_ctime = i_mtime = time(NULL);
        for (int i = 0; i < 8; i++)
        {
            i_pad[i] = (char)0xff;
        }
    }
    //�޸ķ���Ȩ��
    void perm(char p) {
        i_mode = (i_mode & (u16)0xff00) + p;
    }
    //��ȡʱ
    void access() {
        i_atime = time(NULL);
    }
    //�޸�ʱ,sizeΪ�ļ��´�С
    void modify(u32 size) {
        i_mtime = time(NULL);
        i_size = size;
    }
    //ɾ��ʱ
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
};

//Ŀ¼�ļ������е�һ��
struct DirEntry
{
    u16 inode = 0;//��Ŀ¼���Ӧ�ļ���inode��, ��0��Ϊ��ȷ��Ŀ¼��
    u16 rec_len = 7;//��Ŀ¼��ĳ���(������,����7,���ܴ���ʵ�ʳ���,��Ϊ�����п�϶��������ҵ���һ��Ŀ¼��)
    char name_len = 0;//�ļ�������(������\0)
    char file_type = 0;//�ļ�����
    char name[256] = { 0 };//�ļ���

    DirEntry() {}

    //�ж�һ�ζ������ֽ����Ƿ�Ϊ��ȷ���ڵ�Ŀ¼��
    bool is_alive(char* bits) {
        u16* test = (u16*)bits;
        if (*test == 0)
            return false;
        return true;
    }

    //�Ӷ����������н����ṹ, ���ؿ��ܴ��ڵ���һ����׵�ַ
    //ʹ��ʱ�豣֤���ݵ���ȷ��
    char* init(char* bits) {
        u16* pt16 = (u16*)bits;
        inode = *(pt16++);
        rec_len = *pt16;
        char* pt8 = bits + 4;
        name_len = *(pt8++);
        file_type= *(pt8++);
        u16 i = 0;
        for (; i < name_len; i++) {
            name[i]= *(pt8++);
        }
        name[i] = 0;
        return bits + rec_len;
    }

    //��ȡ��ָ��λ��, ����һ������Ŀ¼��֮��Ŀ�϶�ĳ���
    //Ҫ��ָ��ָ��һ���ѱ�ɾ����Ŀ¼���׵�ַ, �����������ȷ���ڵ�Ŀ¼��
    u64 gap_len(char* bits) {
        u64 sum_len = 0;
        u16 this_len = 0;
        while (!is_alive(bits)) {
            this_len = *(((u16*)bits) + 1);
            sum_len += this_len;
            bits += this_len;
        }
        return sum_len;
    }
};
