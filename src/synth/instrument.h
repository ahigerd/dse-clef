#ifndef SYNTH_INSTRUMENT_H
#define SYNTH_INSTRUMENT_H

#include "synth/iinstrument.h"
#include "seq/sequenceevent.h"
#include "../chunks/prgichunk.h"
#include "lfo.h"
class TrkEvent;
class DSEContext;
class Track;

struct Instrument : public DefaultInstrument
{
  Instrument(DSEContext* synth);
  Instrument(const ProgramInfo& preset, DSEContext* synth);

  DSEContext* context;
  int programId;
  double gain, pan;

  std::vector<LFO> lfos;
  std::vector<SplitInfo> splits;

  BaseNoteEvent* makeEvent(Track* track, const TrkEvent& ev) const;
};

#endif
