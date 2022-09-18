#include "mojibake.h"
#include <iostream>
#include <cstdint>

static const int sjisKanaBase = 0xA1, sjisKanaLast = 0xDF, utf8KanaBase = 0xFF61;
static const char* jisMap[2][94] = {
  {
                u8" ",      u8"\u3001", u8"\u3002", u8"\uFF0C", u8"\uFF0E", u8"\u00B7", u8"\uFF1A",
    u8"\uFF1B", u8"\uFF1F", u8"\uFF01", u8"\uFF9E", u8"\uFF9F", u8"\u00B4", u8"\uFF40", u8"\u00A8",
    u8"\uFF3E", u8"\uFFE3", u8"\uFF3F", u8"\u30FD", u8"\u30FE", u8"\u309D", u8"\u309E", u8"\u3003",
    u8"\u4EDD", u8"\u3005", u8"\u3006", u8"\u3007", u8"\u30FC", u8"\u2014", u8"\uFF0D", u8"\uFF0F",
    u8"\uff3c", u8"\u301c", u8"\u2016", u8"\uff5c", u8"\u2026", u8"\u2025", u8"\u2018", u8"\u2019",
    u8"\u201c", u8"\u201d", u8"\uff08", u8"\uff09", u8"\u3014", u8"\u3015", u8"\uff3b", u8"\uff3d",
    u8"\uff5b", u8"\uff5d", u8"\u3008", u8"\u3009", u8"\u300a", u8"\u300b", u8"\u300c", u8"\u300d",
    u8"\u300e", u8"\u300f", u8"\u3010", u8"\u3011", u8"\uff0b", u8"\uff0d", u8"\u00b1", u8"\u00d7",
    u8"\u00f7", u8"\uff1d", u8"\u2260", u8"\uff1c", u8"\uff1e", u8"\u2266", u8"\u2267", u8"\u221e",
    u8"\u2234", u8"\u2642", u8"\u2640", u8"\u00b0", u8"\u2032", u8"\u2033", u8"\u2103", u8"\uffe5",
    u8"\uff04", u8"\uffe0", u8"\uffe1", u8"\uff05", u8"\uff03", u8"\uff06", u8"\uff0a", u8"\uff20",
    u8"\u00a7", u8"\u2606", u8"\u2605", u8"\u25cb", u8"\u25cf", u8"\u25ce", u8"\u25c7"
  },
  {
                u8"\u25c6", u8"\u25a1", u8"\u25a0", u8"\u25b3", u8"\u25b2", u8"\u25bd", u8"\u25bc",
    u8"\u203b", u8"\u3012", u8"\u2192", u8"\u2190", u8"\u2191", u8"\u2193", u8"\u3013", u8"",
    u8"",       u8"",       u8"",       u8"",       u8"",       u8"",       u8"",       u8"",
    u8"",       u8"",       u8"\u2208", u8"\u220b", u8"\u2286", u8"\u2287", u8"\u2282", u8"\u2283",
    u8"\u222a", u8"\u2229", u8"",       u8"",       u8"",       u8"",       u8"",       u8"",
    u8"",       u8"",       u8"\u2227", u8"\u2228", u8"\u00ac", u8"\u21d2", u8"\u21d4", u8"\u2200",
    u8"\u2203", u8"",       u8"",       u8"",       u8"",       u8"",       u8"",       u8"",
    u8"",       u8"",       u8"",       u8"",       u8"\u2220", u8"\u22a5", u8"\u2312", u8"\u2202",
    u8"\u2207", u8"\u2261", u8"\u2252", u8"\u226a", u8"\u226b", u8"\u221a", u8"\u223d", u8"\u221d",
    u8"\u2235", u8"\u222b", u8"\u222c", u8"",       u8"",       u8"",       u8"",       u8"",
    u8"",       u8"",       u8"\u212b", u8"\u2030", u8"\u266f", u8"\u266d", u8"\u266a", u8"\u2020",
    u8"\u2021", u8"\u00b6", u8"",       u8"",       u8"",       u8"",       u8"\u25ef"
  }
  // Could have defined the other three planes we care about here but they're easier to do in code
};
static const char* asciiKana[] = {
  ".a",  "a", ".i",  "i", ".u",  "u", ".e",  "e", ".o",  "o",
  "ka", "ga", "ki", "gi", "ku", "gu", "ke", "ge", "ko", "go",
  "sa", "za", "shi", "ji", "su", "zu", "se", "ze", "so", "zo",
  "ta", "da", "chi", "dji", "\x01", "tsu", "du", "te", "de", "to", "do",
  "na", "ni", "nu", "ne", "no",
  "ha", "ba", "pa", "hi", "bi", "pi", "fu", "bu", "pu", "he", "be", "pe", "ho", "bo", "po",
  "ma", "mi", "mu", "me", "mo",
  ".ya", "ya", ".yu", "yu", ".yo", "yo",
  "ra", "ri", "ru", "re", "ro",
  ".wa", "wa", "wi", "we", "wo", "n",
  "vu", "ka", "ke"
};

