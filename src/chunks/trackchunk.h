#ifndef TRACKCHUNK_H
#define TRACKCHUNK_H

#include "../dsechunk.h"

struct TrkEvent
{
  static double frequency(int midiNote, double bendSemitones = 0);

  enum EventType {
    RepeatRest = 0x90,
    AddRest = 0x91,
    Rest8 = 0x92,
    Rest16 = 0x93,
    Rest24 = 0x94,
    PollSilence = 0x95,
    EndOfTrack = 0x98,
    LoopPoint = 0x99,
    SetOctave = 0xA0,
    AddOctave = 0xA1,
    SetTempo = 0xA4,
    SetTempo2 = 0xA5,
    SetPreset = 0xAC,
    SetDetuneRange = 0xD6,
    SetPitchBend = 0xD7,
    SetPitchBendRange = 0xDB,
    SetVolume = 0xE0,
    SetExpression = 0xE3,
    SetPan = 0xE8,
  };
  uint8_t eventType;
  std::vector<uint8_t> params;

  bool isPlayNote() const;
  bool isRest() const;
  int velocity() const;
  int midiNote(int octave) const;
  int octaveOffset() const;
  int duration(int lastDuration = 0) const;

  uint8_t paramU8() const;
  uint16_t paramU16() const;
  uint16_t paramU16BE() const;
  uint32_t paramU24() const;
  inline int8_t param8() const { return paramU8(); }
  inline int16_t param16() const { return paramU16(); }
  inline int16_t param16BE() const { return paramU16BE(); }
  inline int32_t param24() const { return int32_t(paramU24() << 8) >> 8; }

  std::string debug(int octave, int lastLength, int lastRest) const;
};

class TrackChunk : public DSEChunkBase<TrackChunk, 'trk '>
{
public:
  TrackChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset);

  uint8_t trackID() const;
  uint8_t channelID() const;
  std::vector<TrkEvent> events;

  virtual std::string debug(const std::string& prefix = std::string()) const;
};

#endif
