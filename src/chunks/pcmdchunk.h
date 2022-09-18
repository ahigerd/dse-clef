#ifndef PCMDCHUNK_H
#define PCMDCHUNK_H

#include "../dsechunk.h"
#include "../vectorslice.h"
#include <map>
class SampleInfo;
class SampleData;

class PcmdChunk : public DSEChunkBase<PcmdChunk, 'pcmd'>
{
public:
  PcmdChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  ConstVectorSlice<uint8_t> getRawSample(uint32_t offset, uint32_t length) const;
  ConstVectorSlice<uint8_t> getRawSample(const SampleInfo& info) const;
  SampleData* getSample(uint8_t format, uint32_t offset, uint32_t length);
  SampleData* getSample(const SampleInfo& info);
};

#endif
