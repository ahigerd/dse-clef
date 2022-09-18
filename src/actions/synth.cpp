#include "actions.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include "riffwriter.h"
#include "../dsecontext.h"
#include "../chunks/trackchunk.h"
#include "synth/track.h"
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

// TODO: pass sample rate
bool synthSequence(S2WContext* ctx, const std::vector<std::string>& paths, const std::string& outputPath, const CommandArgs& args)
{
  if (paths.size() < 1) {
    return false;
  }
  double sampleRate = ARM7_CLOCK / 1024.0;
  bool didSomething = false;

  // Check to see if the last path is a sound bank
  std::string bankPath = paths[paths.size() - 1];
  std::unique_ptr<DSEFile> bankFile(new DSEFile(ctx, bankPath));
  if (!bankFile->findChunk('wavi') &&
      !bankFile->findChunk('prgi') &&
      !bankFile->findChunk('kgrp') &&
      !bankFile->findChunk('pcmd')) {
    // It's not, so unload it
    bankFile.reset(nullptr);
    bankPath.clear();
  }

  for (const std::string& path : paths) {
    if (path == bankPath) {
      // TODO: this will be incorrect for all-in-one files
      continue;
    }
    // Look for a paired sound bank
    std::unique_ptr<DSEFile> pairFile;
    try {
      std::string pairPath = path.substr(0, path.size() - 4) + ".swd";
      pairFile.reset(new DSEFile(ctx, pairPath));
    } catch (...) {
      // If the paired file doesn't exist or can't be parsed, it'll throw
    }
    std::unique_ptr<DSEFile> dseFile(new DSEFile(ctx, path));
    DSEContext context(ctx, sampleRate, std::move(dseFile), std::move(pairFile), bankFile.get());
#ifndef _WIN32
    if (outputPath != "-")
#endif
    {
      if (!mkdirIfNeeded(outputPath)) {
        throw std::runtime_error("Unable to create output path '" + outputPath + "'");
      }
    }

    context.prepareTimings();
    std::vector<Track> tracks;
    std::vector<bool> trackDone;
    int trackCount = context.tracks.size();
    int tracksLeft = trackCount;
    int tickPos = 0;

    int64_t progress = 0;
    int percent = 0;
    std::cerr << "Progress: 0%" << std::flush;
    tracks.reserve(context.tracks.size());
    for (TrackChunk* track : context.tracks) {
      tracks.emplace_back(track, &context);
      context.addChannel(&tracks[tracks.size() - 1]);
      trackDone.push_back(false);
    }

#if 0
    int64_t progressMax = int64_t(context.sampleLength) * tracksLeft;
    while (tracksLeft > 0) {
      int newTickPos = tickPos;
      for (int i = 0; i < trackCount; i++) {
        Track& track = tracks[i];
        while (!trackDone[i] && track.tickPos <= tickPos) {
          int beforeSamplePos = track.samplePos;
          trackDone[i] = track.processEvent(samples);
          progress += track.samplePos - beforeSamplePos;
          if (trackDone[i]) {
            --tracksLeft;
          }
        }
        int newPercent = (progress * 100) / progressMax;
        if (newPercent > percent) {
          std::cerr << "\rProgress: " << percent << "%" << std::flush;
          percent = newPercent;
        }
        if (track.tickPos > newTickPos) {
          newTickPos = track.tickPos;
        }
      }
      tickPos = newTickPos;
    }
#endif

    int sampleLength = context.sampleLength;
    if (context.loopSample >= 0) {
      int oldSize = context.sampleLength;
      int loopSample = context.loopSample;
      int loopLength = oldSize - loopSample;
      int fadeLength = int(5.0 / context.sampleTime);
      if (fadeLength > loopLength) {
        fadeLength = loopLength;
      }
      sampleLength = oldSize + loopLength + fadeLength + 1;
    }
    int slashPos = path.find_last_of('/');
    if (slashPos == std::string::npos) {
      slashPos = -1;
    }
    int dotPos = path.find_last_of('.');
    std::string filename = path.substr(slashPos + 1, dotPos == std::string::npos ? dotPos : dotPos - slashPos - 1);
#ifndef _WIN32
    if (outputPath == "-") {
      filename = "/dev/stdout";
    } else
#endif
    {
      filename = outputPath + "/" + filename + ".wav";
    }
    std::cerr << "\rWriting " << filename << "..." << std::endl;
    // TODO: ensure length accuracy
    RiffWriter riff(sampleRate, true, sampleLength * 2);
    riff.open(filename);
    context.save(&riff);
    riff.close();
    didSomething = true;
  }
  return didSomething;
}
