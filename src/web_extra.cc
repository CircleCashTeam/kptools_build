// License: GPLv3
// Author: affggh
// Date: 20260426
// Description: This file is a web support for display more extra info

#ifdef __EMSCRIPTEN__
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <emscripten.h>
#include <emscripten/bind.h>

#include <bzlib.h>
#include <lz4.h>
#include <lz4frame.h>
#include <xz.h>
#include <zlib.h>
// #include <zstd.h>

#include "./kp/tools/bootimg.h"
#include "./kp/version"

extern "C"
{
  // Defined in bootimg.c
  int decompress_xz(const uint8_t *src, size_t srcSize, uint8_t **dst, uint32_t *dstSize);
}

using namespace emscripten;

enum
{
  COMP_RAW,
  COMP_GZIP,
  COMP_LZ4,
  COMP_LZ4_LEGACY,
  COMP_ZSTD,
  COMP_BZ2,
  COMP_XZ,
  COMP_LZMA,
};

#define IKCFG_ST "IKCFG_ST"
#define IKCFG_ED "IKCFG_ED"

struct DecompressResult
{
  std::vector<uint8_t> data;
  bool success = false;
  const char *error_msg = nullptr;
};

int detect_compress_method(compress_head data)
{
  // 1. GZIP / ZOPFLI (1F 8B)
  if (data.magic[0] == 0x1F && data.magic[1] == 0x8B)
    return COMP_GZIP;
  if (data.magic[0] == 0x1F && data.magic[1] == 0x9E)
    return COMP_GZIP;
  // 2. LZ4 (04 22 4D 18 is Frame )
  if (data.magic[0] == 0x04 && data.magic[1] == 0x22 && data.magic[2] == 0x4D &&
      data.magic[3] == 0x18)
    return COMP_LZ4;

  if (data.magic[0] == 0x03 && data.magic[1] == 0x21 && data.magic[2] == 0x4C &&
      data.magic[3] == 0x18)
    return COMP_LZ4;

  // LZ4 Legacy (02 21 4C 18)
  if (data.magic[0] == 0x02 && data.magic[1] == 0x21 && data.magic[2] == 0x4C &&
      data.magic[3] == 0x18)
    return COMP_LZ4_LEGACY;

  // 3. ZSTD  28 B5 2F FD
  if (data.magic[0] == 0x28 && data.magic[1] == 0xB5 && data.magic[2] == 0x2F &&
      data.magic[3] == 0xFD)
    return COMP_ZSTD;

  // 4. BZIP2 (BZh) - 42 5A 68
  if (data.magic[0] == 0x42 && data.magic[1] == 0x5A && data.magic[2] == 0x68)
    return COMP_BZ2;

  // 5. XZ - FD 37 7A 58 5A 00
  if (data.magic[0] == 0xFD && data.magic[1] == 0x37 && data.magic[2] == 0x7A &&
      data.magic[3] == 0x58)
    return COMP_XZ;

  // 6. LZMA - 5D 00 00
  if (data.magic[0] == 0x5D && data.magic[1] == 0x00 && data.magic[2] == 0x00)
    return COMP_LZMA;

  return COMP_RAW; // Raw Kernel
}

