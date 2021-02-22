#pragma once
#include <iostream>
#include <string>

// learned from https://www.v2ray.com/chapter_02/01_overview.html#logobject
enum class LogLevel {
    debug,
    info,
    warning,
    error,
    none
};

void l(std::string log);
