#include "prgichunk.h"
#include "../dseutil.h"
#include "../dsefile.h"
#include <sstream>

REGISTER_CHUNK(PrgiChunk)

PrgiChunk::PrgiChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: PrgiChunk::super(parent, buffer, offset)
{
  int slots = parent->presetCount();
  offset = 0x10 + (slots * 2);
  if (offset & 0x0F) {
    offset = (offset & ~0x0F) + 0x10;
  }
  int dataSize = data.size();
  for (int i = 0; i < slots && offset < dataSize; i++) {
    ProgramInfo info;
    uint16_t progID = parseInt<uint16_t>(data, offset);
    uint16_t numSplits = parseInt<uint16_t>(data, offset + 0x02);
    info.id = progID;
    info.gain = data[offset + 0x04];
    info.pan = data[offset + 0x05];
    uint8_t numLFOs = data[offset + 0x0b];
    offset += 0x10;
    for (int j = 0; j < numLFOs; j++, offset += 0x10) {
      LFOInfo lfo;
      lfo.route = (LFOInfo::Route)data[offset + 0x02];
      lfo.waveform = (LFOInfo::Waveform)data[offset + 0x03];
      lfo.rate = parseInt<uint16_t>(data, offset + 0x04);
      lfo.depth = parseInt<uint16_t>(data, offset + 0x08);
      lfo.delay = parseInt<uint16_t>(data, offset + 0x0a);
      info.lfos.push_back(lfo);
    }
    offset += 0x10;
    for (int j = 0; j < numSplits; j++, offset += 0x30) {
      SplitInfo split;
      split.data = ConstVectorSlice<uint8_t>(data, offset, 0x30);
      split.bendRange = data[offset + 0x02];
      split.lowKey = data[offset + 0x04];
      split.highKey = data[offset + 0x05];
      split.lowVelocity = data[offset + 0x08];
      split.highVelocity = data[offset + 0x09];
      split.sampleIndex = parseInt<uint16_t>(data, offset + 0x12);
      split.fineTune = data[offset + 0x14];
      split.coarseTune = data[offset + 0x15];
      split.rootKey = data[offset + 0x16];
      split.transpose = data[offset + 0x17];
      split.volume = data[offset + 0x18];
      split.pan = data[offset + 0x19];
      split.keygroup = data[offset + 0x1a];
      split.envelope = data[offset + 0x20] != 0;
      split.envelopeMult = data[offset + 0x21];
      split.attackLevel = data[offset + 0x28];
      split.attackTime = data[offset + 0x29];
      split.decayTime = data[offset + 0x2a];
      split.sustainLevel = data[offset + 0x2b];
      split.holdTime = data[offset + 0x2c];
      split.fadeTime = data[offset + 0x2d];
      split.releaseTime = data[offset + 0x2e];

      // TODO: notes suggest that decay/fade/release may not scale the same as attack
      if (!split.decayTime && !split.sustainLevel) {
        // If both decay and sustain are zero, decay is disabled
        split.decayTime = 0x7F;
      }

      info.splits.push_back(split);
    }
    programInfo[progID] = info;
  }
}

std::string PrgiChunk::debug(const std::string& prefix) const
{
  std::ostringstream ss;
  for (const auto& iter : programInfo) {
    ss << prefix << "Program " << (int)iter.first << ": "
      << "gain=" << int(iter.second.gain) << " pan=" << int(iter.second.pan)
      << std::endl;
    for (const auto& split : iter.second.splits) {
      ss << prefix << "\tSplit: (" << int(split.lowKey) << "-" << int(split.highKey) << ")x("
        << int(split.lowVelocity) << "-" << int(split.highVelocity) << ")" << std::endl;
      ss << prefix << "\t\tSample: " << split.sampleIndex << std::endl;
      ss << prefix << "\t\tVolume: " << (int)split.volume << std::endl;
      ss << prefix << "\t\tRoot: " << (int)split.rootKey << " (transpose " << (int)split.transpose << ")" << std::endl;
      ss << prefix << "\t\tTune: " << (int)split.coarseTune << " / " << (int)split.fineTune << std::endl;
      ss << hexdumpToString((std::vector<uint8_t>)split.data, prefix + "\t\t") << std::endl;
    }
    for (const auto& lfo : iter.second.lfos) {
      if (lfo.route == LFOInfo::Disabled) {
        //ss << "Disabled" << std::endl;
        continue;
      }
      ss << prefix << "\tLFO: ";
      switch (lfo.waveform) {
        case LFOInfo::NoWave: ss << "None"; break;
        case LFOInfo::Square: ss << "Square"; break;
        case LFOInfo::Triangle: ss << "Triangle"; break;
        case LFOInfo::Sine: ss << "Sine"; break;
        case LFOInfo::Unknown: ss << "Unknown"; break;
        case LFOInfo::Saw: ss << "Saw"; break;
        case LFOInfo::Random: ss << "Random"; break;
        case LFOInfo::Noise: ss << "Noise"; break;
        default: ss << "???"; break;
      }
      ss << " -> ";
      switch (lfo.route) {
        case LFOInfo::Pitch: ss << "Pitch"; break;
        case LFOInfo::Volume: ss << "Volume"; break;
        case LFOInfo::Pan: ss << "Pan"; break;
        case LFOInfo::Filter: ss << "Filter"; break;
        default: ss << "???"; break;
      }
      ss << std::endl << "\t\t\t\t\tDelay:" << lfo.delay << " Rate:" << lfo.rate
        << " Depth:" << lfo.depth << " Fade:" << lfo.fade << " Cutoff:" << lfo.cutoff << std::endl;
    }
  }
  return ss.str();
}
