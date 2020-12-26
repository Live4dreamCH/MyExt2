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
	//todo:�����?

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
	BitMap(bool is_inode) :start(is_inode) {}

	//��posλΪ1
	void set_bit(u64 pos) {
		pos -= start;
		u64 t = 0x01;
		bits[pos / 64] |= t << (pos % 64);
	}

	//����posλΪ0
	void reset_bit(u64 pos) {
		pos -= start;
		u64 t = 0x01;
		bits[pos / 64] &= ~(t << (pos % 64));
	}

	//��ȡλͼ�׵�ַ,����д�����
	char* pointer() {
		return (char*)bits;
	}

	//ȡĳһλ��ֵ
	bool get_bit(u64 pos) {
		pos -= start;
		u64 t = (u64)0x01 << (pos % 64);
		return bool(bits[pos / 64] & t);
	}

	//Ѱ�ҿ�λ: ��ĳһλ(pos,��)���Ѱ��
	//����������nλΪ��, �򷵻���nλ����λλ��
	//��������nλ, �򷵻�����������λ�����෴��
	//��ν"����"����βѭ����
	long long int find_zeros(u64 pos, u64 n) {
		pos -= start;
		long long int curr = 0, max = 0;
		for (u64 i = pos; i < BlockSize * 8; i++)
		{
			if (this->get_bit(i)) {
				if (max < curr)
					max = curr;
				curr = 0;
			}
			else {
				curr++;
				if (curr == n) {
					return i - n + 1;
				}
			}
		}
		for (u64 i = 0; i < pos; i++)
		{
			if (this->get_bit(i)) {
				if (max < curr)
					max = curr;
				curr = 0;
			}
			else {
				curr++;
				if (curr == n) {
					if (i + 1 >= n)
						return i - n + 1;
					else
						return i + BlockSize * 8 - n + 1;
				}
			}
		}
		return (curr > max) ? -curr : -max;
	}
};

//��������, �����ļ�ϵͳ�Ļ�����Ϣ
//sizeof(Group_Descriptor) = 512 Bytes
struct Group_Descriptor
{
	char volume_name[16];//����
	u16 block_bitmap = 1;//���ݿ�λͼ���ڵĴ��̿��
	u16 inode_bitmap = 2;//�������λͼ�Ĵ��̿��
	u16 inode_table = 3;//�����������ʼ���̿��

	//todo:����⼸��ֵ��һϵ�в���,������ �Լ���.��װ���Ǻ�û��˼,ֱ���ⲿ�޸İ�
	u16 free_blocks_count;//���п�ĸ���(ָ���ݿ�?)
	u16 free_inodes_count;//�����������ĸ���
	u16 used_dirs_count;//Ŀ¼�ĸ���

	char pad[4];//���
	char remain_padding[480] = { 0 };//���������512�ֽ�

	Group_Descriptor(char name[], u16 fb = BlockSize * 8, u16 fi = BlockSize * 8, u16 dir = 0)
		:free_blocks_count(fb), free_inodes_count(fi), used_dirs_count(dir)
	{
		for (int i = 0; i < 4; i++)
		{
			pad[i] = (char)0xff;
		}
	}
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
	//����һ�����ݿ�����, block�����ݿ��, index�ǵڼ�������(0ʼ), bmp�ǿ�λͼ
	void set_index(u16 block, u16 index, DiskSim& dsk, BitMap& bmp) {
		if (index < 6) {
			i_block[index] = block;
		}
		else if (index < 6 + BlockSize / 2) {
			IndexBlock i1;
			index -= 6;
			//todo:����������������Ѿ�����, ʵ���ϲ���,��ͬ��������
			//�Ƚϸ��ӵ������ݿ�Ŵ�0��ʼ,����жϼ���������Ƿ����?������Ҫ����0�ű�Ϊ��Ŀ¼��Ŀ¼��������/i_blocks����+��ʱ����
			//����������һ�����ݿ�������ͬʱ,��Ҫ�ٶ�η����·���ϵ��������Կ��ÿ��,����Ҫ��λͼ,����Ӧ��ô��?

			//�������:ֻ������ĩβi_blocks���һ������,�õ����������ʱ��������,��i_blocks���жϼ���������Ƿ��ѽ���
			//��Ϊ��Ҫ�õ�dsk,bmp��MyExt2�ĳ�Ա,�������Ӧ��Ų��ȥ
			//get_index���ѿ���,���漰dsk,��Ӧ�ƶ�
			dsk.read(i_block[6] + DataBlockOffset, (char*)&i1);
			i1.index[index] = block;
			dsk.write(i_block[6] + DataBlockOffset, (char*)&i1);
		}
		else {
			IndexBlock i1, i2;
			index -= 6 + BlockSize / 2;
			dsk.read(i_block[7] + DataBlockOffset, (char*)&i1);
			dsk.read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
			i2.index[index % (BlockSize / 2)] = block;
			dsk.write(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
		}
		i_blocks++;
	}
	//��������, ��ȡһ�����ݿ��
	u16 get_index(u16 index, DiskSim& dsk) {
		if (index > i_blocks) {
			std::cout << "index in i_node.get_index() out of range!please check\n";
			exit(-1);
		}
		if (index < 6) {
			return i_block[index];
		}
		else if (index < 6 + BlockSize / 2) {
			IndexBlock i1;
			index -= 6;
			dsk.read(i_block[6] + DataBlockOffset, (char*)&i1);
			return i1.index[index];
		}
		else {
			IndexBlock i1, i2;
			index -= 6 + BlockSize / 2;
			dsk.read(i_block[7] + DataBlockOffset, (char*)&i1);
			dsk.read(i1.index[index / (BlockSize / 2)] + DataBlockOffset, (char*)&i2);
			return i2.index[index % (BlockSize / 2)];
		}
	}
};

//Ŀ¼�ļ������е�һ��
//todo:Ŀ¼�����,�����Ƚ��鷳
struct DirEntry
{
	u16 inode = 0;//��Ŀ¼���Ӧ�ļ���inode��
	u16 rec_len = 7;//��Ŀ¼��ĳ���(������,7~261)
	char name_len = 1;//�ļ�������
	char file_type = 0;//�ļ�����
	char name[256] = { 0 };//�ļ���
};

//���ڼ��Ѱַ�����ݿ�����
struct IndexBlock
{
	u16 index[BlockSize / 2] = { 0 };
};
