#ifndef SADLCHUNK_H
#define SADLCHUNK_H

#include "../dsechunk.h"
class SampleData;

class SadlChunk : public DSEChunkBase<SadlChunk, 'sadl'>
{
public:
  SadlChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  uint32_t sampleRate;
  bool stereo;
  SampleData* sample;
};

#endif
