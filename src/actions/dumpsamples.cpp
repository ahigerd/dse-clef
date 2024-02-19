#include "actions.h"
#include "riffwriter.h"
#include "codec/sampledata.h"
#include "../dsefile.h"
#include "../dseutil.h"
#include "../chunks/wavichunk.h"
#include "../chunks/pcmdchunk.h"
#include <memory>
#include <iostream>
#include <sstream>
#include <exception>

bool dumpSamples(ClefContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  bool raw = args.hasKey("raw");
  DSEFile dseFile(ctx, paths[0]);
  std::unique_ptr<DSEFile> soundBank;
  if (paths.size() > 1) {
    soundBank.reset(new DSEFile(ctx, paths[1]));
  }

  WaviChunk* wavi = dseFile.findChunk<WaviChunk>();
  PcmdChunk* pcmd = soundBank ? soundBank->findChunk<PcmdChunk>() : dseFile.findChunk<PcmdChunk>();
  if (wavi && pcmd) {
    if (!mkdirIfNeeded(outputPath)) {
      throw std::runtime_error("Unable to create output path '" + outputPath + "'");
    }
    for (auto iter : wavi->sampleInfo) {
      const SampleInfo& info = iter.second;
      std::ostringstream sstr;
      if (raw) {
        std::vector<uint8_t> sample = pcmd->getRawSample(info);
        sstr << outputPath << "/raw" << iter.first << ".";
        if (info.format == SampleInfo::Pcm8) {
          sstr << "pcm8";
        } else if (info.format == SampleInfo::Pcm16) {
          sstr << "pcm16";
        } else if (info.format == SampleInfo::Adpcm) {
          sstr << "adpcm";
        } else {
          sstr << "bin";
        }
        std::string outputFilename = sstr.str();
        std::cout << "Writing " << outputFilename << std::endl;
        std::ofstream file(outputFilename, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(&sample[0]), sample.size());
        file.close();
      } else {
        SampleData* sample = pcmd->getSample(iter.first, info);
        sstr << outputPath << "/sample" << iter.first << ".wav";
        std::string outputFilename = sstr.str();
        std::cout << "Writing " << outputFilename << std::endl;
        bool stereo = sample->channels.size() == 2;
        RiffWriter riff(info.sampleRate, stereo);
        riff.open(outputFilename);
        if (stereo) {
          riff.write(sample->channels[0], sample->channels[1]);
        } else {
          riff.write(sample->channels[0]);
        }
        riff.close();
      }
    }
  } else if (wavi && !pcmd) {
    std::cout << "Unable to find sample bank. Samples identified:" << std::endl;
    for (auto iter : wavi->sampleInfo) {
      std::cout << "\tsample " << iter.first << " offset " << iter.second.sampleStart << std::endl;
    }
  }
  return true;
}
