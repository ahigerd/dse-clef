#ifndef SYNTH_TRACK_H
#define SYNTH_TRACK_H

#include "note.h"
#include <vector>
#include <utility>
#include <set>
class TrackChunk;
class DSEContext;
class Instrument;

struct Track
{
  Track(const TrackChunk* track, DSEContext* synth);

  const TrackChunk* const trk;
  DSEContext* context;
  const Instrument* currentInstrument;
  int eventPos, tickPos, samplePos;
  std::set<int>::const_iterator timingIter, timingEnd;

  int lastRestLength, lastNoteLength, octave, bendRange;
  double volume, detune, pitchBend, expression, leftGain, rightGain;
  double samplesPerTick;

  std::vector<Note> notes;

  std::pair<double, double> gain() const;
  bool processEvent(std::vector<int16_t>& buffer);
};

#endif
