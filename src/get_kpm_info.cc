// License: GPLv3
// Author: affggh
// Date: 20250919
// Description: This file is a web support for display kpm info

#ifdef __EMSCRIPTEN__

#include <map>
#include <string>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <vector>
#include <emscripten.h>
#include <emscripten/bind.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "./kp/tools/kpm.h"
#ifdef __cplusplus
}
#endif

using namespace emscripten;

val getKpmInfo(uintptr_t ptr, size_t len)
{
    std::map<std::string, std::string> info;
    val obj = val::object();
    auto raw_data = reinterpret_cast<const uint8_t *>(ptr);

    kpm_info_t kpm_info = {
        .name = nullptr,
        .version = nullptr,
        .license = nullptr,
        .author = nullptr,
        .description = nullptr,
    };
    int ret = get_kpm_info(
        reinterpret_cast<const char *>(raw_data),
        static_cast<int>(len),
        &kpm_info);
    if (!ret)
    {
        emscripten_log(EM_LOG_CONSOLE, "Get module name: %s info!", kpm_info.name);
        info["name"] = kpm_info.name;
        info["version"] = kpm_info.version;
        info["license"] = kpm_info.license;
        info["author"] = kpm_info.author;
        info["description"] = kpm_info.description;

        for (auto &kv : info)
        {
            obj.set(kv.first, kv.second);
        }
    }

    return obj;
}

EMSCRIPTEN_BINDINGS(kptools)
{
    function("getKpmInfo", &getKpmInfo);
}

#endif
