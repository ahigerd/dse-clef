#ifndef DSEFILE_H
#define DSEFILE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "dsechunk.h"
#include "vectorslice.h"
class S2WContext;

class DSEFile
{
public:
  DSEFile(S2WContext* ctx, const std::string& filename);
  DSEFile(S2WContext* ctx, const std::vector<uint8_t>& buffer, int offset = 0);

  inline S2WContext* context() const { return ctx; }

  uint32_t magic() const;
  uint32_t fileLength() const;
  uint16_t version() const;
  std::string originalFilename() const;

  int chunkCount() const;
  DSEChunk* chunk(int index) const;
  DSEChunk* findChunk(uint32_t magic) const;
  template <typename T> inline T* findChunk() const {
    DSEChunk* c = findChunk(T::Magic);
    return c ? c->cast<T>() : nullptr;
  }
  template <typename T> std::vector<T*> allChunks() const {
    std::vector<T*> result;
    for (const auto& chunk : chunks) {
      if (chunk->magic() == T::Magic) {
        result.push_back(chunk->cast<T>());
      }
    }
    return result;
  }

  int sampleCount() const;
  int presetCount() const;

  inline const ConstVectorSlice<uint8_t>& getHeader() const { return header; }
  inline const std::vector<uint8_t>& getData() const { return data; }

private:
  void load(const std::vector<uint8_t>& buffer, int offset);
  std::vector<uint8_t> data;
  ConstVectorSlice<uint8_t> header;
  std::vector<std::unique_ptr<DSEChunk>> chunks;
  S2WContext* ctx;
};

#endif
