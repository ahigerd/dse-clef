#include "kgrpchunk.h"
#include "../dseutil.h"

REGISTER_CHUNK(KgrpChunk)

KgrpChunk::KgrpChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: KgrpChunk::super(parent, buffer, offset)
{
  int dataSize = data.size();
  offset = 0x10;
  uint16_t counter = 0;
  while (offset < dataSize) {
    uint16_t groupID = parseInt<uint16_t>(data, offset);
    if (groupID != counter) {
      break;
    }
    KeyGroup group;
    group.polyphony = data[offset + 2];
    group.priority = data[offset + 3];
    group.minChannel = data[offset + 4];
    group.maxChannel = data[offset + 5];
    groups.push_back(group);
    offset += 8;
    counter++;
  }
}
