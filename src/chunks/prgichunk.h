#ifndef PRGICHUNK_H
#define PRGICHUNK_H

#include "../dsechunk.h"
#include <map>
#include <vector>

struct LFOInfo {
  uint16_t delay;
  int16_t rate;
  uint16_t depth;
  int16_t fade;
  uint16_t cutoff;

  enum Route : uint8_t {
    Disabled,
    Pitch,
    Volume,
    Pan,
    Filter
  };
  Route route;

  enum Waveform : uint8_t {
    NoWave,
    Square,
    Triangle,
    Sine,
    Unknown,
    Saw,
    Noise,
    Random
  };
  Waveform waveform;
};

struct SplitInfo {
  uint16_t sampleIndex;
  int8_t lowKey, highKey, lowVelocity, highVelocity, fineTune, coarseTune, rootKey, transpose;
  uint8_t volume, pan, attackLevel, attackTime, decayTime, sustainLevel, holdTime, fadeTime, releaseTime;
  uint8_t keygroup, envelopeMult, bendRange;
  bool envelope : 1;
  ConstVectorSlice<uint8_t> data;
};

struct ProgramInfo {
  int8_t id, gain, pan;
  std::vector<LFOInfo> lfos;
  std::vector<SplitInfo> splits;
};

class PrgiChunk : public DSEChunkBase<PrgiChunk, 'prgi'>
{
public:
  PrgiChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  std::map<uint8_t, ProgramInfo> programInfo;

  virtual std::string debug(const std::string& prefix = std::string()) const;
};

#endif
