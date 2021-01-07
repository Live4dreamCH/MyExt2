// Main.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// C++17

#include "pch.h"
#include <iostream>
#include <string>
#include "MyExt2.cpp"
#include <regex>

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块

void format(MyExt2& fs) {
    std::cout << "WARNING!!! ALL data will be lost!!! continue?(y/N) ";
    std::string res;
    std::getline(std::cin, res);
    if (res == "y" || res == "Y") {
        std::cout << "enter new volume name:(default:MyExt2) ";
        std::string vn;
        std::getline(std::cin, vn);
        if (vn == "")
            vn = "MyExt2";
        if (vn.size() > 15) {
            std::cout << "volume name too long! < 16 chars.\n";
        }
        fs.format(vn);
    }
}

void test_features() {
    //std::cout << sizeof(Group_Descriptor) << ' ' << sizeof(Inode) << ' ' << sizeof(BitMap);

    //std::string a("Hi!");
    //char b[16] = "123456789012";
    //a.copy(b, sizeof b);
    //std::cout << b;

    //std::string command, token;
    //std::regex whites("/");
    //std::sregex_token_iterator end;
    //while (true)
    //{
    //    std::cout << "$ ";
    //    std::getline(std::cin, command);
    //    std::sregex_token_iterator it(command.begin(), command.end(), whites, -1);
    //    while (it != end)
    //    {
    //        std::cout << it->str() << '\n';
    //        it++;
    //    }
    //}

    //std::string sn = "Hello!";
    //char cn[100] = "Hello!";
    //char* pt = cn;
    //std::cout << (sn == pt);
}

int main(int argc, char* argv[])
{
    test_features();

    MyExt2 test;
    while (!test.is_formatted()) {
        std::cout << "New disk, please format it.\n";
        format(test);
    }
    std::string command, user("root"), token;
    if (argc > 1)
        user = argv[1];
    std::regex whites("\\s+");
    std::sregex_token_iterator end;
    while (true)
    {
        std::cout << user + "@" + test.volume_name() + ":" + test.curr_path() + "$ ";
        std::getline(std::cin, command);
        std::sregex_token_iterator it(command.begin(), command.end(), whites, -1);
        if (it != end)
            token = it->str();

        if (token == "") {
            if (++it != end)
                token = it->str();
            if (token == "")
                continue;
        }
        
        if (token == "ls") {
            if (++it == end) {
                test.ls("./");
            }
            else {
                do {
                    token = it->str();
                    test.ls(token);
                } while (++it != end);
            }
        }
        else if (token == "cd") {
            if (++it == end) {
                continue;
            }
            token = it->str();
            test.cd(token);
        }
        //else if (token == "mkdir") {
        //    if (++it == end) {
        //        l("mkdir: missing operand");
        //        continue;
        //    }
        //    do {
        //        token = it->str();
        //        test.mkdir(token);
        //    } while (++it != end);
        //}
        //else if (token == "create") {
        //    if (++it == end) {
        //        l("create: missing operand");
        //        continue;
        //    }
        //    do {
        //        token = it->str();
        //        test.create(token);
        //    } while (++it != end);
        //}
        //else if (token == "rm") {
        //    if (++it == end) {
        //        l("rm: missing operand");
        //        continue;
        //    }
        //    do {
        //        token = it->str();
        //        test.rm(token);
        //    } while (++it != end);
        //}
        //else if (token == "read") {
        //    if (++it == end) {
        //        l("read: missing operand");
        //        continue;
        //    }
        //    do {
        //        token = it->str();
        //        test.read(token);
        //    } while (++it != end);
        //}
        //else if (token == "write") {
        //    if (++it == end) {
        //        l("write: missing operand");
        //        continue;
        //    }
        //    token = it->str();
        //    test.write(token);
        //}
        else if (token == "format") {
            format(test);
        }
        //else if (token == "chmod") {
        //    if (++it == end) {
        //        l("chmod: missing mode");
        //        continue;
        //    }
        //    token = it->str();
        //    if (++it == end) {
        //        l("chmod: missing file");
        //        continue;
        //    }
        //    std::string file;
        //    do {
        //        file = it->str();
        //        test.chmod(token, file);
        //    } while (++it != end);
        //}
        else if (token == "exit" || token == "quit") {
            std::cout << "OK, exit.\n";
            break;
        }
        else {
            std::cout << token + ": command not found\n";
        }
    }
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
