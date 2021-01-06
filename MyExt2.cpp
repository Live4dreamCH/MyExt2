#pragma once

#include "pch.h"
#include "structures.cpp"
#include <map>
#include <string>
#include <regex>
#include "files.cpp"

//�ļ�ϵͳ���ڴ����ݽṹ������
//"��̬��"�ļ�ϵͳ
class MyExt2
{
    DiskSim disk;
    std::map<u16, File*> fopen_table;//�ļ��򿪱�
    u16 current_dir = 0;//��ǰĿ¼(������㣩
    std::string current_path = "";//��ǰ·��(�ַ���) 
    Group_Descriptor gdcache;//�����������ڴ滺��
    BitMap inode_map, block_map;//λͼ���ڴ滺��
    bool is_fmt;//�Ƿ��Ѹ�ʽ��

    /*
    //��ĳInode�ڵ�node, ��������Ϊnode.i_blocks��, ���һ�����ݿ�����, block�����ݿ��
    //���ڹ����п��ܻ�������������,�����������ʧ��,�ʷ����Ƿ�ɹ�
    //��ͬʱ����һ�������������,����ʧ��, ����л���,����1���������Ϊ"����"
    bool add_block(Inode& node, u16 block) {
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
    */

    //��һ��pathת��Ϊinode��, ��path����Ϊ�մ�
    //ʧ�ܷ���0, һ�������inode��
    //���ɹ�, nameֵΪ�ļ���
    u16 path2inode(std::string path, std::string& name) {
        std::regex split("/");
        std::sregex_token_iterator end;
        std::sregex_token_iterator it(path.begin(), path.end(), split, -1);
        u16 index;
        if (it->str() == "") {
            index = 1;
        }
        else {
            index = current_dir;
        }
        while (it!=end)
        {
            name = it->str();
            index = this->name2inode(name, index);
            if (index == 0) {
                name = "";
                return 0;
            }
        }
        return index;
    }

    //��Ŀ¼dir_index��,Ѱ����Ϊname���ļ�, ��������inode��
    //�����п���һ��Ŀ¼�����������̿�, ��ʹ��2��������,�ﵽ"����"��Ч��
    //δ�ҵ�����0
    u16 name2inode(std::string name, u16 dir_index) {
        //u16 res = 0;
        if (dir_index == 0) {
            std::cout << name << ":file not found\n";
            return 0;
        }
        Inode dir_inode = get_inode(dir_index);
        char buff[2 * BlockSize] = { 0 }, * head = buff;
        char* mid = head + BlockSize, * curr = head, * temp;
        DirEntry onebyone;
        bool last, live;
        //��ҳ
        u16 i = 0, dbn, maxlen = 0, len = 0;
        while (i < dir_inode.i_blocks)
        {
            if (i == dir_inode.i_blocks - 1)
                last = true;

            dbn = get_block(dir_inode, i);
            if (i == 0)
                disk.read(dbn + DataBlockOffset, head);
            else if (i == 1)
                disk.read(dbn + DataBlockOffset, mid);
            else {
                for (int j = 0; j < BlockSize; j++) {
                    *(head + j) = *(mid + j);
                }
                curr -= BlockSize;
                disk.read(dbn + DataBlockOffset, mid);
            }
            maxlen += BlockSize;

            //Ŀ¼��
            while (true)
            {
                live = onebyone.is_alive(curr, len);
                if (len > maxlen) {
                    break;
                }
                if (live) {
                    onebyone.init(curr);
                    if (name == onebyone.name) {
                        return onebyone.inode;
                    }
                }
                temp = onebyone.next_head(curr, maxlen);
                if (temp == nullptr)
                    break;
                else {
                    maxlen -= (temp - curr);
                    curr = temp;
                }
            }
            i++;
        }
        return 0;
    }

    //todo:��Ŀ¼dir_inode���½��ļ�name
    bool create_file_inode(char name[], u16 dir_inode) {
        ;
    }

    //todo:��һ���½��Ŀ��ļ��޸�ΪĿ¼�ļ�, ����ʼ��. ..����
    void file2dir(u16 inode, u16 parent_inode) {
        ;
    }
    void file2dir(Inode& inode, u16 parent) {
        ;
    }

