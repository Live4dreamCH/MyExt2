#pragma once

typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

constexpr u16 BlockSize = 512;//���С
constexpr u16 InodeSize = 64;//inode��С
constexpr u64 FS_Size = BlockSize * (1 + 1 + 1 + InodeSize * 8 + BlockSize * 8);//�����ļ�ϵͳ��ռ���̴�С
constexpr u16 InodeOffset = 2;
constexpr u16 DataBlockOffset = 1 + 1 + 1 + InodeSize * 8;

//����:���̿�Ŵ�0��ʼ, �����������Ŀ�ž�Ϊ0
//�����ڵ�(inode)�Ŵ�1��ʼ, ���Ŀ¼��inode��Ϊ1
//���ݿ�Ŵ�0��ʼ, ���Ŀ¼��Ŀ¼�ļ����ڵ�0��

/*todo:��
main�е�ָ����������ʽ������
*/