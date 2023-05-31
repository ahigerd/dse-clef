#ifndef ACTIONS_H
#define ACTIONS_H

#include <vector>
#include <string>
#include <iostream>
#include "commandargs.h"
class S2WContext;
class DSEContext;

using ActionFn = bool (*)(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool listChunks(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpSamples(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpStreams(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool extractDSE(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
DSEContext* prepareSynthContext(S2WContext* ctx, std::istream& inputFile, const std::string& inputPath, const std::string& pairPath, const std::string& bankPath, const CommandArgs* args);
bool synthSequenceToFile(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);

#endif
