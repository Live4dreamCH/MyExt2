// Main.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// C++17

#include "pch.h"
#include <iostream>
#include <string>
#include "MyExt2.cpp"

//定义:磁盘块号从0开始, 如组描述符的块号就为0
//索引节点(inode)号从1开始, 如根目录的inode就为1
//数据块号从0开始, 如根目录的目录文件就在第0块

int main()
{
	//std::cout << sizeof(Group_Descriptor) << ' ' << sizeof(Inode) << ' ' << sizeof(BitMap);
	MyExt2 test;
	std::string command;
	while (true)
	{
		std::cout << "root@" + test.volume_name() + ":" + test.cwd() + "$ ";
		std::getline(std::cin, command);

		if (command.find("ls") != std::string::npos) {
			;
		}
		else if (command.find("cd") != std::string::npos) {
			;
		}
		else if (command.find("mkdir") != std::string::npos) {
			;
		}
		else if (command.find("touch") != std::string::npos) {
			;
		}
		else if (command.find("rm") != std::string::npos) {
			;
		}
		else if (command.find("cat") != std::string::npos) {
			;
		}
		else if (command.find("edit") != std::string::npos) {
			;
		}
		else if (command.find("format") != std::string::npos) {
			std::cout << "enter new volume name: ";
			std::string vn;
			std::getline(std::cin, vn);
			test.format(vn);
		}
		else if (command.find("chmod") != std::string::npos) {
			;
		}
		else if (command.find("exit") != std::string::npos) {
			std::cout << "OK, exit.\n";
			break;
		}
		else {
			std::cout << command << ": command not found\n";
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
