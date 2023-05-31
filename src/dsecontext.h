#ifndef D2W_DSECONTEXT_H
#define D2W_DSECONTEXT_H

#include "synth/synthcontext.h"
#include <map>
#include <set>
class DSEFile;
class PrgiChunk;
class WaviChunk;
class KgrpChunk;
class PcmdChunk;
class SongChunk;
class TrackChunk;
class Sample;
class Instrument;
class SplitInfo;

class DSEContext : public SynthContext
{
public:
  DSEContext(S2WContext* ctx, double sampleRate, std::unique_ptr<DSEFile> _smdl, std::unique_ptr<DSEFile> _swdl, std::shared_ptr<DSEFile> _bank);
  ~DSEContext();

  int loopSample;
  int sampleLength;
  uint32_t ticksPerBeat;

  std::unique_ptr<DSEFile> smdl, swdl;
  PrgiChunk *prgi, *bankPrgi;
  WaviChunk *wavi, *bankWavi;
  KgrpChunk *kgrp, *bankKgrp;
  PcmdChunk *pcmd, *bankPcmd;
  SongChunk *song;
  std::vector<TrackChunk*> tracks;

  std::map<int, int> tempos; // ({{ 0, 120 }}); // In the absence of tempo markings, default to 120bpm
  std::map<int, std::set<int>> allTimings;
  std::vector<double> channelGain;

  void prepareTimings();
  inline double samplesPerTick(int tempo) const { return (sampleRate * 60.0) / (ticksPerBeat * tempo); }
  const Sample* findSample(const SplitInfo& split);
  const Instrument* findInstrument(int presetId);

private:
  std::map<int, std::unique_ptr<Instrument>> instruments;
  std::map<int, std::unique_ptr<Sample>> samples;
  std::shared_ptr<DSEFile> bank;
};

#endif
