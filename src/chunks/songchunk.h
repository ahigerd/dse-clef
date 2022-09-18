#ifndef SONGCHUNK_H
#define SONGCHUNK_H

#include "../dsechunk.h"

class SongChunk : public DSEChunkBase<SongChunk, 'song'>
{
public:
  SongChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);
  int16_t ticksPerBeat() const;
};

#endif
