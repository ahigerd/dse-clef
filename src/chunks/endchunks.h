#ifndef ENDCHUNKS_H
#define ENDCHUNKS_H

#include "../dsechunk.h"

#define END_CHUNK(Name, MAGIC) struct Name : public DSEChunkBase<Name, MAGIC> { Name(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset); }
END_CHUNK(EODChunk, 'eod ');
END_CHUNK(EOCChunk, 'eoc ');
#undef END_CHUNK

#endif
