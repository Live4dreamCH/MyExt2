#include "TestAndTry.h"

void MyTest() {

}

#include <string>
#include <regex>
#include <iostream>
#include "log.h"
#include "structures.h"

void MyTry() {
    //硬盘上数据结构的大小
    std::cout << sizeof(Group_Descriptor) << ' ' << sizeof(Inode) << ' ' << sizeof(BitMap);

    //string.copy(dest, count, pos=0) 不是空终止的
    std::string a("Hi!");
    char b[16] = "123456789012";
    a.copy(b, sizeof b);
    std::cout << b;

    //使用sregex_token_iterator分出token
    std::string command, token;
    std::regex whites("/");
    std::sregex_token_iterator end;
    while (true)
    {
        std::cout << "$ ";
        std::getline(std::cin, command);
        std::sregex_token_iterator it(command.begin(), command.end(), whites, -1);
        while (it != end)
        {
            std::cout << it->str() << '\n';
            it++;
        }
    }

    //string == char*
    std::string sn = "Hello!";
    char cn[100] = "Hello!";
    char* pt = cn;
    std::cout << (sn == pt);

    //使用re, 去除路径中的./和../, 并且和原路径等价
    std::string path, old;
    std::regex again("/\\./");
    std::regex up("/[^/]*/\\.\\./");
    while (true) {
        std::cout << "input:$ ";
        std::cin >> path;
        do {
            old = path;
            path = std::regex_replace(path, again, "/");
        } while (old != path);

        while (path.substr(0, 4) == "/../") {
            path.erase(1, 3);
        }
        do {
            old = path;
            path = std::regex_replace(path, up, "/");
        } while (old != path);
        l(path);
        l("");
    }

    //判断是否为可执行文件
    std::string name;
    std::regex ext(".*(\\.exe|\\.bin|\\.com)");
    while (true)
    {
        std::cout << "$ ";
        std::getline(std::cin, name);
        if (name.find('.') == std::string::npos)
            l("true");
        else if (std::regex_match(name, ext)) {
            l("true");
        }
        else
            l("flase");
    }

}