DecompressResult auto_decompress(const std::vector<uint8_t> &input,
                                 size_t expected_size, int method = COMP_RAW)
{
  DecompressResult res;
  if (input.size() < 4)
  {
    res.error_msg = "Input too small";
    return res;
  }

  //// 1. 识别压缩方式
  // compress_head k_head;
  // std::memcpy(&k_head, input.data(), sizeof(k_head));
  // int method = detect_compress_method(k_head);

  // 处理 Raw 情况
  if (method == COMP_RAW)
  {
    res.data = input;
    res.success = true;
    return res;
  }

  // 预估输出大小，如果用户没给 expected_size，默认给个基数
  size_t out_cap = expected_size ? expected_size : (input.size() * 4);

  switch (method)
  {
  case COMP_GZIP:
  {
    // 参考原函数逻辑：使用 zlib 的 inflate
    res.data.resize(out_cap);
    z_stream strm = {0};
    strm.next_in = (Bytef *)input.data();
    strm.avail_in = input.size();

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
    {
      res.error_msg = "Gzip Init Fail";
      return res;
    }

    while (true)
    {
      strm.next_out = res.data.data() + strm.total_out;
      strm.avail_out = res.data.size() - strm.total_out;
      int ret = inflate(&strm, Z_NO_FLUSH);
      if (ret == Z_STREAM_END)
        break;
      if (ret != Z_OK)
      {
        inflateEnd(&strm);
        res.error_msg = "Gzip Decompress Fail";
        return res;
      }
      res.data.resize(res.data.size() * 2);
    }
    res.data.resize(strm.total_out);
    inflateEnd(&strm);
    res.success = true;
    break;
  }

  case COMP_LZ4:
  { // LZ4 Frame
    LZ4F_decompressionContext_t dctx;
    if (LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION) != 0)
    {
      res.error_msg = "LZ4 Context Fail";
      return res;
    }

    res.data.resize(out_cap);
    size_t produced = res.data.size();
    size_t consumed = input.size();

    size_t ret = LZ4F_decompress(dctx, res.data.data(), &produced, input.data(),
                                 &consumed, nullptr);
    LZ4F_freeDecompressionContext(dctx);

    if (LZ4F_isError(ret))
    {
      res.error_msg = LZ4F_getErrorName(ret);
      return res;
    }
    res.data.resize(produced);
    res.success = true;
    break;
  }

  case COMP_LZ4_LEGACY:
  {                                      // LZ4 Block (参考你原函数的 while 循环逻辑)
    const uint8_t *p = input.data() + 4; // Skip magic
    const uint8_t *end = input.data() + input.size();

    // 临时块缓冲区
    std::vector<uint8_t> block_out(64 * 1024); // 64KB block

    while (p + 4 <= end)
    {
      uint32_t b_size;
      std::memcpy(&b_size, p, 4);
      p += 4;
      if (b_size == 0 || p + b_size > end)
        break;

      int decoded = LZ4_decompress_safe(
          (const char *)p, (char *)block_out.data(), b_size, block_out.size());
      if (decoded < 0)
        break;

      res.data.insert(res.data.end(), block_out.begin(),
                      block_out.begin() + decoded);
      p += b_size;
    }
    if (!res.data.empty())
      res.success = true;
    else
      res.error_msg = "LZ4 Legacy Fail";
    break;
  }

    // case COMP_ZSTD: {
    //   size_t dSize = ZSTD_getFrameContentSize(input.data(), input.size());
    //   if (dSize == ZSTD_CONTENTSIZE_ERROR || dSize == ZSTD_CONTENTSIZE_UNKNOWN)
    //     dSize = out_cap;
    //
    //  res.data.resize(dSize);
    //  size_t actual =
    //      ZSTD_decompress(res.data.data(), dSize, input.data(), input.size());
    //  if (ZSTD_isError(actual)) {
    //    res.error_msg = ZSTD_getErrorName(actual);
    //    return res;
    //  }
    //  res.data.resize(actual);
    //  res.success = true;
    //  break;
    //}

  case COMP_BZ2:
  {
    unsigned int produced = (unsigned int)out_cap;
    res.data.resize(produced);
    int ret = BZ2_bzBuffToBuffDecompress((char *)res.data.data(), &produced,
                                         (char *)input.data(),
                                         (unsigned int)input.size(), 0, 0);
    if (ret == BZ_OK)
    {
      res.data.resize(produced);
      res.success = true;
    }
    else
    {
      res.error_msg = "BZIP2 Fail";
    }
    break;
  }

  case COMP_XZ:
  case COMP_LZMA:
  {
    res.data.resize(out_cap);
    uint8_t *xz_dst = nullptr;
    uint32_t xz_size = 0;

    if (decompress_xz(input.data(), input.size(), &xz_dst, &xz_size) == 0)
    {
      res.data.assign(xz_dst, xz_dst + xz_size);
      if (xz_dst)
        free(xz_dst);
      res.success = true;
    }
    else
    {
      res.error_msg = "XZ/LZMA Decompress Fail";
      res.success = false;
    }
    break;
  }

  default:
    res.error_msg = "Unknown compression method";
    break;
  }

  return res;
}

