#include "trackchunk.h"
#include "../dseutil.h"
#include <sstream>
#include <cmath>
#include <exception>

REGISTER_CHUNK(TrackChunk)

namespace UnusedEvent {
  enum {
    NoOp16 = 0xCB,
    NoOp16_2 = 0xF8,
    NoOp8 = 0xAB,
  };
};

namespace UnknownEvent {
  enum {
    EventD8 = 0xD8,
    EventBF = 0xBF,
    EventC0 = 0xC0,
    EventF6 = 0xF6,
  };
};

TrackChunk::TrackChunk(DSEFile* parent, const std::vector<uint8_t>& buffer, int offset)
: TrackChunk::super(parent, buffer, offset)
{
  int bufferSize = data.size();
  for (int i = 0x14; i < bufferSize; i++) {
    TrkEvent ev;
    ev.eventType = data[i];
    int paramLen = 0;
    switch (ev.eventType) {
      // 16-bit noop
      case UnusedEvent::NoOp16:
      case UnusedEvent::NoOp16_2:
        paramLen = 2;
        break;
      // 8-bit noop
      case UnusedEvent::NoOp8:
        paramLen = 1;
        break;
      // 5-byte
      case TrkEvent::ReplaceLFO1AsPitch:
      case TrkEvent::ReplaceLFO2AsVolume:
      case TrkEvent::ReplaceLFO3AsPan:
      case TrkEvent::ReplaceLFO:
        paramLen = 5;
        break;
      // 4-byte
      case TrkEvent::SetLFO1DelayFade:
      case TrkEvent::SetLFO2DelayFade:
      case TrkEvent::SetLFO3DelayFade:
      case TrkEvent::SetLFODelayFade:
        paramLen = 4;
        break;
      // 3-byte
      case TrkEvent::Rest24:
      case TrkEvent::SweepSongVolume:
      case TrkEvent::SweepTune:
      case TrkEvent::SweepVolume:
      case TrkEvent::SweepPan:
      case TrkEvent::SetLFORoute:
        paramLen = 3;
        break;
      // 2-byte
      case TrkEvent::Rest16:
      case TrkEvent::SetDetuneRange:
      case TrkEvent::SetPitchBend:
      case TrkEvent::SetSwdlAndBank:
      case TrkEvent::SetEnvelopeDecaySustain:
      case TrkEvent::AddToTune:
      case TrkEvent::SetRandomNoteRange:
      case TrkEvent::SetLFOParam:
      case UnknownEvent::EventD8:
        paramLen = 2;
        break;
      // 1-byte
      case TrkEvent::AddRest:
      case TrkEvent::Rest8:
      case TrkEvent::PollSilence:
      case TrkEvent::SetOctave:
      case TrkEvent::AddOctave:
      case TrkEvent::SetTempo:
      case TrkEvent::SetTempo2:
      case TrkEvent::SetPreset:
      case TrkEvent::SetPitchBendRange:
      case TrkEvent::SetVolume:
      case TrkEvent::AddVolume:
      case TrkEvent::SetExpression:
      case TrkEvent::SetPan:
      case TrkEvent::AddToPan:
      case TrkEvent::Segno:
      case TrkEvent::SetSwdl:
      case TrkEvent::SetBank:
      case TrkEvent::SetEnvelopeAttack:
      case TrkEvent::SetEnvelopeAttackTime:
      case TrkEvent::SetEnvelopeHold:
      case TrkEvent::SetEnvelopeFade:
      case TrkEvent::SetEnvelopeRelease:
      case TrkEvent::SetNoteVolume:
      case TrkEvent::SetChannelPan:
      case TrkEvent::SetChannelVolume:
      case TrkEvent::SetFineTune:
      case TrkEvent::AddToFineTune:
      case TrkEvent::SetCoarseTune:
      case TrkEvent::SetLFO1ToPitchEnabled:
      case TrkEvent::SetLFO2ToVolumeEnabled:
      case TrkEvent::SetLFO3ToPanEnabled:
      case UnknownEvent::EventBF:
      case UnknownEvent::EventC0:
      case UnknownEvent::EventF6:
        paramLen = 1;
        break;
      default:
        if (ev.eventType < 0x80) {
          // Note on is variable-length
          uint8_t p1 = data[i + 1];
          paramLen = 1 + (p1 >> 6);
        } else {
          paramLen = 0;
        }
    };
    while (paramLen > 0) {
      ++i;
      --paramLen;
      ev.params.push_back(data[i]);
    }
    events.push_back(ev);
    if (ev.eventType == 0x98) {
      return;
    }
  }
  throw std::runtime_error("no EndOfTrack event found");
}

uint8_t TrackChunk::trackID() const
{
  return data[0x10];
}

uint8_t TrackChunk::channelID() const
{
  return data[0x11];
}

