#include "dsefile.h"
#include "dsechunk.h"
#include "dseutil.h"
#include "clefcontext.h"
#include <exception>
#include <iostream>

DSEFile::DSEFile(ClefContext* ctx, const std::string& filename)
: ctx(ctx)
{
  load(readFile(ctx, filename), 0);
}

DSEFile::DSEFile(ClefContext* ctx, const std::vector<uint8_t>& buffer, int offset)
: ctx(ctx)
{
  load(buffer, offset);
}

void DSEFile::load(const std::vector<uint8_t>& buffer, int offset)
{
  uint32_t magic = parseIntBE<uint32_t>(buffer, offset);
  int fileSize = buffer.size();
  int headerSize;
  if (magic == 'swdl') {
    headerSize = 0x50;
  } else if (magic == 'smdl' || magic == 'sedl' || magic == 'sadl') {
    headerSize = 0x40;
  } else {
    throw std::runtime_error("unknown file type " + magicString(magic));
  }
  if (offset + headerSize > fileSize || parseInt<uint32_t>(buffer, offset + 4)) {
    throw std::runtime_error("invalid header in " + magicString(magic) + " file");
  }
  int fileSizeField = parseInt<uint32_t>(buffer, offset + 8);
  if (offset + fileSizeField > fileSize) {
    throw std::runtime_error("invalid file length in " + magicString(magic) + " file");
  }
  data = ConstVectorSlice<uint8_t>(buffer, offset, fileSizeField);
  header = ConstVectorSlice<uint8_t>(data, 0, headerSize);
  fileSize = fileSizeField;
  if (magic == 'sadl') {
    // SADL doesn't use chunks, so treat the file header itself as the chunk header
    offset = 0;
  } else {
    offset = headerSize;
  }
  while (offset < fileSize) {
    chunks.emplace_back(DSEChunk::parse(this, data, offset));
    offset += chunks.back()->size();
  }
}

uint32_t DSEFile::magic() const
{
  return parseIntBE<uint32_t>(header, 0);
}

uint32_t DSEFile::fileLength() const
{
  return data.size();
}

uint16_t DSEFile::version() const
{
  return parseInt<uint16_t>(header, 0x08);
}

int DSEFile::chunkCount() const
{
  return chunks.size();
}

DSEChunk* DSEFile::chunk(int index) const
{
  return chunks.at(index).get();
}

DSEChunk* DSEFile::findChunk(uint32_t magic) const
{
  for (const auto& chunk : chunks) {
    if (chunk->magic() == magic) {
      return chunk.get();
    }
  }
  return nullptr;
}

std::string DSEFile::originalFilename() const
{
  // The round trip through c_str() is to ensure a null terminator.
  return std::string(reinterpret_cast<const char*>(&header[32]), 16).c_str();
}

int DSEFile::sampleCount() const
{
  if (magic() == 'swdl') {
    return parseInt<uint16_t>(header, 0x46);
  }
  return 0;
}

int DSEFile::presetCount() const
{
  if (magic() == 'swdl') {
    return parseInt<uint16_t>(header, 0x48);
  }
  return 0;
}
