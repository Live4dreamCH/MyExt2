// Main.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// C++17

#include <regex>
#include <iostream>
#include <string>
#include "constants.h"
#include "log.h"
#include "MyExt2.h"
#include "CLI.h"
#include "TestAndTry.h"

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块


int main(int argc, char* argv[])
{
    //MyTry();
    user_level = LogLevel::debug;

    MyExt2 test;
    CLI(test, argc, argv);
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
