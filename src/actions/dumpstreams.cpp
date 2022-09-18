#include "actions.h"
#include "riffwriter.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include "../chunks/sadlchunk.h"
#include "codec/sampledata.h"
#include <iostream>

bool dumpStreams(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  for (const std::string& path : paths) {
    if (!mkdirIfNeeded(outputPath)) {
      throw std::runtime_error("Unable to create output path '" + outputPath + "'");
    }
    DSEFile dseFile(ctx, path);
    DSEChunk* chunk = dseFile.findChunk('sadl');
    SadlChunk* sadl = chunk ? chunk->cast<SadlChunk>() : nullptr;
    if (!sadl) {
      throw std::runtime_error("Not a SADL file: " + path);
    }
    std::string filename = dseFile.originalFilename();
    int dotPos = filename.find('.');
    if (dotPos != std::string::npos) {
      filename = filename.substr(0, dotPos);
    }
    filename = outputPath + "/" + filename + ".wav";
    std::cout << "Writing " << filename << std::endl;
    RiffWriter riff(sadl->sampleRate, sadl->stereo);
    riff.open(filename);
    if (sadl->stereo) {
      riff.write(sadl->sample->channels[0], sadl->sample->channels[1]);
    } else {
      riff.write(sadl->sample->channels[0]);
    }
    riff.close();

  }
  return true;
}
