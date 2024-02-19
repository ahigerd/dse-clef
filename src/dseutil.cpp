#include "dseutil.h"
#include "clefcontext.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <exception>
#include <cstdlib>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#ifndef W_OK
#define W_OK 2
#endif
#else
#include <unistd.h>
#endif

template <typename T>
std::string hexdumpToString(const std::vector<T>& buffer, const std::string& prefix)
{
  const int wordlen = sizeof(T) * 2;
  std::string result;
  int cols = 0;
  for (T ch : buffer) {
    std::ostringstream ss;
    int len = ss.tellp();
    ss << std::hex << std::setfill('0') << std::setw(wordlen) << uint32_t(ch);
    len = int(ss.tellp()) - len;
    result += ((cols % 32) ? " " : "\n" + prefix) + ss.str().substr(len - wordlen);
    cols += wordlen;
  }
  return result.substr(1);
}

template std::string hexdumpToString<char>(const std::vector<char>& buffer, const std::string& prefix);
template std::string hexdumpToString<uint8_t>(const std::vector<uint8_t>& buffer, const std::string& prefix);
template std::string hexdumpToString<int8_t>(const std::vector<int8_t>& buffer, const std::string& prefix);
template std::string hexdumpToString<int16_t>(const std::vector<int16_t>& buffer, const std::string& prefix);

void hexdump(const std::vector<uint8_t>& buffer, int limit)
{
  int size = buffer.size();
  int offset = 0;
  while (offset < size && (limit < 0 || offset < limit)) {
    int lineStart = offset;
    int i;
    std::string printable;
    for (i = 0; i < 16; i++) {
      if (offset < size) {
        std::printf("%02x ", uint32_t(uint8_t(buffer[offset])));
        printable += char((buffer[offset] >= 0x20 && buffer[offset] < 0x7F) ? buffer[offset] : '.');
      } else {
        std::printf("   ");
      }
      ++offset;
    }
    std::printf("%s\n", printable.c_str());
  }
}

bool mkdirIfNeeded(const std::string& path) {
  if (::access(path.c_str(), W_OK) != 0) {
#ifdef _WIN32
    return ::mkdir(path.c_str()) == 0;
#else
    return ::mkdir(path.c_str(), 0755) == 0;
#endif
  }
  return true;
}

std::vector<uint8_t> readFile(ClefContext* ctx, const std::string& filename)
{
  auto fptr = ctx->openFile(filename);
  if (!fptr || !*fptr) {
    throw std::runtime_error("unable to read " + filename);
  }
  return readFile(*fptr);
}

std::vector<uint8_t> readFile(std::istream& file, const std::string& filename)
{
  auto startPos = file.tellg();
  file.seekg(0, std::ios::end);
  auto endPos = file.tellg();
  file.seekg(0, std::ios::beg);
  int fileSize = endPos - startPos;
  std::vector<uint8_t> buffer(fileSize);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
  if (!file) {
    throw std::runtime_error("error while reading " + (filename.size() ? filename : "file"));
  }
  return buffer;
}
