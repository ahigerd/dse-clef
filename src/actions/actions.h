#ifndef ACTIONS_H
#define ACTIONS_H

#include <vector>
#include <string>
#include "commandargs.h"
class S2WContext;

using ActionFn = bool (*)(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool listChunks(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpSamples(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool dumpStreams(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool extractDSE(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);
bool synthSequence(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args);

#endif
