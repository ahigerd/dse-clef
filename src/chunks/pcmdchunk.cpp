#include "pcmdchunk.h"
#include "wavichunk.h"
#include "codec/pcmcodec.h"
#include "codec/adpcmcodec.h"
#include "codec/sampledata.h"
#include "../dsefile.h"
#include "../dseutil.h"
#include "s2wcontext.h"
#include <exception>

REGISTER_CHUNK(PcmdChunk)

PcmdChunk::PcmdChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: PcmdChunk::super(parent, buffer, offset)
{
  // initializers only
}

ConstVectorSlice<uint8_t> PcmdChunk::getRawSample(uint32_t offset, uint32_t length) const
{
  return ConstVectorSlice<uint8_t>(data, offset + 0x10, length);
}

ConstVectorSlice<uint8_t> PcmdChunk::getRawSample(const SampleInfo& info) const
{
  return getRawSample(info.sampleStart, info.byteLength());
}

SampleData* PcmdChunk::getSample(uint64_t sampleID, uint8_t format, uint32_t offset, uint32_t length)
{
  SampleData* sample = parent()->context()->getSample(sampleID);
  if (sample) {
    return sample;
  }
  offset += 0x10;
  if (format == SampleInfo::Pcm8) {
    PcmCodec codec(parent()->context(), 8);
    return codec.decode(ConstVectorSlice<uint8_t>(data, offset, length), sampleID);
  } else if (format == SampleInfo::Pcm16) {
    // TODO: This assumes that PCM16 samples are stored little-endian
    PcmCodec codec(parent()->context(), 16, 1, false);
    return codec.decode(ConstVectorSlice<uint8_t>(data, offset, length), sampleID);
  } else if (format == SampleInfo::Adpcm) {
    AdpcmCodec codec(parent()->context(), AdpcmCodec::NDS);
    return codec.decode(ConstVectorSlice<uint8_t>(data, offset, length), sampleID);
  } else {
    throw std::runtime_error("unknown sample format " + std::to_string(format));
  }
}

SampleData* PcmdChunk::getSample(uint64_t sampleID, const SampleInfo& info)
{
  return getSample(sampleID, info.format, info.sampleStart, info.byteLength());
}
