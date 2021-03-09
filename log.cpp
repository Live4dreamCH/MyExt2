#pragma once
#include "log.h"

LogLevel user_level = LogLevel::info;

void l(std::string log) {
    std::cerr << log << '\n';
}