std::string getVersion(void)
{
  auto version = (MAJOR << 16) | (MINOR << 8) | PATCH;
  return std::format("{:x}", version);
}

static inline std::string dump_ikconfig(std::vector<uint8_t> &input)
{
  std::string ret;

  std::string_view view(reinterpret_cast<const char *>(input.data()), input.size());
  auto pos_start_idx = view.find(IKCFG_ST);
  auto pos_end_idx = view.find(IKCFG_ED);

  if (pos_start_idx == std::string_view::npos ||
      pos_end_idx == std::string_view::npos)
  {
    emscripten_log(EM_LOG_ERROR, "Cannot find kernel config tags!\n");
    return ret;
  }

  size_t data_start = pos_start_idx + 8;
  size_t data_len = pos_end_idx - data_start;

  if (data_len <= 0)
  {
    emscripten_log(EM_LOG_ERROR, "Find ikconfig data len is 0.\n");
    return ret;
  }

  const std::vector<uint8_t> ikconfig_data(
      input.begin() + data_start,
      input.begin() + data_start + data_len);
  const auto ikconfig_raw = auto_decompress(ikconfig_data, 0, COMP_GZIP);
  if (ikconfig_raw.success)
  {
    ret.assign(ikconfig_raw.data.begin(), ikconfig_raw.data.end());
  }
  else
  {
    emscripten_log(EM_LOG_ERROR, "Could not decompress ikconfig: %s\n", ikconfig_raw.error_msg);
  }

  return ret;
}

/*
  Get kernel config from extracted kernel path
  on web environment FS.
*/
std::string getIKConfig(uintptr_t ptr, size_t len)
{
  auto raw_data = reinterpret_cast<const uint8_t *>(ptr);

  if (len < 8)
  {
    return "";
  }

  std::vector<uint8_t> input;
  if (memcmp("ANDROID!", raw_data, 8) == 0)
  {
    const struct boot_img_hdr *hdr;

    hdr = reinterpret_cast<const boot_img_hdr *>(raw_data);
    uint32_t page_size = hdr->page_size;
    uint32_t kernel_offset = page_size;
    if (hdr->unused[0] >= 3)
      kernel_offset = 4096;
    if (hdr->unused[0] > 10)
      kernel_offset = page_size;

    emscripten_log(EM_LOG_DEBUG, "Kernel size: %d,Header Version: %d, Offset: %d\n",
                   hdr->kernel_size, hdr->unused[0], kernel_offset);

    std::vector<uint8_t> kernel_data(raw_data + kernel_offset, raw_data + kernel_offset + hdr->kernel_size);
    const compress_head *k_head;
    k_head = reinterpret_cast<const compress_head *>(kernel_data.data());
    auto method = detect_compress_method(*k_head);
    auto output = auto_decompress(kernel_data, 0, method);
    if (output.success)
    {
      input = output.data;
    }
    else
    {
      emscripten_log(EM_LOG_ERROR, "Failed: %s\n", output.error_msg);
      return "";
    }
  }
  else
  {
    const std::vector<uint8_t> data(raw_data, raw_data+len);
    const compress_head *k_head = reinterpret_cast<const compress_head *>(raw_data);
    auto method = detect_compress_method(*k_head);
    auto output = auto_decompress(data, 0, method);
    if (output.success)
    {
      input = output.data;
    }
    else
    {
      emscripten_log(EM_LOG_ERROR, "Failed: %s\n", output.error_msg);
      return "";
    }
  }

  if (input.size() == 0)
  {
    return "";
  }

  return dump_ikconfig(input);
}

EMSCRIPTEN_BINDINGS(kptools)
{
  function("getVersion", &getVersion);
  function("getIKConfig", &getIKConfig);
}

#endif
