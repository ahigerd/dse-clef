#ifndef PCMDCHUNK_H
#define PCMDCHUNK_H

#include "../dsechunk.h"
#include "../vectorslice.h"
#include <map>
struct SampleInfo;
struct SampleData;

class PcmdChunk : public DSEChunkBase<PcmdChunk, 'pcmd'>
{
public:
  PcmdChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  ConstVectorSlice<uint8_t> getRawSample(uint32_t offset, uint32_t length) const;
  ConstVectorSlice<uint8_t> getRawSample(const SampleInfo& info) const;
  SampleData* getSample(uint64_t sampleID, uint8_t format, uint32_t offset, uint32_t length);
  SampleData* getSample(uint64_t sampleID, const SampleInfo& info);
};

#endif