bool isAscii(const std::string& str)
{
  for (char ch : str) {
    if (ch == '\0') {
      return true;
    } else if (ch < 0x20 || ch > 0x7e) {
      return false;
    }
  }
  return true;
}

bool isUtf8(const std::string& str)
{
  int len = str.size();
  int follow = 0;
  for (int i = 0; i < len; i++) {
    uint8_t ch = str[i];
    if (follow > 0) {
      if ((ch & 0xC0) != 0x80) {
        return false;
      }
      --follow;
      continue;
    }
    if (ch == '\0') {
      return true;
    } else if (ch < 0x20) {
      return false;
    } else if ((ch & 0xE0) == 0xC0) {
      follow = 1;
    } else if ((ch & 0xF0) == 0xE0) {
      follow = 2;
    } else if ((ch & 0xF8) == 0xF0) {
      follow = 3;
    } else if (ch & 0x80) {
      return false;
    }
  }
  return follow == 0;
}

std::string toUtf8(uint16_t utf16)
{
  return {
    char(0xE0 | ((utf16 & 0xF000) >> 12)),
    char(0x80 | ((utf16 & 0x0FC0) >> 6)),
    char(0x80 | (utf16 & 0x003F))
  };
}

uint16_t fromUtf8(const std::string& str, int* pos)
{
  uint8_t ch = str[*pos];
  uint16_t utf16 = 0;
  int follow;
  if ((ch & 0xE0) == 0xC0) {
    follow = 1;
    utf16 = ch & 0x1f;
  } else if ((ch & 0xF0) == 0xE0) {
    follow = 2;
    utf16 = ch & 0x0f;
  } else if ((ch & 0xF8) == 0xF0) {
    follow = 3;
    utf16 = ch & 0x07;
  } else {
    return ch;
  }
  while (follow > 0) {
    ++*pos;
    utf16 = (utf16 << 6) | uint8_t(str[*pos] & 0x3f);
    --follow;
  }
  return utf16;
}

// Caller must ensure it's a sjis multibyte sequence
static std::string sjisToUtf8(uint8_t high, uint8_t low)
{
  if (low < 0x40 || low == 0x7f || low > 0xfc) {
    // invalid low byte
    return "";
  }
  bool highEven = (low >= 0x9F);
  int jisHigh = (high - (high < 0xA0 ? 112 : 176)) * 2 - (highEven ? 0 : 1) - 0x20;
  if (jisHigh <= 0 || (jisHigh >= 9 && jisHigh <= 15) || jisHigh >= 85) {
    // Invalid high byte
    return "";
  }
  int jisLow = (highEven ? low - 126 : (low - 30) * 96 / 97) - 0x21;
  if (jisLow < 0 || jisLow > 0x5d) {
    // invalid low byte
    return "";
  }
  int utf16 = 0;
  switch (jisHigh) {
    case 1: return jisMap[0][jisLow];
    case 2: return jisMap[1][jisLow];
    case 3: utf16 = 0xFF12 + jisLow; break;
    case 4: utf16 = 0x3041 + jisLow; break;
    case 5: utf16 = 0x30a1 + jisLow; break;
    default: return "@"; // (probably) valid character not worth rendering
  }
  if ((utf16 >= 0x3094 && utf16 <= 0x30A0) ||
      (utf16 >= 0x30F7 && utf16 <= 0xFF11) ||
      (utf16 >= 0xFF1a && utf16 <= 0xFF20) ||
      (utf16 >= 0xFF3b && utf16 <= 0xFF40) ||
      (utf16 >= 0xFF5b)) {
    // Not in a validly mapped range
    return "";
  }
  return toUtf8(utf16);
}

