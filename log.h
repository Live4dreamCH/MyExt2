#pragma once
#include <iostream>
#include <string>

// learned from https://www.v2ray.com/chapter_02/01_overview.html#logobject
enum class LogLevel {
    debug,  //打印所有能打印的信息
    info,   //打印全部运行时的状态
    warning,//打印警示, 程序不退出, 但出现问题
    error,  //打印错误, 程序需退出
    none    //不打印
};

extern LogLevel user_level;

void l(std::string log);
