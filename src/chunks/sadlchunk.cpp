#include "sadlchunk.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include "codec/procyoncodec.h"
#include "codec/adpcmcodec.h"
#include <exception>
#include <iostream>

SadlChunk::SadlChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: SadlChunk::super(parent, buffer, offset)
{
  offset = 0;
  uint32_t sampleLength = parseInt<uint32_t>(data, offset + 0x40) - 0x100;
  uint8_t sampleType = data[offset + 0x33] & 0x06;
  stereo = data[offset + 0x32] > 1;
  sampleRate = sampleType * 8182;
  if (!sampleRate) sampleRate = 16384;
  uint8_t codec = data[offset + 0x33] & 0xf0;
  offset += 0x100;
  if (codec == 0x70 || codec == 0x00) {
    AdpcmCodec codec(parent->context(), AdpcmCodec::NDS, stereo ? 0x10 : 0);
    sample = codec.decodeRange(data.begin() + offset, data.begin() + offset + sampleLength, uint64_t(offset) << 32);
  } else if (codec == 0xb0) {
    ProcyonCodec codec(parent->context(), stereo);
    sample = codec.decodeRange(data.begin() + offset, data.begin() + offset + sampleLength, uint64_t(offset) << 32);
  } else {
    throw std::runtime_error("unknown codec in SADL data");
  }
}
