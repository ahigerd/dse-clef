#include "dsechunk.h"
#include "dseutil.h"
#include "chunks/endchunks.h"
#include "chunks/kgrpchunk.h"
#include "chunks/pcmdchunk.h"
#include "chunks/prgichunk.h"
#include "chunks/sadlchunk.h"
#include "chunks/songchunk.h"
#include "chunks/trackchunk.h"
#include "chunks/wavichunk.h"
#include <map>

struct IRegistrar {
  virtual DSEChunk* create(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset) const = 0;
};

template <typename ChunkType>
struct ChunkRegistrar : public IRegistrar {
  DSEChunk* create(DSEFile* p, const std::vector<uint8_t>& b, int o) const {
    return new ChunkType(p, b, o);
  }
};

static std::map<uint32_t, IRegistrar*>& chunkRegistry()
{
  static std::map<uint32_t, IRegistrar*> registry{
    { 'eoc ', new ChunkRegistrar<EOCChunk>() },
    { 'eod ', new ChunkRegistrar<EODChunk>() },
    { 'kgrp', new ChunkRegistrar<KgrpChunk>() },
    { 'pcmd', new ChunkRegistrar<PcmdChunk>() },
    { 'prgi', new ChunkRegistrar<PrgiChunk>() },
    { 'sadl', new ChunkRegistrar<SadlChunk>() },
    { 'song', new ChunkRegistrar<SongChunk>() },
    { 'trk ', new ChunkRegistrar<TrackChunk>() },
    { 'wavi', new ChunkRegistrar<WaviChunk>() },
  };
  return registry;
}

DSEChunk::DSEChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: m_parent(parent)
{
  uint32_t magic = parseIntBE<uint32_t>(buffer, offset);
  uint32_t size;
  if (magic == 'song') {
    size = 64;
  } else if (magic == 'sadl') {
    size = parseInt<uint32_t>(buffer, offset + 8);
    if (size < 0x100) {
      size = 0x100;
    }
  } else {
    size = parseInt<uint32_t>(buffer, offset + 12) + 16;
    // Chunks must be 32-bit aligned
    if (size & 0x03) {
      size = (size & ~0x03) + 0x04;
    }
  }

  // There may or may not be padding after the end of the chunk before
  // the next chunk. Seek forward to find either the end of the buffer
  // or the next valid magic number.
  int bufferSize = buffer.size();
  while (offset + size < bufferSize) {
    if (isValidMagic64(parseIntBE<uint64_t>(buffer, offset + size))) {
      break;
    }
    size += 4;
  }

  if (offset + size > buffer.size()) {
    throw std::runtime_error("invalid chunk length in " + magicString(magic) + " chunk");
  }
  data = ConstVectorSlice<uint8_t>(buffer, offset, size);
  //hexdump(data);
}

int DSEChunk::size() const
{
  return data.size();
}

std::string DSEChunk::debug(const std::string& prefix) const
{
  // Default implementation has no debug representation
  return std::string();
}

DSEChunk* DSEChunk::parse(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
{
  uint32_t magic = parseIntBE<uint32_t>(buffer, offset);
  auto registrar = chunkRegistry().find(magic);
  if (registrar == chunkRegistry().end()) {
    return new UnknownChunk(parent, buffer, offset);
  }
  return registrar->second->create(parent, buffer, offset);
}

UnknownChunk::UnknownChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: DSEChunk(parent, buffer, offset)
{
  // initializers only
}

uint32_t UnknownChunk::magic() const
{
  return parseIntBE<uint32_t>(data, 0);
}
