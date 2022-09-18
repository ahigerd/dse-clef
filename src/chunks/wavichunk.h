#ifndef WAVICHUNK_H
#define WAVICHUNK_H

#include "../dsechunk.h"
#include <map>

struct SampleInfo
{
  uint32_t sampleRate, sampleStart, loopStart, loopLength;
  int8_t fineTune, coarseTune, volume, pan;
  uint8_t attackLevel, attackTime, decayTime, sustainLevel, holdTime, fadeTime, releaseTime;
  uint8_t rootKey, transpose, keygroup, envelopeMult, rateScale;
  enum Format : uint8_t {
    Pcm8,
    Pcm16,
    Adpcm,
    Unknown
  };
  Format format;
  bool loop : 1;
  bool envelope : 1;

  inline uint32_t byteLength() const { return 4 * (loopStart + loopLength); }
  ConstVectorSlice<uint8_t> data;
};

class WaviChunk : public DSEChunkBase<WaviChunk, 'wavi'>
{
public:
  WaviChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  std::map<uint16_t, SampleInfo> sampleInfo;

  virtual std::string debug(const std::string& prefix = std::string()) const;
};

#endif
