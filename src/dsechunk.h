#ifndef DSECHUNK_H
#define DSECHUNK_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include "vectorslice.h"
class DSEFile;

class DSEChunk
{
public:
  struct Registrar {
    virtual DSEChunk* create(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset) const = 0;
  };
  static void registerType(uint32_t magic, Registrar* rr);

  static DSEChunk* parse(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset = 0);

  virtual ~DSEChunk() {}
  virtual uint32_t magic() const = 0;
  int size() const;
  inline DSEFile* parent() const { return m_parent; }
  virtual std::string debug(const std::string& prefix = std::string()) const;

  template<typename T> const T* cast() const {
    return (magic() == T::Magic) ? static_cast<const T*>(this) : nullptr;
  }

  template<typename T> T* cast() {
    return (magic() == T::Magic) ? static_cast<T*>(this) : nullptr;
  }

protected:
  DSEChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset = 0);
  std::vector<uint8_t> data;
  //ConstVectorSlice<uint8_t> data;

private:
  DSEFile* m_parent;
};

template <class DERIVED, uint32_t MAGIC>
class DSEChunkBase : public DSEChunk
{
public:
  enum { Magic = MAGIC };
  virtual uint32_t magic() const final { return MAGIC; }

protected:
  using super = DSEChunkBase;
  DSEChunkBase(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset = 0)
    : DSEChunk(parent, buffer, offset) {}
};

#define REGISTER_CHUNK(ChunkType) namespace ChunkType ## R { static struct Registrar : public DSEChunk::Registrar {\
  Registrar() { DSEChunk::registerType(ChunkType::Magic, this); } \
  DSEChunk* create(DSEFile* p, const std::vector<uint8_t>& b, int o) const { return new ChunkType(p, b, o); } \
} registrar; }

class UnknownChunk : public DSEChunk
{
public:
  UnknownChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);
  virtual uint32_t magic() const;
};

#endif
