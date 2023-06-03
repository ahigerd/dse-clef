#ifndef SYNTH_TRACK_H
#define SYNTH_TRACK_H

#include "seq/itrack.h"
#include <vector>
#include <utility>
#include <set>
class TrackChunk;
class DSEContext;
struct Instrument;

struct Track : public ITrack
{
  Track(const TrackChunk* track, DSEContext* synth);

  const TrackChunk* const trk;
  DSEContext* context;
  const Instrument* currentInstrument;
  int eventPos, tickPos, samplePos, channelID, channelEventPos;
  std::set<int>::const_iterator timingIter, timingEnd;

  int lastRestLength, lastNoteLength, octave, bendRange;
  double volume, channelVolume, detune, pitchBend, expression, pan, channelPan, totalGain, totalPan;
  double samplesPerTick;

  virtual bool isFinished() const;
  virtual double length() const;

  void enqueueEvent(std::shared_ptr<SequenceEvent> event);

protected:
  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  void internalReset();
  SequenceEvent* updateTotalGain();
  SequenceEvent* updateTotalPan();
};

#endif
