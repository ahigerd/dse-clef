#include "wavichunk.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include <sstream>
#include <iomanip>

WaviChunk::WaviChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: WaviChunk::super(parent, buffer, offset)
{
  //hexdump(data);
  int slots = parent->sampleCount();
  offset = 0x10 + (slots * 2);
  if (offset & 0x0F) {
    offset = (offset & ~0x0F) + 0x10;
  }
  int dataSize = data.size();
  for (int i = 0; i < slots && offset < dataSize; i++, offset += 64) {
    uint16_t index = parseInt<uint16_t>(data, offset + 2);
    SampleInfo info;
    info.data = ConstVectorSlice<uint8_t>(data, offset, 64);
    info.fineTune = data[offset + 0x04];
    info.coarseTune = data[offset + 0x05];
    info.rootKey = data[offset + 0x06];
    info.transpose = data[offset + 0x07];
    info.volume = data[offset + 0x08];
    info.pan = data[offset + 0x09];
    info.keygroup = data[offset + 0x0a];
    info.format = SampleInfo::Format(data[offset + 0x13]);
    info.loop = data[offset + 0x15] != 0;
    info.rateScale = data[offset + 0x1a];
    info.sampleRate = parseInt<uint32_t>(data, offset + 0x20);
    info.sampleStart = parseInt<uint32_t>(data, offset + 0x24);
    info.loopStart = parseInt<uint32_t>(data, offset + 0x28);
    info.loopLength = parseInt<uint32_t>(data, offset + 0x2c);
    info.envelope = data[offset + 0x30] != 0;
    info.envelopeMult = data[offset + 0x31];
    info.attackLevel = data[offset + 0x38];
    info.attackTime = data[offset + 0x39];
    info.decayTime = data[offset + 0x3a];
    info.sustainLevel = data[offset + 0x3b];
    info.holdTime = data[offset + 0x3c];
    info.fadeTime = data[offset + 0x3d];
    info.releaseTime = data[offset + 0x3e];
    sampleInfo[index] = info;
  }
}

std::string WaviChunk::debug(const std::string& prefix) const
{
  std::ostringstream ss;
  for (const auto& iter : sampleInfo) {
    ss << prefix << "Sample #" << iter.first << ":\t0x"
      << std::hex << std::setw(8) << std::setfill('0') << iter.second.sampleStart
      << std::dec << std::setw(0) << " + " << iter.second.byteLength();
    switch (iter.second.format) {
    case SampleInfo::Pcm8:
      ss << " Pcm8" << std::endl;
      break;
    case SampleInfo::Pcm16:
      ss << " Pcm16" << std::endl;
      break;
    case SampleInfo::Adpcm:
      ss << " Adpcm" << std::endl;
      break;
    default:
      ss << " (Unknown)" << std::endl;
      break;
    }
    ss << prefix << "\tRoot: " << int(iter.second.rootKey) << std::endl;
    ss << prefix << "\tTranspose: " << int(iter.second.transpose) << std::endl;
    ss << prefix << "\tTune: " << (int)iter.second.coarseTune << " / " << (int)iter.second.fineTune << std::endl;
    ss << prefix << "\tSample rate: " << iter.second.sampleRate << "Hz" << std::endl;
    ss << prefix << "\tVolume: " << int(iter.second.volume) << std::endl;
    if (iter.second.loop) {
      ss << prefix << "\tLoop: " << int(iter.second.loopStart) << std::endl;
    }
    ss << hexdumpToString((std::vector<uint8_t>)iter.second.data, prefix + "\t") << std::endl;
  }
  return ss.str();
}
