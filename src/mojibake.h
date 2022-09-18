#ifndef MOJIBAKE_H
#define MOJIBAKE_H

#include <string>

bool isAscii(const std::string& str);
bool isUtf8(const std::string& str);
std::string tryShiftJIS(const std::string& str);
std::string forceSafeAscii(const std::string& str);
std::string transliterateRomaji(const std::string& str);

#endif
