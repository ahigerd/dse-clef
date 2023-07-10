#include "actions.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include "riffwriter.h"
#include "../dsecontext.h"
#include "../chunks/trackchunk.h"
#include "../synth/instrument.h"
#include "synth/track.h"
#include "s2wcontext.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>

#define ARM7_CLOCK 33513982

static std::string makeRelative(const std::string& base, const std::string& path)
{
  int pathSlashPos = path.find_last_of(PATH_CHARS);
  if (pathSlashPos != std::string::npos) {
    // already absolute path
    return path;
  }
  int slashPos = base.find_last_of(PATH_CHARS);
  if (slashPos == std::string::npos) {
    // no path in base
    return path;
  }
  return base.substr(0, slashPos + 1) + path;
}

DSEContext* prepareSynthContext(S2WContext* ctx, std::istream& inputFile, const std::string& inputPath, const std::string& pairPath, const std::string& bankPath, const CommandArgs* args)
{
  double sampleRate = ARM7_CLOCK / 1024.0;

  // Check to see if the last path is a sound bank
  std::shared_ptr<DSEFile> bankFile;
  if (!bankPath.empty()) {
    bankFile.reset(new DSEFile(ctx, makeRelative(inputPath, bankPath)));
    if (!bankFile->findChunk('wavi') &&
        !bankFile->findChunk('prgi') &&
        !bankFile->findChunk('kgrp') &&
        !bankFile->findChunk('pcmd')) {
      // It's not, so unload it
      bankFile = nullptr;
    }
  }

  // Look for a paired sound bank
  std::unique_ptr<DSEFile> pairFile;
  try {
    std::string path = pairPath.empty() ? inputPath.substr(0, inputPath.size() - 4) + ".swd" : pairPath;
    pairFile.reset(new DSEFile(ctx, makeRelative(inputPath, path)));
  } catch (...) {
    // If the paired file doesn't exist or can't be parsed, it'll throw
  }
  std::unique_ptr<DSEFile> dseFile(new DSEFile(ctx, readFile(inputFile), 0));
  std::unique_ptr<DSEContext> context(new DSEContext(ctx, sampleRate, std::move(dseFile), std::move(pairFile), bankFile));

  if (ctx->isDawPlugin) {
    for (int i = 0; i < 256; i++) {
      const Instrument* inst = context->findInstrument(i);
      if (inst) {
        context->registerInstrument(i, std::unique_ptr<IInstrument>(new Instrument(*inst)));
      }
    }
  } else {
    context->prepareTimings();

    for (TrackChunk* track : context->tracks) {
      context->addChannel(new Track(track, context.get()));
    }
  }

  return context.release();
}

// TODO: pass sample rate
bool synthSequenceToFile(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  if (paths.size() < 1) {
    return false;
  }
  std::string bankPath, pairPath;
  if (paths.size() > 1) {
    bankPath = paths[paths.size() - 1];
  }
  if (paths.size() > 2) {
    pairPath = paths[1];
  }
  auto inputFile = ctx->openFile(paths[0]);
  std::unique_ptr<DSEContext> context(prepareSynthContext(ctx, *inputFile, paths[0], pairPath, bankPath, &args));
  if (!context) {
    return false;
  }

  bool didSomething = false;

#ifndef _WIN32
  if (outputPath != "-")
#endif
  {
    if (!mkdirIfNeeded(outputPath)) {
      throw std::runtime_error("Unable to create output path '" + outputPath + "'");
    }
  }

  std::vector<bool> trackDone;
  int trackCount = context->tracks.size();
  int tracksLeft = trackCount;

  int sampleLength = context->sampleLength;
  if (context->loopSample >= 0) {
    int oldSize = context->sampleLength;
    int loopSample = context->loopSample;
    int loopLength = oldSize - loopSample;
    int fadeLength = int(5.0 / context->sampleTime);
    if (fadeLength > loopLength) {
      fadeLength = loopLength;
    }
    sampleLength = oldSize + loopLength + fadeLength + 1;
  }
  std::string filename = outputPath;
#ifndef _WIN32
  if (outputPath == "-") {
    filename = "/dev/stdout";
  }
#endif
  std::cerr << "\rWriting " << filename << "..." << std::endl;
  // TODO: ensure length accuracy
  RiffWriter riff(context->sampleRate, true, sampleLength * 2);
  riff.open(filename);
  //context.seek(38.0);
  context->save(&riff);
  riff.close();
  return true;
}
