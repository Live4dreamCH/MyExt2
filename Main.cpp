// C++17

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
