#ifndef DSE_DSEUTIL_H
#define DSE_DSEUTIL_H

#include <vector>
#include <string>
#include <cstdint>
#include "utility.h"
class ClefContext;

inline std::string magicString(uint32_t magic) { return fourccToString(magic); }
inline std::string magicString(uint64_t magic) { return fourccToString(uint32_t(magic >> 32)); }

inline constexpr bool isValidMagic64(uint64_t magic) {
  // This is technically too permissive, but it hasn't failed on
  // any known files yet.
  return
    !(magic & 0x80808080FFFF0000) &&  // 4 chars < 128, 2 nulls
    (magic & 0x6000000000000000) &&   // 1st char is printable
    (magic & 0x0060000000000000) &&   // 2nd char is printable
    (magic & 0x0000600000000000) &&   // 3rd char is printable
    (magic & 0x0000006000000000);     // 4th char is printable
}

void hexdump(const std::vector<uint8_t>& buffer, int limit = -1);
template <typename T>
std::string hexdumpToString(const std::vector<T>& buffer, const std::string& prefix = std::string());

/* Filesystem I/O */

bool mkdirIfNeeded(const std::string& path);
std::vector<uint8_t> readFile(ClefContext* ctx, const std::string& filename);
std::vector<uint8_t> readFile(std::istream& file, const std::string& filename = std::string());

#endif
