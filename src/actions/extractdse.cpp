#include "actions.h"
#include "../mojibake.h"
#include "../dsefile.h"
#include "../dseutil.h"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <set>

static const char spinner[] = "|/-\\";

bool extractDSE(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  bool romaji = args.hasKey("romaji");
  if (!std::filesystem::create_directories(std::filesystem::path(outputPath))) {
    std::cerr << "Unable to create directories for " << outputPath << std::endl;
    return false;
  }
  for (const std::string& path : paths) {
    std::cout << "Scanning " << path << "..." << std::endl;
    std::vector<uint8_t> buffer = readFile(ctx, path);
    int end = buffer.size() - 64;
    bool foundFile = false;
    int lastProgress = -1;
    int spinnerFrame = 0;
    std::set<std::string> usedNames;
    for (int i = 0; i < end; i++) {
      int progress = (double(i) / end * 100);
      if (progress != lastProgress) {
        lastProgress = progress;
        std::cout << "\rProgress: " << std::setw(3) << progress << "% " << spinner[spinnerFrame];
      } else if (i % 64 == 0) {
        std::cout << "\b" << spinner[spinnerFrame];
        spinnerFrame = (spinnerFrame + 1) % 4;
      }
      if (buffer[i] == 0) {
        continue;
      }
      uint64_t magic64 = parseIntBE<uint64_t>(buffer, i);
      if (!isValidMagic64(magic64) || magic64 & 0xFFFF) {
        continue;
      }
      try {
        DSEFile dseFile(ctx, buffer, i);
        std::string filename = dseFile.originalFilename();
        if (!filename.size()) {
          filename = std::to_string(i);
        }
        int dotPos = filename.find('.');
        if (dotPos != std::string::npos) {
          filename = filename.substr(0, dotPos);
        }
        if (!isUtf8(filename)) {
          std::string sjisFilename = tryShiftJIS(filename);
          if (sjisFilename.size() == 0 || !isUtf8(sjisFilename)) {
            filename = forceSafeAscii(filename);
          } else {
            filename = sjisFilename;
          }
        }
        if (romaji) {
          filename = transliterateRomaji(filename);
        }
        std::string baseFilename = filename;
        std::string extension = "." + magicString(dseFile.magic()).substr(0, 3);
        filename += extension;
        char counter = '1';
        while (usedNames.count(filename)) {
          filename = baseFilename + "_" + counter + extension;
          ++counter;
        }
        usedNames.insert(filename);
        filename = outputPath + "/" + filename;
        std::ofstream outputFile(filename);
        std::cout << "\rWriting " << filename << std::endl;
        outputFile.write(reinterpret_cast<const char*>(&buffer[i]), dseFile.fileLength());
        i += dseFile.fileLength() - 1;
        foundFile = true;
      } catch (std::exception& err) {
        // consume errors silently because they indicate that there wasn't a (good) file there
        if (std::string(err.what()).find("unknown file type") == std::string::npos) {
          std::cout << "With " << magicString(magic64) << " header: " << err.what() << std::endl;
        }
      }
    }
    if (!foundFile) {
      std::cout << "\rFound no DSE files in '" << path << "'" << std::endl;
    } else {
      std::cout << "\rProgress: 100%    " << std::endl;
    }
  }
  return paths.size() > 0;
}
