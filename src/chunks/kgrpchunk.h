#ifndef KGRPCHUNK_H
#define KGRPCHUNK_H

#include "../dsechunk.h"
#include <vector>

struct KeyGroup
{
  uint8_t polyphony, priority;
  uint8_t minChannel, maxChannel;
};

class KgrpChunk : public DSEChunkBase<KgrpChunk, 'kgrp'>
{
public:
  KgrpChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  std::vector<KeyGroup> groups;
};

#endif