std::string TrackChunk::debug(const std::string& prefix) const
{
  std::ostringstream ss;
  int octave = 0, lastLength = 0, lastRest = 0, tickPos = 0;
  ss << prefix << "Track ID: " << (int)trackID() << std::endl << prefix << "Channel: " << (int)channelID() << std::endl;
  for (const TrkEvent& ev : events) {
    ss << prefix << tickPos << ":\t" << ev.debug(octave, lastLength, lastRest) << std::endl;
    if (ev.eventType == TrkEvent::SetOctave) {
      octave = ev.paramU8();
    } else if (ev.eventType == TrkEvent::AddOctave) {
      octave += ev.paramU8();
    } else if (ev.isRest()) {
      lastRest = ev.duration(lastRest);
      tickPos += lastRest;
    } else if (ev.isPlayNote()) {
      lastLength = ev.duration(lastLength);
    }
  }
  return ss.str();
}

bool TrkEvent::isPlayNote() const
{
  return eventType < 0x80;
}

bool TrkEvent::isRest() const
{
  return eventType >= 0x80 && eventType <= 0x95;
}

int TrkEvent::velocity() const
{
  if (eventType < 0x80) {
    return eventType;
  }
  return 0;
}

int TrkEvent::midiNote(int octave) const
{
  if (eventType < 0x80) {
    return octave * 12 + (params[0] & 0x0F);
  }
  return -1;
}

int TrkEvent::octaveOffset() const
{
  if (eventType < 0x80) {
    return ((params[0] & 0x30) >> 4) - 2;
  }
  return 0;
}

int TrkEvent::duration(int lastDuration) const
{
  if (isPlayNote()) {
    int len = params.size();
    if (len < 2) {
      return lastDuration;
    }
    int duration = 0;
    for (int i = 1; i < len; i++) {
      duration = (duration << 8) | params[i];
    }
    return duration;
  } else if (isRest() && eventType < 0x90) {
    static const int restLength[] = { 96, 72, 64, 48, 36, 32, 24, 18, 16, 12, 9, 8, 6, 4, 3, 2 };
    return restLength[eventType - 0x80];
  } else {
    switch (eventType) {
      case RepeatRest:
        return lastDuration;
      case AddRest:
        return lastDuration + param8();
      case Rest8:
        return paramU8();
      case Rest16:
        return paramU16();
      case Rest24:
        return paramU24();
      default:
        return 0;
    }
  }
}

uint8_t TrkEvent::paramU8() const
{
  return params[0];
}

uint16_t TrkEvent::paramU16() const
{
  return params[0] | (uint8_t(params[1]) << 8);
}

uint16_t TrkEvent::paramU16BE() const
{
  return params[1] | (uint8_t(params[0]) << 8);
}

uint32_t TrkEvent::paramU24() const
{
  return params[0] | (uint8_t(params[1]) << 8) | (uint8_t(params[2]) << 16);
}

double TrkEvent::frequency(int midiNote, double bendSemitones)
{
  return 440.0 * std::pow(2.0, ((midiNote + bendSemitones) - 69) / 12.0);
}

std::string TrkEvent::debug(int octave, int lastLength, int lastRest) const
{
  std::ostringstream ss;
  int d = 0;
  if (isPlayNote()) {
    d = duration(lastLength);
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C'", "C#'", "D'", "D#'" };
    int n = midiNote(octave);
    int o = std::floor(n / 12.0);
    ss << noteNames[n % 12] << (o < 0 ? "+" : "") << o << "@" << velocity();
  } else if (isRest()) {
    d = duration(lastRest);
    ss << "R";
  } else {
    switch (eventType) {
      case 0xCB: case 0xF8: case 0xAB:
        ss << "nop";
        break;
      case RepeatRest:
        ss << "R*";
        break;
      case AddRest:
        ss << "R+";
        break;
      case Rest8:
      case Rest16:
      case Rest24:
        ss << "R";
        break;
      case PollSilence:
        ss << "Poll";
        break;
      case EndOfTrack:
        ss << "End";
        break;
      case LoopPoint:
        ss << "Loop";
        break;
      case SetOctave:
        ss << "O" << (int)param8();
        break;
      case AddOctave:
        ss << "O+" << (int)param8();
        break;
      case SetTempo:
      case SetTempo2:
        ss << "Tempo(" << (unsigned int)paramU8() << ")";
        break;
      case SetPreset:
        ss << "Preset(" << (unsigned int)paramU8() << ")";
        break;
      case SetDetuneRange:
        ss << "Detune(" << (int)param16() << ")";
        break;
      case SetPitchBend:
        ss << "Bend(" << (int)param16() << ")";
        break;
      case SetPitchBendRange:
        ss << "BendRange(" << (int)paramU8() << ")";
        break;
      case SetVolume:
        ss << "Volume(" << (int)param8() << ")";
        break;
      case SetExpression:
        ss << "Expression(" << (int)param8() << ")";
        break;
      case SetPan:
        ss << "Pan(" << (param8() > 0x40 ? "+" : "") << (param8() - 0x40) << ")";
        break;
      default:
        ss << "?" << std::hex << int32_t(eventType) << std::dec;
        if (params.size()) {
          ss << "[" << hexdumpToString(params) << "]";
        }
    }
  }
  if (d > 0) {
    ss << "(" << d << ")";
  }
  return ss.str();
}
