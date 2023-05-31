#include "endchunks.h"

#define END_CHUNK(Name) Name::Name(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset) : Name::super(parent, buffer, offset) {}
END_CHUNK(EODChunk);
END_CHUNK(EOCChunk);