std::string tryShiftJIS(const std::string& str)
{
  std::string result;
  int len = str.size();
  for (int i = 0; i < len; i++) {
    uint8_t ch = str[i];
    if (ch == '\0') {
      // Null terminator
      break;
    } else if (ch == '\\') {
      result += u8"\u00a5";
    } else if (ch == '~') {
      result += u8"\u00af";
    } else if (ch >= ' ' && ch <= '}') {
      // SJIS and ASCII match in this range
      result += ch;
    } else if (ch == 0x80 || ch == 0xA0 || ch >= 0xF0) {
      // Unmapped SJIS codepoint
      return "";
    } else if (ch >= sjisKanaBase && ch <= sjisKanaLast) {
      result += toUtf8(utf8KanaBase + (ch - sjisKanaBase));
    } else if (ch & 0x80) {
      // Multibyte SJIS sequence
      if (i == len - 1) {
        // The low byte is missing, but there's a good chance
        // it was just truncated, so let's truncate too.
        break;
      }
      std::string utf8 = sjisToUtf8(ch, str[i + 1]);
      if (utf8.size() == 0) {
        return "";
      }
      ++i;
      result += utf8;
    } else {
      // Mapped but unprintable means it's probably not SJIS
      return "";
    }
  }
  return result;
}

std::string forceSafeAscii(const std::string& str)
{
  std::string result;
  for (char ch : str) {
    if (ch < '0' || ch > 'z' || (ch > '9' && ch < '@') || ch == '\\' || ch == '`') {
      result += '_';
    } else {
      result += ch;
    }
  }
  return result;
}

// Caller is responsible for ensuring UTF-8
std::string transliterateRomaji(const std::string& str)
{
  std::string result;
  int len = str.size();
  for (int i = 0; i < len; i++) {
    uint16_t utf16 = fromUtf8(str, &i);
    if (utf16 < 0x7f) {
      // ascii
      result += uint8_t(utf16);
    } else if ((utf16 >= 0x3041 && utf16 <= 0x3093) || (utf16 >= 0x30a1 && utf16 <= 0x30f4)) {
      // hiragana / katakana
      bool katakana = utf16 >= 0x30a1;
      std::string romaji = asciiKana[utf16 - (katakana ? 0x30a1 : 0x3041)];
      if (katakana) {
        int rlen = romaji.size();
        for (int j = 0; j < rlen; j++) {
          if (romaji[j] >= 'a' && romaji[j] <= 'z') {
            romaji[j] -= ('a' - 'A');
          }
        }
      }
      if (result.size() > 0 && result[result.size() - 1] == '\x01') {
        if (romaji[0] == '.' || romaji[0] == '\x01') {
          result[result.size() - 1] = '_';
        } else {
          result[result.size() - 1] = romaji[0];
        }
      }
      if (romaji[0] != '.') {
        result += romaji;
        continue;
      }
      romaji = romaji.substr(1);
      if (result.size() < 2) {
        // string starts with a small character or a vowel
        result += romaji;
      } else {
        std::string tail = result.substr(result.size() - 2);
        if (romaji[2] == 'y' || romaji[2] == 'Y') {
          bool properGlide = false;
          if ((tail == "hi" || tail == "HI") && result.size() >= 3) {
            char prev = result[result.size() - 3];
            if (prev == 's' || prev == 'S' || prev == 'c' || prev == 'C') {
              properGlide = true;
            }
          } else if (tail == "ji" || tail == "JI") {
            properGlide = true;
          }
          if (properGlide) {
            // shi + ya -> sha, not shya
            result[result.size() - 1] = romaji[romaji.size() - 1];
            continue;
          }
        }
        if (tail == "fu" || tail == "FU" || tail == "vu" || tail == "VU") {
          // fu + a -> fa
          result.pop_back();
        }
        result += romaji;
      }
    } else if (utf16 >= 0xff00) {
      // fullwidth ascii
      result += uint8_t(utf16 - 0xfee0);
    } else if (utf16 == 0x30fc) {
      if (result.size() == 0) {
        result += "-";
        continue;
      }
      char ch = result[result.size() - 1];
      if (ch == 'a' || ch == 'A' || ch == 'e' || ch == 'E' || ch == 'i' ||
          ch == 'I' || ch == 'o' || ch == 'O' || ch == 'u' || ch == 'U') {
        result += ch;
      } else {
        result += "-";
      }
    } else {
      // unprintable
      result += "_";
    }
  }
  return result;
}