    /*
    //��ȡindex�Ŷ�Ӧ��inode�����ṹ
    Inode get_inode(u16 index) {
        Inode res, *pt;
        if (index == 0) {
            std::cout << "Inode get_inode(u16 index=0)\n";
            return res;
        }
        char block[BlockSize] = { 0 };
        disk.read((index - 1) / InodePerBlock + gdcache.inode_table, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        res = *pt;
        return res;
    }

    //���õ�index��inode
    void set_inode(Inode& inode, u16 index) {
        if (index == 0) {
            std::cout << "void set_inode(Inode& inode, u16 index=0)\n";
            return;
        }
        Inode* pt;
        char block[BlockSize] = { 0 };
        disk.read((index - 1) / InodePerBlock + gdcache.inode_table, block);
        pt = ((Inode*)block) + ((index - 1) % InodePerBlock);
        *pt = inode;
        disk.write((index - 1) / InodePerBlock + gdcache.inode_table, block);
    }
    */

    //��Ŀ¼, �г����µ��ļ�
    //���ļ�, �г����ļ�����
    void list(u16 index, std::string name) {
        Inode target = get_inode(index);
        u16 type = (target.i_mode >> 8) & 0x00ff;
        if (type == 2) {
            //todo:1
        }
        else {
            char rwx[4] = "---", d = '-';
            rwx[2] = (target.i_mode & (0x0001 << 0)) == 1 ? 'x' : '-';
            rwx[1] = (target.i_mode & (0x0001 << 1)) == 1 ? 'w' : '-';
            rwx[0] = (target.i_mode & (0x0001 << 2)) == 1 ? 'r' : '-';
            std::cout << d << rwx << ' ' << target.i_size << '\t' << ctime((time_t*)&(target.i_mtime)) << '\t' << name << '\n';
        }
    }

public:
    MyExt2()
        :inode_map(true), block_map(false) {
        is_fmt = !disk.is_new();
        if (is_fmt) {
            disk.read(0, (char*)&gdcache);
            disk.read(gdcache.block_bitmap, block_map.pointer());
            disk.read(gdcache.inode_bitmap, inode_map.pointer());
            current_dir = 1;
            current_path = "/";
        }
    }

    std::string curr_path() const {
        return current_path;
    }

    std::string volume_name() {
        return gdcache.volume_name;
    }

    bool is_formatted() {
        return is_fmt;
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

        //�½���Ŀ¼
        Inode root_inode(2, 7);
        root_inode.i_blocks = 1;
        root_inode.i_size = 64 * 2;
        root_inode.i_block[0] = 0;
        this->set_inode(root_inode, 1);

        DirEntry p(1,".",2), pp(1, "..", 2);
        char temp[BlockSize] = { 0 }, * pt = temp;
        *((DirEntry*)pt) = p;
        pt += p.rec_len;
        *((DirEntry*)pt) = pp;
        disk.write(0 + DataBlockOffset, temp);

        current_dir = 1;
        current_path = "/";
        gdcache.free_blocks_count--;
        gdcache.free_inodes_count--;
        gdcache.used_dirs_count++;
        block_map.set_bit(0);
        inode_map.set_bit(1);

        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
        is_fmt = true;
    }

    //�ڵ�ǰĿ¼���½��ļ�name
    bool create_file(char name[]) {
        return this->create_file_inode(name, current_dir);
    }

    //��Ŀ¼path���½��ļ�name
    bool create_file(char name[], std::string path) {
        std::string dir;
        u16 inode = this->path2inode(path, dir);
        if (inode == 0)
            return false;
        return this->create_file_inode(name, inode);
    }

    void ls(std::string path) {
        std::string name;
        u16 in = path2inode(path, name);
        if (in == 0) {
            std::cout << "ls: cannot access \'" + path + "\': No such file or directory\n";
        }
        else {
            std::cout << path + ":\n";
            this->list(in, name);
        }
    }

    ~MyExt2()
    {
        for (auto it = fopen_table.begin(); it != fopen_table.end(); it++) {
            if (!it->second->close())
                l("close fail!may lose data!");
        }
        disk.write(0, (const char*)&gdcache);
        disk.write(gdcache.block_bitmap, block_map.pointer());
        disk.write(gdcache.inode_bitmap, inode_map.pointer());
    }
};
