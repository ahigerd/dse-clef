#include "songchunk.h"
#include "../dseutil.h"

SongChunk::SongChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: SongChunk::super(parent, buffer, offset)
{
  // initializers only
}

int16_t SongChunk::ticksPerBeat() const
{
  return parseInt<int16_t>(data, 0x12);
}
