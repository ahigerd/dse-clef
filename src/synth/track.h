#ifndef SYNTH_TRACK_H
#define SYNTH_TRACK_H

#include "seq/itrack.h"
#include "note.h"
#include <vector>
#include <utility>
#include <set>
class TrackChunk;
class DSEContext;
class Instrument;

struct Track : public ITrack
{
  Track(const TrackChunk* track, DSEContext* synth);

  const TrackChunk* const trk;
  DSEContext* context;
  const Instrument* currentInstrument;
  int eventPos, tickPos, samplePos;
  std::set<int>::const_iterator timingIter, timingEnd;

  int lastRestLength, lastNoteLength, octave, bendRange;
  double volume, detune, pitchBend, expression, pan, gain;
  double samplesPerTick;

  virtual bool isFinished() const;
  virtual double length() const;

protected:
  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  virtual void internalReset();
};

#endif
