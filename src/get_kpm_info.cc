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
#include <kpm.h>
#ifdef __cplusplus
}
#endif

using namespace emscripten;

val getKpmInfo(std::string path)
{
    std::map<std::string, std::string> info;
    val obj = val::object();

    emscripten_log(EM_LOG_CONSOLE, "getKpmInfo: recive path %s", path.c_str());
    if (access(path.c_str(), F_OK) != 0)
    {
        emscripten_log(EM_LOG_ERROR, "Could not find path: %s", path.c_str());
    }
    else
    {
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp)
        {
            emscripten_log(EM_LOG_ERROR, "Could not open file: %s", path.c_str());
            return obj;
        }

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        std::vector<uint8_t> buffer(size);
        fread(buffer.data(), 1, size, fp);
        if (fp)
        {
            fclose(fp);
        }

        kpm_info_t kpm_info = {
            .name = nullptr,
            .version = nullptr,
            .license = nullptr,
            .author = nullptr,
            .description = nullptr,
        };
        int ret = get_kpm_info(
            reinterpret_cast<const char *>(buffer.data()),
            static_cast<int>(buffer.size()),
            &kpm_info);
        if (!ret)
        {
            emscripten_log(EM_LOG_CONSOLE, "Get module name: %s info!", kpm_info.name);
            info["name"] = kpm_info.name;
            info["version"] = kpm_info.version;
            info["license"] = kpm_info.license;
            info["author"] = kpm_info.author;
            info["description"] = kpm_info.description;

            for (auto& kv : info) {
                obj.set(kv.first, kv.second);
            }
        }
    }

    return obj;
}

EMSCRIPTEN_BINDINGS(kptools)
{
    function("getKpmInfo", &getKpmInfo);
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    void init_kptools() {}
}
#endif