// License: GPLv3
// Author: affggh
// Date: 20260426
// Description: This file is a web support for display more extra info

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <string>
#include <format>
#include "./kp/version"

using namespace emscripten;

std::string getVersion(void) {
    auto version = (MAJOR << 16) | (MINOR << 8) | PATCH;
    return std::format("{:x}", version);
}

EMSCRIPTEN_BINDINGS(kptools)
{
    function("getVersion", &getVersion);
}

#endif
