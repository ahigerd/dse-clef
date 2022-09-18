#ifndef SYNTH_INSTRUMENT_H
#define SYNTH_INSTRUMENT_H

#include "../chunks/prgichunk.h"
#include "note.h"
#include "lfo.h"
class TrkEvent;
class DSEContext;

struct Instrument
{
  Instrument();
  Instrument(const ProgramInfo& preset, DSEContext* synth);

  DSEContext* context;
  int programId;
  double gain, pan;

  std::vector<LFO> lfos;
  std::vector<SplitInfo> splits;

  Note startNote(const TrkEvent& ev, double time, int octave, int lastLength) const;
};

#endif
