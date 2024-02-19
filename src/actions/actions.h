#ifndef ACTIONS_H
#define ACTIONS_H

#include <vector>
#include <string>
#include <iostream>
#include "commandargs.h"
class ClefContext;
class DSEContext;

using ActionFn = bool (*)(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool listChunks(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpSamples(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpStreams(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool extractDSE(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
DSEContext* prepareSynthContext(ClefContext* ctx, std::istream& inputFile, const std::string& inputPath, const std::string& pairPath, const std::string& bankPath, const CommandArgs* args);
bool synthSequenceToFile(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);

#endif
