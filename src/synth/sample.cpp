#include "sample.h"
#include "dsecontext.h"
#include "codec/sampledata.h"
#include "../chunks/wavichunk.h"
#include "../chunks/prgichunk.h"
#include "../chunks/pcmdchunk.h"
#include "../chunks/trackchunk.h"
#include <iostream>

Sample::Sample(const SplitInfo* split, const SampleInfo* info, SampleData* pcm, DSEContext* synth)
: sampleInfo(info), context(synth), sample(pcm)
{
  if (!info || !sample) {
    return;
  }
  double pitch, tune;
  if (split) {
    pitch = double(split->rootKey);
    //tune = (split->coarseTune - sampleInfo->coarseTune) + ((split->fineTune - sampleInfo->fineTune) / 255.0);
    //tune = split->transpose + (split->fineTune / 255.0);
    tune = 0;
  } else {
    pitch = double(sampleInfo->rootKey);
    tune = 0;
  }
  int sr = sampleInfo->sampleRate;
  pcm->sampleRate = sr;
  //double baseFreq = TrkEvent::frequency(pitch, tune) * context->sampleRate / sr;
  //cycleWidth = sr / baseFreq; // * sampleInfo->rateScale;
  int loopLength;
  switch (sampleInfo->format) {
  case SampleInfo::Pcm8:
    pcm->loopStart = info->loopStart * 4;
    loopLength = info->loopLength * 4;
    break;
  case SampleInfo::Pcm16:
    pcm->loopStart = info->loopStart * 2;
    loopLength = info->loopLength * 2;
    break;
  case SampleInfo::Adpcm:
    pcm->loopStart = info->loopStart * 8 - 8;
    loopLength = info->loopLength * 8;
    break;
  default:
    // unknown format
    sample = nullptr;
    return;
  }
  pcm->loopEnd = pcm->loopStart + loopLength;
  //sampleLength = loopStart + loopLength;
  /*
  std::cerr << "sample start ls=" << info->loopStart << " ll=" << info->loopLength << " p=" << pitch << " sr=" << sr << " bf=" << baseFreq << " cw=" << cycleWidth
    << " f=" << TrkEvent::frequency(pitch, tune) << std::endl;
    */
}
