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
    Segno = 0x9C,
    DalSegno = 0x9D,
    ToCoda = 0x9E,
    SetOctave = 0xA0,
    AddOctave = 0xA1,
    SetTempo = 0xA4,
    SetTempo2 = 0xA5,
    SetSwdlAndBank = 0xA8,
    SetSwdl = 0xA9,
    SetBank = 0xAA,
    SetPreset = 0xAC,
    SweepSongVolume = 0xAF,
    DisableEnvelope = 0xB0,
    SetEnvelopeAttack = 0xB1,
    SetEnvelopeAttackTime = 0xB2,
    SetEnvelopeHold = 0xB3,
    SetEnvelopeDecaySustain = 0xB4,
    SetEnvelopeFade = 0xB5,
    SetEnvelopeRelease = 0xB6,
    SetNoteVolume = 0xBC,
    SetChannelPan = 0xBE,
    SetChannelVolume = 0xC3,
    SetFineTune = 0xD0,
    AddToFineTune = 0xD1,
    SetCoarseTune = 0xD2,
    AddToTune = 0xD3,
    SweepTune = 0xD4,
    SetRandomNoteRange = 0xD5,
    SetDetuneRange = 0xD6,
    SetPitchBend = 0xD7,
    SetPitchBendRange = 0xDB,
    ReplaceLFO1AsPitch = 0xDC,
    SetLFO1DelayFade = 0xDD,
    SetLFO1ToPitchEnabled = 0xDF,
    SetVolume = 0xE0,
    AddVolume = 0xE1,
    SweepVolume = 0xE2,
    SetExpression = 0xE3,
    ReplaceLFO2AsVolume = 0xE4,
    SetLFO2DelayFade = 0xE5,
    SetLFO2ToVolumeEnabled = 0xE7,
    SetPan = 0xE8,
    AddToPan = 0xE9,
    SweepPan = 0xEA,
    ReplaceLFO3AsPan = 0xEC,
    SetLFO3DelayFade = 0xED,
    SetLFO3ToPanEnabled = 0xEF,
    ReplaceLFO = 0xF0,
    SetLFODelayFade = 0xF1,
    SetLFOParam = 0xF2,
    SetLFORoute = 0xF3,
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
